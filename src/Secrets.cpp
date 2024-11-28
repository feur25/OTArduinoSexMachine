#include <Arduino.h>

#include "Secrets.h"

const char* ssid = "WiFi@YNOV";
const char* api_url[] = { "https://alder-shore-grain.glitch.me/api/receive", "https://alder-shore-grain.glitch.me/api/log" };

String payload;
String logs;
String FirmwareVer = "1.9";