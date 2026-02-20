#include "Net.h"
#include <WiFi.h>

Net::Net(const char* ssid, const char* pass, const char* hostname)
: ssid_(ssid), pass_(pass), hostname_(hostname) {}

void Net::begin() {
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);           // keep radio awake for lower latency
  WiFi.setAutoReconnect(true);
  WiFi.setHostname(hostname_);
  Serial.printf("[NET] Connecting to %s...\n", ssid_);
  WiFi.begin(ssid_, pass_);
  lastReconnectAttemptMs_ = millis();
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

void Net::loop() {
  if (WiFi.status() == WL_CONNECTED) return;

  const uint32_t now = millis();
  if ((now - lastReconnectAttemptMs_) < reconnectIntervalMs_) return;

  lastReconnectAttemptMs_ = now;
  Serial.println(F("[NET] reconnecting..."));
  WiFi.begin(ssid_, pass_);
}

bool Net::connected() const { return WiFi.status() == WL_CONNECTED; }
