#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <ESPAsyncHTTPUpdateServer.h>
#include <esp_wifi.h>

#include "Secrets.h"
#include "DataSender.cpp"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#if __has_include("esp_eap_client.h")
#include "esp_eap_client.h"
#else
#include "esp_wpa2.h"
#endif

String FirmwareVer = { "1.2" };

class WiFiManager {
private:
    const char* ssid;
    const char* eapIdentity;
    const char* eapPassword;
    int connectionTimeout;
    int attempts;

public:
    WiFiManager(const char* ssid, const char* eapIdentity, const char* eapPassword, int timeout = 60)
        : ssid(ssid), eapIdentity(eapIdentity), eapPassword(eapPassword), connectionTimeout(timeout) {}

    void connect() {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);

#if __has_include("esp_eap_client.h")
        esp_eap_client_set_identity((uint8_t*)eapIdentity, strlen(eapIdentity));
        esp_eap_client_set_username((uint8_t*)eapIdentity, strlen(eapIdentity));
        esp_eap_client_set_password((uint8_t*)eapPassword, strlen(eapPassword));
        esp_wifi_sta_enterprise_enable();
#else
        esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)eapIdentity, strlen(eapIdentity));
        esp_wifi_sta_wpa2_ent_set_username((uint8_t*)eapIdentity, strlen(eapIdentity));
        esp_wifi_sta_wpa2_ent_set_password((uint8_t*)eapPassword, strlen(eapPassword));
        esp_wifi_sta_wpa2_ent_enable();
#endif
        WiFi.begin(ssid);

        attempts = 0;

        while (WiFi.status() != WL_CONNECTED && attempts++ < connectionTimeout) delay(1000), Serial.print(".");

        if (WiFi.status() == WL_CONNECTED) Serial.println("\nWiFi Connected! \n IP Address: " + WiFi.localIP().toString());
        else Serial.println("\nConnection Failed!");
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }

    void reconnectIfNeeded() {
        if (!isConnected()) WiFi.begin(ssid);
    }
};

class OTAHandler {
private:
    static OTAHandler otaHandler;
public:
    static void setupOTA() { ArduinoOTA.begin(); }

    static void handleOTA() { ArduinoOTA.handle(); }

    static void firmwareUpdate() {
        WiFiClientSecure client;
        client.setInsecure();
        httpUpdate.setLedPin(LED_BUILTIN, LOW);

        Serial.println("Starting firmware update...");

        HTTPUpdateResult ret = httpUpdate.update(client, URL_fw_Bin);

        if (ret == HTTP_UPDATE_FAILED) {
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        } 
        else if (ret == HTTP_UPDATE_NO_UPDATES) Serial.println("HTTP_UPDATE_NO_UPDATES: No update available");
        else if (ret == HTTP_UPDATE_OK) Serial.println("HTTP_UPDATE_OK: Firmware updated successfully");
    }

    int httpsEnd(int httpCode, HTTPClient& https, String message) {
        Serial.println(message);
        https.end();
        return httpCode;
    }

    static int firmwareVersionCheck() {
        String payload;
        int httpCode;
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient https;

        if (https.begin(client, URL_fw_Version)) {
            httpCode = https.GET();

            if (httpCode != HTTP_CODE_OK) {
                https.end();
                return 0;
            }

            payload = https.getString();
            payload.trim();

            if (payload.equals(FirmwareVer)) otaHandler.httpsEnd(httpCode, https, "Firmware is Already Updated");
            else otaHandler.httpsEnd(httpCode, https, "New Firmware Available");
        } 
        else Serial.println("Unable to connect to update server");

        return 0;
    }
};

class ESP32App {
private:
    WiFiManager wifiManager;
    ESPAsyncHTTPUpdateServer httpUpdate;
    DataSender dataSender;
    unsigned long previousMillis;
    unsigned long currentMillis;
    const long interval;

public:
    ESP32App(const char* ssid, const char* eapIdentity, const char* eapPassword, const String& apiUrl)
        : wifiManager(ssid, eapIdentity, eapPassword), dataSender(apiUrl), httpUpdate(), previousMillis(0), interval(60000) {}

    void setup() {
        Serial.begin(115200);

        wifiManager.connect();

        OTAHandler::setupOTA();
        TimeManager::configure();
    }

    void loop() {
        wifiManager.reconnectIfNeeded();
        OTAHandler::handleOTA();

        currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            dataSender.send();

            if (OTAHandler::firmwareVersionCheck()) OTAHandler::firmwareUpdate();
        }
    }
};

ESP32App app(ssid, EAP_IDENTITY, EAP_PASSWORD, api_url);

void setup() { app.setup(); }
void loop() { app.loop(); }
