/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grid_esp32_usb_ncm.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include <string.h>

#if CFG_TUD_NCM

#include "class/net/net_device.h"
#include "dhserver.h"
#include "dnserver.h"
#include "freertos/semphr.h"
#include "lwip/ethip6.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"

static const char* TAG = "USB_NCM";

// MAC address for NCM device (first byte 0x02 indicates locally administered)
uint8_t tud_network_mac_address[6] = {0x02, 0x50, 0x4F, 0x4E, 0x45, 0x54}; // "PONET" in hex

// lwIP netif for USB network
static struct netif ncm_netif_data;
static bool ncm_netif_initialized = false;

// Network configuration - Device is 192.168.7.1, host gets 192.168.7.2
#define INIT_IP4(a, b, c, d)                                                                                                                                                                           \
  { PP_HTONL(LWIP_MAKEU32(a, b, c, d)) }

static const ip4_addr_t ncm_ipaddr = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t ncm_netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t ncm_gateway = INIT_IP4(0, 0, 0, 0);

// DHCP entries - addresses that can be offered to the host
static dhcp_entry_t dhcp_entries[] = {
    {{0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router = INIT_IP4(0, 0, 0, 0), .port = 67, .dns = INIT_IP4(192, 168, 7, 1), .domain = "usb", .num_entry = sizeof(dhcp_entries) / sizeof(dhcp_entries[0]), .entries = dhcp_entries};

// DNS query handler - resolve "grid.usb" to device IP
static bool dns_query_proc(const char* name, ip4_addr_t* addr) {
  if (strcmp(name, "grid.usb") == 0) {
    *addr = ncm_ipaddr;
    return true;
  }
  return false;
}

// lwIP linkoutput function - sends packets from lwIP to USB
static err_t ncm_linkoutput_fn(struct netif* netif, struct pbuf* p) {
  (void)netif;

  if (!tud_ready()) {
    return ERR_USE;
  }

  // Try a few times with tud_task() calls in between
  for (int i = 0; i < 10; i++) {
    if (tud_network_can_xmit(p->tot_len)) {
      tud_network_xmit(p, 0);
      return ERR_OK;
    }
    tud_task();
  }

  // Can't transmit now, let lwIP retry later
  return ERR_WOULDBLOCK;
}

// lwIP IPv4 output function
static err_t ncm_ip4_output_fn(struct netif* netif, struct pbuf* p, const ip4_addr_t* addr) { return etharp_output(netif, p, addr); }

// lwIP netif init callback
static err_t ncm_netif_init_cb(struct netif* netif) {
  netif->mtu = CFG_TUD_NET_MTU;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'U';
  netif->name[1] = 'S';
  netif->linkoutput = ncm_linkoutput_fn;
  netif->output = ncm_ip4_output_fn;
  return ERR_OK;
}

// Semaphore for tcpip_init synchronization
static SemaphoreHandle_t tcpip_init_done;

// Callback when tcpip thread is initialized
static void tcpip_init_done_cb(void* arg) { xSemaphoreGive(tcpip_init_done); }

// Initialize the NCM network interface
static void ncm_netif_init(void) {
  // Initialize lwIP TCPIP thread (ESP-IDF lwIP uses threaded mode)
  tcpip_init_done = xSemaphoreCreateBinary();
  tcpip_init(tcpip_init_done_cb, NULL);
  xSemaphoreTake(tcpip_init_done, portMAX_DELAY);
  vSemaphoreDelete(tcpip_init_done);
  ESP_LOGI(TAG, "lwIP TCPIP thread initialized");

  struct netif* netif = &ncm_netif_data;

  // Set MAC address (toggle LSB to differ from host)
  netif->hwaddr_len = sizeof(tud_network_mac_address);
  memcpy(netif->hwaddr, tud_network_mac_address, sizeof(tud_network_mac_address));
  netif->hwaddr[5] ^= 0x01;

  // Add netif to lwIP (use tcpip_input for thread-safe input)
  netif_add(netif, &ncm_ipaddr, &ncm_netmask, &ncm_gateway, NULL, ncm_netif_init_cb, tcpip_input);
  netif_set_default(netif);
  netif_set_link_up(netif);

  // Initialize DHCP server
  while (dhserv_init(&dhcp_config) != ERR_OK) {
    ESP_LOGE(TAG, "DHCP server init failed, retrying...");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "DHCP server started");

  // Initialize DNS server
  while (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK) {
    ESP_LOGE(TAG, "DNS server init failed, retrying...");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "DNS server started");

  ncm_netif_initialized = true;
  ESP_LOGI(TAG, "NCM network interface ready at 192.168.7.1");
}

// ========================= NCM TinyUSB Callbacks =============================== //

// Called when network driver is initialized by TinyUSB
void tud_network_init_cb(void) {
  ESP_LOGI(TAG, "NCM network initialized by TinyUSB");
  // Notify USB host that link is up
  tud_network_link_state(0, true);
}

// Called when a packet is received from the host
bool tud_network_recv_cb(const uint8_t* src, uint16_t size) {
  if (!ncm_netif_initialized || size == 0) {
    return true; // Accept but discard if not ready
  }

  struct netif* netif = &ncm_netif_data;
  struct pbuf* p = pbuf_alloc(PBUF_RAW, size, PBUF_RAM);

  if (p == NULL) {
    ESP_LOGW(TAG, "Failed to allocate pbuf for %d bytes", size);
    return false; // Reject, try again later
  }

  // Copy received data to pbuf
  pbuf_take(p, src, size);

  // Pass to lwIP network stack
  if (netif->input(p, netif) != ERR_OK) {
    ESP_LOGW(TAG, "netif input failed");
    pbuf_free(p);
  }

  // Signal TinyUSB to prepare for next packet
  tud_network_recv_renew();

  return true;
}

// Called by TinyUSB to copy transmit data to USB buffer
uint16_t tud_network_xmit_cb(uint8_t* dst, void* ref, uint16_t arg) {
  struct pbuf* p = (struct pbuf*)ref;
  (void)arg;

  return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

// Service function - in ESP-IDF threaded lwIP mode, timeouts are handled by TCPIP thread
void grid_platform_ncm_service(void) {
  // Nothing needed - TCPIP thread handles lwIP timers
}

// Initialize NCM networking - call after USB init
void grid_platform_ncm_init(void) { ncm_netif_init(); }

#else // !CFG_TUD_NCM - stub implementations

void grid_platform_ncm_service(void) {}
void grid_platform_ncm_init(void) {}

#endif // CFG_TUD_NCM
