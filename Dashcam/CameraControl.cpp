#include "CameraControl.h"
#include <esp_camera.h>
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

namespace CameraControl {
bool recording = false;
int currentResolution = FRAMESIZE_VGA;
File videoFile;

bool initCamera() {
  Serial.println("Initializing Camera...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = (framesize_t)currentResolution;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("[ERROR] Camera Initialization Failed!");
    return false;
  }

  Serial.println("[OK] Camera Initialized Successfully!");
  return true;
}

bool startRecording() {
  if (recording) {
    Serial.println("Already recording.");
    return false;
  }
  videoFile = SD_MMC.open("/video.avi", FILE_WRITE);
  if (!videoFile) {
    Serial.println("Failed to open video file.");
    return false;
  }

  recording = true;
  Serial.println("Recording started.");
  return true;
}

bool stopRecording() {
  if (!recording) {
    Serial.println("Not currently recording.");
    return false;
  }

  videoFile.close();
  recording = false;
  Serial.println("Recording stopped.");
  return true;
}

bool capturePhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Photo capture failed!");
    return false;
  }

  String path = "/photo_" + String(millis()) + ".jpg";
  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to write photo!");
    esp_camera_fb_return(fb);
    return false;
  }

  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  Serial.printf("Photo saved: %s\n", path.c_str());
  return true;
}

void setResolution(int resolution) {
  stopRecording();
  currentResolution = resolution;
  esp_camera_deinit();
  initCamera();
  startRecording();
}

bool isRecording() {
  return recording;
}
}
