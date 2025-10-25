#include "OfflineLog.h"
#include "Notifier.h"
#include <LittleFS.h>
#include <time.h>

OfflineLog::OfflineLog(const char* path, size_t rotateKB)
: path_(path), rotateKB_(rotateKB) {}

void OfflineLog::begin(bool formatOnFail) {
  if (fsReady_) return;
  if (!LittleFS.begin(formatOnFail)) {
    Serial.println("[offline_log] LittleFS mount failed");
    fsReady_ = false; return;
  }
  fsReady_ = true;
  Serial.println("[offline_log] LittleFS mounted");
}

void OfflineLog::logPress(uint32_t epochSecs) {
  if (!fsReady_) return;
  File f = LittleFS.open(path_, FILE_APPEND);
  if (!f) { Serial.println("[offline_log] open append failed"); return; }
  f.printf("{\"t\":%lu}\n", (unsigned long)epochSecs);
  f.close();

  File r = LittleFS.open(path_, FILE_READ);
  if (r) {
    size_t sz = r.size(); r.close();
    if (sz > rotateKB_ * 1024UL) {
      LittleFS.remove(String(path_) + ".1");
      LittleFS.rename(path_, String(path_) + ".1");
      Serial.println("[offline_log] rotated");
    }
  }
}

String OfflineLog::buildSummary_() {
  String msg; msg.reserve(1024);
  msg += "Queued presses (outside hours):\n";
  if (!LittleFS.exists(path_)) return msg;

  File f = LittleFS.open(path_, FILE_READ);
  if (!f) return msg;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (!line.length()) break;

    // parse t:
    int ti = line.indexOf("\"t\":");
    time_t t = 0;
    if (ti >= 0) { ti += 4; t = (time_t) strtoul(line.c_str() + ti, nullptr, 10); }

    if (t > 0) {
      struct tm lt{}; localtime_r(&t, &lt);
      char hhmm[6]; snprintf(hhmm, sizeof(hhmm), "%02d:%02d", lt.tm_hour, lt.tm_min);
      msg += " • "; msg += hhmm; msg += "\n";
    } else {
      msg += " • epoch 0\n";
    }
    if (msg.length() > 3000) { msg += " …"; break; }
  }
  f.close();
  return msg;
}

bool OfflineLog::reportAndClear(Notifier& notifier) {
  if (!fsReady_ || !LittleFS.exists(path_)) return true;
  String summary = buildSummary_();
  // If file exists but summary only has header, still try to clear
  bool ok = notifier.sendSummary(summary);
  if (ok) { LittleFS.remove(path_); Serial.println("[offline_log] reported+cleared"); }
  else    { Serial.println("[offline_log] report failed; keeping"); }
  return ok;
}

bool OfflineLog::clear() {
  if (!fsReady_) return false;
  if (LittleFS.exists(path_)) return LittleFS.remove(path_);
  return true;
}
