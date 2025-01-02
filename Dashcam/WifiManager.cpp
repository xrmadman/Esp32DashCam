#include "WiFiManager.h"
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>  // For mDNS functionality

#define WIFI_SSID "Dashcam"          // Default SSID for AP mode
#define WIFI_PASSWORD "Michelle236"  // Default password for AP mode
#define HOSTNAME "dashcam"           // Hostname for mDNS

Preferences preferences;

namespace WiFiManager {
bool initWiFi() {
  preferences.begin("wifi", false);

  // Attempt to retrieve stored credentials
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password", "");

  if (ssid.length() > 0) {
    // Connect to stored Wi-Fi credentials
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.printf("Connecting to Wi-Fi: %s\n", ssid.c_str());

    // Wait for connection with a timeout
    int retry_count = 0;
    const int max_retries = 20;  // ~20 seconds timeout
    while (WiFi.status() != WL_CONNECTED && retry_count < max_retries) {
      delay(1000);
      Serial.print(".");
      retry_count++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi.");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      // Set up mDNS responder
      if (!MDNS.begin(HOSTNAME)) {
        Serial.println("Error setting up mDNS responder!");
      } else {
        Serial.printf("mDNS responder started at http://%s.local\n", HOSTNAME);
      }
      return true;
    } else {
      Serial.println("\nFailed to connect to Wi-Fi.");
    }
  }

  // If connection fails, start in AP mode
  Serial.println("Starting AP mode...");
  WiFi.mode(WIFI_AP);

  IPAddress local_IP(192, 168, 4, 1);  // Default static IP for AP mode
  IPAddress gateway(192, 168, 4, 1);   // Gateway (same as static IP for AP mode)
  IPAddress subnet(255, 255, 255, 0);  // Subnet mask
  WiFi.softAPConfig(local_IP, gateway, subnet);

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("AP mode started. Connect to '%s' with password '%s'\n", WIFI_SSID, WIFI_PASSWORD);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up mDNS responder for AP mode
  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error setting up mDNS responder in AP mode!");
  } else {
    Serial.printf("mDNS responder started in AP mode at http://%s.local\n", HOSTNAME);
  }

  return false;
}

void saveCredentials(const String& ssid, const String& password) {
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  Serial.printf("Wi-Fi credentials saved: SSID='%s'\n", ssid.c_str());
}
}
