#include "Net.h"
#include <WiFi.h>
#include <IPAddress.h>

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
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[NET] IP: %s\n", WiFi.localIP().toString().c_str());
    
    // Wait a bit for DNS to be available
    delay(1000);
    
    // Set DNS servers as fallback (Google DNS)
    IPAddress dns1(8, 8, 8, 8);
    IPAddress dns2(8, 8, 4, 4);
    WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), dns1, dns2);
    
    // Wait for DNS to be ready by trying a simple DNS lookup
    Serial.print(F("[NET] Waiting for DNS..."));
    uint32_t dnsStart = millis();
    bool dnsReady = false;
    while ((millis() - dnsStart) < 5000) {
      IPAddress addr;
      if (WiFi.hostByName("api.telegram.org", addr) == 1) {
        dnsReady = true;
        break;
      }
      delay(500);
      Serial.print('.');
    }
    Serial.println();
    if (dnsReady) {
      Serial.println(F("[NET] DNS ready"));
    } else {
      Serial.println(F("[NET] DNS timeout (continuing anyway)"));
    }
  } else {
    Serial.println("[NET] not connected (continuing)");
  }
}

void Net::loop() { /* optional reconnect logic here */ }
bool Net::connected() const { return WiFi.status() == WL_CONNECTED; }
