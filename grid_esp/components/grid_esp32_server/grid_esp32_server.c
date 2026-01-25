#include "grid_esp32_server.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char* TAG = "SERVER";

static httpd_handle_t http_server = NULL;
static volatile bool ws_busy = false;

// Multi-client support
#define WS_MAX_CLIENTS 4
static int ws_clients[WS_MAX_CLIENTS] = {-1, -1, -1, -1};
static int ws_client_count = 0;

static bool ws_add_client(int fd) {
  if (ws_client_count >= WS_MAX_CLIENTS) {
    return false;
  }
  for (int i = 0; i < WS_MAX_CLIENTS; i++) {
    if (ws_clients[i] < 0) {
      ws_clients[i] = fd;
      ws_client_count++;
      return true;
    }
  }
  return false;
}

static void ws_remove_client(int fd) {
  for (int i = 0; i < WS_MAX_CLIENTS; i++) {
    if (ws_clients[i] == fd) {
      ws_clients[i] = -1;
      ws_client_count--;
      return;
    }
  }
}

static const char index_html[] = "<!DOCTYPE html>"
                                 "<html>"
                                 "<head>"
                                 "<meta charset=\"UTF-8\">"
                                 "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                                 "<title>Grid</title>"
                                 "<style>"
                                 "body { font-family: system-ui, sans-serif; max-width: 600px; margin: 40px auto; padding: 20px; }"
                                 "h1 { color: #333; }"
                                 "#log { background: #f0f0f0; padding: 10px; font-family: monospace; height: 300px; overflow-y: auto; white-space: pre-wrap; word-break: break-all; }"
                                 "</style>"
                                 "</head>"
                                 "<body>"
                                 "<h1>Grid Device</h1>"
                                 "<p>WebSocket mirror of CDC-ACM output</p>"
                                 "<div id=\"log\"></div>"
                                 "<script>"
                                 "const log = document.getElementById('log');"
                                 "const ws = new WebSocket('ws://' + location.host + '/ws');"
                                 "ws.binaryType = 'arraybuffer';"
                                 "ws.onopen = () => log.innerHTML += '[Connected]\\n';"
                                 "ws.onclose = () => log.innerHTML += '[Disconnected]\\n';"
                                 "ws.onmessage = (e) => { const text = new TextDecoder().decode(e.data); log.innerHTML += text; log.scrollTop = log.scrollHeight; };"
                                 "</script>"
                                 "</body>"
                                 "</html>";

static esp_err_t index_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, index_html, sizeof(index_html) - 1);
  return ESP_OK;
}

static void session_close_handler(httpd_handle_t hd, int fd) {
  ws_remove_client(fd);
  ESP_LOGI(TAG, "WS client disconnected: fd=%d (total: %d)", fd, ws_client_count);
}

static esp_err_t ws_handler(httpd_req_t* req) {
  if (req->method == HTTP_GET) {
    int fd = httpd_req_to_sockfd(req);
    if (ws_add_client(fd)) {
      ESP_LOGI(TAG, "WS client connected: fd=%d (total: %d)", fd, ws_client_count);
    } else {
      ESP_LOGW(TAG, "WS client rejected, max clients reached: fd=%d", fd);
      return ESP_FAIL;
    }
    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(ws_pkt));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) {
    return ret;
  }

  if (ws_pkt.len > 0) {
    uint8_t* buf = malloc(ws_pkt.len + 1);
    ws_pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret == ESP_OK) {
      buf[ws_pkt.len] = '\0';
      ESP_LOGI(TAG, "WS received: %s", buf);
    }
    free(buf);
  }

  return ret;
}

esp_err_t grid_esp32_server_ws_broadcast(const char* data, size_t len) {
  if (ws_client_count == 0 || http_server == NULL || ws_busy) {
    return ESP_ERR_INVALID_STATE;
  }

  ws_busy = true;

  httpd_ws_frame_t ws_pkt = {
      .type = HTTPD_WS_TYPE_BINARY,
      .payload = (uint8_t*)data,
      .len = len,
  };

  for (int i = 0; i < WS_MAX_CLIENTS; i++) {
    if (ws_clients[i] >= 0) {
      httpd_ws_send_frame_async(http_server, ws_clients[i], &ws_pkt);
    }
  }

  ws_busy = false;

  return ESP_OK;
}

int32_t grid_platform_websocket_ready(void) { return (ws_client_count > 0 && http_server != NULL && !ws_busy) ? 1 : 0; }

int32_t grid_platform_websocket_write(char* buffer, uint32_t length) {
  if (ws_client_count == 0 || http_server == NULL || ws_busy) {
    return 0;
  }

  ws_busy = true;

  httpd_ws_frame_t ws_pkt = {
      .type = HTTPD_WS_TYPE_BINARY,
      .payload = (uint8_t*)buffer,
      .len = length,
  };

  for (int i = 0; i < WS_MAX_CLIENTS; i++) {
    if (ws_clients[i] >= 0) {
      httpd_ws_send_frame_async(http_server, ws_clients[i], &ws_pkt);
    }
  }

  ws_busy = false;

  return 1;
}

esp_err_t grid_esp32_server_init(void) {
  esp_event_loop_create_default();

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.close_fn = session_close_handler;

  esp_err_t ret = httpd_start(&http_server, &config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
    return ret;
  }

  httpd_uri_t index_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = index_handler,
  };
  httpd_register_uri_handler(http_server, &index_uri);

  httpd_uri_t ws_uri = {
      .uri = "/ws",
      .method = HTTP_GET,
      .handler = ws_handler,
      .is_websocket = true,
  };
  httpd_register_uri_handler(http_server, &ws_uri);

  ESP_LOGI(TAG, "HTTP server started on port 80");
  return ESP_OK;
}
