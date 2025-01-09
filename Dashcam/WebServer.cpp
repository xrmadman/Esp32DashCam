#include <WiFi.h>
#include <esp_camera.h>
#include <SD_MMC.h>
#include "esp_http_server.h"
#include "WebServer.h"
#include "CameraControl.h"

const char *ssid = "dashcam";
const char *password = "Michelle236";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[ERROR] Camera capture failed.");
      res = ESP_FAIL;
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n", 2);
    }

    if (fb) {
      esp_camera_fb_return(fb);
    }

    if (res != ESP_OK) break;
  }
  return res;
}

static esp_err_t command_handler(httpd_req_t *req) {
  char buf[32];
  int ret = httpd_req_recv(req, buf, sizeof(buf));

  if (ret > 0) {
    buf[ret] = '\0';
    if (strcmp(buf, "start") == 0) {
      CameraControl::startRecording();
    } else if (strcmp(buf, "stop") == 0) {
      CameraControl::stopRecording();
    } else if (strcmp(buf, "photo") == 0) {
      CameraControl::capturePhoto();
    } else if (strncmp(buf, "resolution=", 11) == 0) {
      int res = atoi(buf + 11);
      CameraControl::setResolution(res);
    }
  }

  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t main_page_handler(httpd_req_t *req) {
  const char HTML_PAGE[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>ESP32-CAM Dashcam</title>
        <style>
            body { font-family: Arial, sans-serif; text-align: center; background: #222; color: white; }
            h1 { margin-top: 10px; }
            #stream { width: 100%; max-width: 640px; border: 2px solid white; }
            button { background: #444; color: white; padding: 10px; border: none; margin: 5px; cursor: pointer; }
            button:hover { background: #666; }
        </style>
    </head>
    <body>
        <h1>ESP32-CAM Dashcam</h1>
        <img id="stream" src="/stream" alt="Live Stream">
        <br>
        <button onclick="sendCommand('start')">Start Recording</button>
        <button onclick="sendCommand('stop')">Stop Recording</button>
        <button onclick="sendCommand('photo')">Capture Photo</button>
        <br>
        <label for="resolution">Resolution:</label>
        <select id="resolution" onchange="changeResolution()">
            <option value="8">UXGA (1600x1200)</option>
            <option value="7">SXGA (1280x1024)</option>
            <option value="6">XGA (1024x768)</option>
            <option value="5">SVGA (800x600)</option>
            <option value="4">VGA (640x480)</option>
            <option value="3">CIF (352x288)</option>
        </select>
        <script>
            function sendCommand(cmd) {
                fetch("/command", {
                    method: "POST",
                    body: cmd
                });
            }
            function changeResolution() {
                var res = document.getElementById("resolution").value;
                fetch("/command", {
                    method: "POST",
                    body: "resolution=" + res
                });
            }
        </script>
    </body>
    </html>
    )rawliteral";

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, HTML_PAGE, strlen(HTML_PAGE));
}

void WebServer::initServer() {
  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_uri_t uri_main = { .uri = "/", .method = HTTP_GET, .handler = main_page_handler, .user_ctx = NULL };
    httpd_uri_t uri_stream = { .uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL };
    httpd_uri_t uri_command = { .uri = "/command", .method = HTTP_POST, .handler = command_handler, .user_ctx = NULL };

    httpd_register_uri_handler(stream_httpd, &uri_main);
    httpd_register_uri_handler(stream_httpd, &uri_stream);
    httpd_register_uri_handler(stream_httpd, &uri_command);

    Serial.println("[OK] Web server started.");
  } else {
    Serial.println("[ERROR] Failed to start web server.");
  }
}
