#include <esp_camera.h>
#include <Arduino.h>
#include <SD_MMC.h>
#include "CameraControl.h"

// Pin definitions for the ESP32-CAM module
#define PWDN_GPIO_NUM -1
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
int frameDelay = 33;  // Default: ~30 FPS
TaskHandle_t recordTaskHandle = NULL;

void recordTask(void *param) {
  while (recording) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Frame capture failed!");
      delay(frameDelay);
      continue;
    }

    String path = "/video_frame_" + String(millis()) + ".jpg";
    File file = SD_MMC.open(path.c_str(), FILE_WRITE);
    if (file) {
      file.write(fb->buf, fb->len);
      file.close();
      Serial.printf("Saved frame: %s\n", path.c_str());
    } else {
      Serial.println("Failed to write frame to SD card!");
    }

    esp_camera_fb_return(fb);
    delay(frameDelay);  // Control frame rate
  }
  vTaskDelete(NULL);
}

bool initCamera() {
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
  config.pixel_format = PIXFORMAT_JPEG;  // Use JPEG format

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed.");
    return false;
  }

  Serial.println("Camera initialized successfully.");
  return true;
}

bool startRecording() {
  if (recording) {
    Serial.println("Recording already in progress.");
    return false;
  }
  recording = true;
  xTaskCreatePinnedToCore(recordTask, "Record Task", 4096, NULL, 1, &recordTaskHandle, 1);
  Serial.println("Recording started.");
  return true;
}

bool stopRecording() {
  if (!recording) {
    Serial.println("Not currently recording.");
    return false;
  }
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
    Serial.println("Failed to write photo to SD card!");
    esp_camera_fb_return(fb);
    return false;
  }

  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  Serial.printf("Photo saved to: %s\n", path.c_str());
  return true;
}

void setResolution(int resolution) {
  sensor_t *s = esp_camera_sensor_get();
  if (s != nullptr) {
    s->set_framesize(s, (framesize_t)resolution);
    Serial.printf("Resolution set to %d\n", resolution);
  }
}

void setFrameRate(int framerate) {
  switch (framerate) {
    case 5:
      frameDelay = 200;  // 5 FPS
      break;
    case 10:
      frameDelay = 100;  // 10 FPS
      break;
    case 12:
      frameDelay = 83;  // 12 FPS
      break;
    default:
      frameDelay = 33;  // Default to ~30 FPS
      break;
  }
  Serial.printf("Frame rate set to %d FPS\n", framerate);
}
}
