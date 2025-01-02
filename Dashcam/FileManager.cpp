#include <SD_MMC.h>
#include "FileManager.h"

namespace FileManager {
bool initSDCard() {
  if (!SD_MMC.begin()) {
    Serial.println("SD Card initialization failed.");
    return false;
  }
  Serial.println("SD Card initialized.");
  return true;
}

void manageStorage() {
  uint64_t totalBytes = SD_MMC.totalBytes();
  uint64_t usedBytes = SD_MMC.usedBytes();
  uint64_t freeBytes = totalBytes - usedBytes;

  if (freeBytes < (totalBytes * 0.1)) {
    File root = SD_MMC.open("/");
    File oldestFile;
    time_t oldestTime = UINT32_MAX;

    while (File file = root.openNextFile()) {
      if (!file.isDirectory() && file.getLastWrite() < oldestTime) {
        oldestFile = file;
        oldestTime = file.getLastWrite();
      }
    }

    if (oldestFile) {
      SD_MMC.remove(oldestFile.name());
      Serial.printf("Deleted %s to free space.\n", oldestFile.name());
    }
  }
}
}
