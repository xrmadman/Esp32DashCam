#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "WebServer.h"
#include "CameraControl.h"
#include <WiFi.h>
#include <esp_camera.h>

AsyncWebServer server(80);

namespace WebServer {
void initServer() {
  // Live stream endpoint
  server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->sendChunked("multipart/x-mixed-replace; boundary=frame", [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed.");
        return 0;
      }

      size_t headerLen = snprintf((char *)buffer, maxLen, "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
      if (index == 0) {
        memcpy(buffer, fb->buf, maxLen);
        esp_camera_fb_return(fb);
        return fb->len + headerLen;
      }
      return 0;
    });
  });

  // Main web interface
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
                <!DOCTYPE html>
                <html>
                <head>
                    <title>ESP32-CAM Dashcam</title>
                    <style>
                        body { font-family: Arial; text-align: center; }
                        select, button, input { margin: 10px; padding: 10px; font-size: 16px; }
                        video { width: 100%; max-width: 400px; }
                    </style>
                </head>
                <body>
                    <h1>ESP32-CAM Dashcam</h1>
                    <video id="stream" autoplay></video>
                    <script>
                        const video = document.getElementById("stream");
                        video.src = "http://" + window.location.hostname + ":81/stream";

                        function updateSettings() {
                            const resolution = document.getElementById("resolution").value;
                            const framerate = document.getElementById("framerate").value;
                            fetch(`/settings?resolution=${resolution}&framerate=${framerate}`);
                        }

                        function scanNetworks() {
                            fetch('/scan')
                                .then(response => response.text())
                                .then(data => {
                                    document.getElementById('networks').innerHTML = data;
                                });
                        }

                        function connectNetwork() {
                            const ssid = document.getElementById('ssid').value;
                            const password = document.getElementById('password').value;
                            fetch('/connect', {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                                body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
                            }).then(response => response.text())
                              .then(alert);
                        }
                    </script>
                    <div>
                        <button onclick="fetch('/start')">Start Recording</button>
                        <button onclick="fetch('/stop')">Stop Recording</button>
                        <button onclick="fetch('/photo')">Capture Photo</button>
                    </div>
                    <div>
                        <label for="resolution">Resolution:</label>
                        <select id="resolution">
                            <option value="10">SVGA (800x600)</option>
                            <option value="9">VGA (640x480)</option>
                            <option value="7">QVGA (320x240)</option>
                        </select>
                        <label for="framerate">Frame Rate:</label>
                        <select id="framerate">
                            <option value="5">5 FPS</option>
                            <option value="10">10 FPS</option>
                            <option value="12">12 FPS</option>
                        </select>
                        <button onclick="updateSettings()">Update Settings</button>
                    </div>
                    <div>
                        <h2>Wi-Fi Settings</h2>
                        <button onclick="scanNetworks()">Scan Networks</button>
                        <div id="networks"></div>
                        <form onsubmit="event.preventDefault(); connectNetwork();">
                            <label for="ssid">SSID:</label>
                            <input id="ssid" type="text" required>
                            <label for="password">Password:</label>
                            <input id="password" type="password">
                            <button type="submit">Connect</button>
                        </form>
                    </div>
                </body>
                </html>
            )rawliteral");
  });

  // Settings update endpoint
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("resolution") && request->hasParam("framerate")) {
      int resolution = request->getParam("resolution")->value().toInt();
      int framerate = request->getParam("framerate")->value().toInt();
      CameraControl::setResolution(resolution);
      CameraControl::setFrameRate(framerate);
      request->send(200, "text/plain", "Settings updated.");
    } else {
      request->send(400, "text/plain", "Invalid settings.");
    }
  });

  // Wi-Fi scanning endpoint
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanNetworks();
    String networks = "<ul>";
    for (int i = 0; i < n; ++i) {
      networks += "<li>" + WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + " dBm)</li>";
    }
    networks += "</ul>";
    request->send(200, "text/html", networks);
  });

  // Wi-Fi connect endpoint
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();
      WiFi.begin(ssid.c_str(), password.c_str());
      request->send(200, "text/plain", "Attempting to connect to Wi-Fi.");
    } else {
      request->send(400, "text/plain", "Invalid parameters.");
    }
  });

  // Camera control endpoints
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (CameraControl::startRecording()) {
      request->send(200, "text/plain", "Recording started.");
    } else {
      request->send(500, "text/plain", "Failed to start recording.");
    }
  });

  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (CameraControl::stopRecording()) {
      request->send(200, "text/plain", "Recording stopped.");
    } else {
      request->send(500, "text/plain", "Failed to stop recording.");
    }
  });

  server.on("/photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (CameraControl::capturePhoto()) {
      request->send(200, "text/plain", "Photo captured.");
    } else {
      request->send(500, "text/plain", "Failed to capture photo.");
    }
  });

  server.begin();
}

void handleClient() {
  // ESPAsyncWebServer handles clients asynchronously
}
}
