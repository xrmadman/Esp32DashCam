#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

#include <Arduino.h>

namespace CameraControl {
bool initCamera();
bool startRecording();
bool stopRecording();
bool capturePhoto();
void setResolution(int resolution);
bool isRecording();
}

#endif
