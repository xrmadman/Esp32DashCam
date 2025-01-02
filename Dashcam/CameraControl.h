#ifndef CAMERA_CONTROL_H
#define CAMERA_CONTROL_H

namespace CameraControl {
bool initCamera();
bool startRecording();
bool stopRecording();
bool capturePhoto();
void setResolution(int resolution);
void setFrameRate(int framerate);
}

#endif
