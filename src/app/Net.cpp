#include "Net.h"
#include <WiFi.h>

Net::Net(const char* ssid, const char* pass, const char* hostname)
: ssid_(ssid), pass_(pass), hostname_(hostname) {}

void Net::begin() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname_);
  Serial.printf("[NET] Connecting to %s...\n", ssid_);
  WiFi.begin(ssid_, pass_);
}

void Net::waitReady(uint32_t ms) {
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < ms) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("[NET] IP: %s\n", WiFi.localIP().toString().c_str());
  else
    Serial.println("[NET] not connected (continuing)");
}

void Net::loop() { /* optional reconnect logic here */ }
bool Net::connected() const { return WiFi.status() == WL_CONNECTED; }
