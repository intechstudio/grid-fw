#include "grid_esp32_http.h"

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char* TAG = "HTTP";

static httpd_handle_t http_server = NULL;
static int ws_client_fd = -1;
static volatile bool ws_busy = false;

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
                                 "ws.onopen = () => log.innerHTML += '[Connected]\\n';"
                                 "ws.onclose = () => log.innerHTML += '[Disconnected]\\n';"
                                 "ws.onmessage = (e) => { log.innerHTML += e.data; log.scrollTop = log.scrollHeight; };"
                                 "</script>"
                                 "</body>"
                                 "</html>";

static esp_err_t index_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, index_html, sizeof(index_html) - 1);
  return ESP_OK;
}

static void session_close_handler(httpd_handle_t hd, int fd) {
  if (fd == ws_client_fd) {
    ESP_LOGI(TAG, "WS client disconnected: fd=%d", fd);
    ws_client_fd = -1;
  }
}

static esp_err_t ws_handler(httpd_req_t* req) {
  if (req->method == HTTP_GET) {
    ws_client_fd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WS client connected: fd=%d", ws_client_fd);
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

esp_err_t grid_esp32_ws_broadcast(const char* data, size_t len) {
  // Skip if no client or already busy
  if (ws_client_fd < 0 || http_server == NULL || ws_busy) {
    return ESP_ERR_INVALID_STATE;
  }

  // Mark as busy to prevent re-entrant calls
  ws_busy = true;

  httpd_ws_frame_t ws_pkt = {
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t*)data,
      .len = len,
  };

  httpd_ws_send_frame_async(http_server, ws_client_fd, &ws_pkt);

  ws_busy = false;

  return ESP_OK;
}

esp_err_t grid_esp32_http_init(void) {
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
