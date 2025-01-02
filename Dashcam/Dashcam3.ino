#include "WiFiManager.h"
#include "FileManager.h"
#include "WebServer.h"
#include "CameraControl.h"

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!WiFiManager::initWiFi()) {
    Serial.println("Wi-Fi initialization failed.");
  }

  if (!FileManager::initSDCard()) {
    Serial.println("SD card initialization failed.");
  }

  if (!CameraControl::initCamera()) {
    Serial.println("Camera initialization failed.");
  }

  WebServer::initServer();
  Serial.println("Setup complete.");
}

void loop() {
  WebServer::handleClient();
}
