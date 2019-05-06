#line 2 "Main.cpp"

#include <ArduinoUnit.h>

#include "SwishDB.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <math.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include "AQTime.h"
#include <TimeLib.h>
#include "SwishDBTester.h"

extern "C" {
#include "user_interface.h"
}

AsyncWebServer adminWebServer = AsyncWebServer(80);
DNSServer dns;

void webInit();

SwishDBTester testCreate("testCreate");
DataCreateTest dataCreateTest("dataTestCreate");
IndexFileListTest indexFileListTest("fileListTest");

void setup() {
    Serial.begin(115200);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    webInit();
    initTime();
    pinMode(LED_BUILTIN, OUTPUT);

    Test::min_verbosity |= TEST_VERBOSITY_ASSERTIONS_ALL;
}



void loop() {
    digitalWrite(LED_BUILTIN, LOW);

    ArduinoOTA.handle();

    if ( ! isTimeSynced() ) {
        prettyPrint(now(), Serial);
        Log.warning(F(" - preconditions not met, waiting 1 second and trying again. " CR));
        yield();
        delay(1000);
        return;
    }

    Test::run();

    digitalWrite(LED_BUILTIN, HIGH);

}

void printFSInfo() {
    FSInfo fsInfo;
    SPIFFS.info(fsInfo);
    Log.trace(F("FSInfo: totalBytes: %l usedBytes: %l" CR), fsInfo.totalBytes, fsInfo.usedBytes);

}



void webInit() {
    Log.notice(F("Connecting to WIFI with AyncWifiManager... " CR));

    //Ennsure we have a wifi connection
    if (!WiFi.isConnected()) {
        AsyncWiFiManager wifiManager(&adminWebServer,&dns);

        if (wifiManager.autoConnect("AQThingSetup")) {
            Log.notice(F("Connected to WIFI: "));
            WiFi.printDiag(Serial);
        }

        // Port defaults to 8266
        // ArduinoOTA.setPort(8266);

        // Hostname defaults to esp8266-[ChipID]
        ArduinoOTA.setHostname("aqthingdev");

        // No authentication by default
        // ArduinoOTA.setPassword("admin");

        // Password can be set with it's md5 value as well
        // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
        // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

        ArduinoOTA.onStart([]() {
            Log.trace(F("Start" CR));

        });
        ArduinoOTA.onEnd([]() {
            Log.trace(F(CR "End" CR));
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Log.trace(F("Progress: %d%%" CR), (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Log.trace(F("Error: "), error);
            if (error == OTA_AUTH_ERROR) Log.trace(F("Error: Auth Failed" CR));
            else if (error == OTA_BEGIN_ERROR) Log.trace(F("Error: Begin Failed" CR));
            else if (error == OTA_CONNECT_ERROR) Log.trace(F("Error: Connect Failed" CR));
            else if (error == OTA_RECEIVE_ERROR) Log.trace(F("Error: Receive Failed" CR));
            else if (error == OTA_END_ERROR) Log.trace(F("Error: End Failed" CR));
        });
        ArduinoOTA.begin();
        MDNS.addService("http", "tcp", 80);

    } else {
        Log.error(F("Unable to connect to WIFI. Sleeping 30 seconds and resetting." CR));
        delay(30000);
        ESP.reset();
    }

    adminWebServer.begin();

}




