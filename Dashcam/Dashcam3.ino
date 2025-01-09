#include <Arduino.h>
#include "CameraControl.h"
#include "WebServer.h"
#include "WiFiManager.h"
#include <SD_MMC.h>
#include <ESPmDNS.h>

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-CAM Dashcam...");

  if (!CameraControl::initCamera()) {
    Serial.println("[ERROR] Restarting ESP32-CAM...");
    delay(5000);
    ESP.restart();
  }

  Serial.println("Initializing SD Card...");
  if (!SD_MMC.begin()) {
    Serial.println("[ERROR] Failed to initialize SD card!");
  } else {
    Serial.println("[OK] SD Card Initialized.");
  }

  WiFiManager::initWiFi();

  if (MDNS.begin("dashcam")) {
    Serial.println("[OK] mDNS responder started (dashcam.local)");
  }

  WebServer::initServer();

  CameraControl::startRecording();
}

void loop() {
  delay(10);
}
