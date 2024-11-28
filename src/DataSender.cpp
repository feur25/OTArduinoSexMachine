#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <thread>

#include "Secrets.h"

using namespace std;

namespace Services {

    class DataSender {
    private:
        struct tm timeinfo;
        String apiUrl;

        String getTimestamp() {
            if (!getLocalTime(&timeinfo)) return "0000-00-00T00:00:00Z";

            char buffer[30];
            strftime(buffer, sizeof(buffer), "%FT%T%Z", &timeinfo);

            return String(buffer);
        }

        String generateJsonData() {
            int randomValue = random(1, 21);
            String type = random(0, 2) == 0 ? "entree" : "sortie";

            String jsonData = "{\"timestamp\": \"" + getTimestamp() + "\", ";
            jsonData += "\"value\": " + String(randomValue) + ", ";
            jsonData += "\"type\": \"" + type + "\"}";

            return jsonData;
        }

    public:
        DataSender() : apiUrl("") {}
        DataSender(const String& apiUrl) : apiUrl(apiUrl) {}

        void send(int option) {
            thread([=]() {
                if (!(WiFi.status() == WL_CONNECTED)) return;

                String jsonData = option == 0 ? generateJsonData() : logs;

                HTTPClient http;

                http.begin(apiUrl);
                http.addHeader("Content-Type", "application/json");

                int httpResponseCode = http.POST(jsonData);

                if (httpResponseCode > 0) Serial.println("Data sent successfully");
                else Serial.println("HTTP Request failed. Error code: \n" + String(httpResponseCode));

                http.end();
            }).detach();
        }
    };

    class TimeManager {
    public:
        static void configure() { configTime(0, 0, "pool.ntp.org", "time.nist.gov"); }
    };
}