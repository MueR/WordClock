#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <wifi.h>
#include <config.h>
#include <statusbar.h>
#include <display.h>

const char* wifi_ssid = WIFI_SSID;
const char* wifi_pass = WIFI_PASSWD;
static uint32_t _reconnect_at = 0;

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

void WIFI::Initialize() {
    WiFi.mode(WIFI_STA);

    gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
    {
        Serial.printf("Station connected, IP: %s\n", WiFi.localIP().toString().c_str());
        WIFI::UpdateStatus();
    });

    disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
    {
        Serial.println("Station disconnected");
        WIFI::UpdateStatus();
        _reconnect_at = millis() + 5000;
    });

    Connect();

    if (WiFi.waitForConnectResult(WIFI_CONNECTTIMEOUT) != WL_CONNECTED) {
        WIFI::UpdateStatus();
        Serial.println("Unable to connect");
        delay(5000);
        ESP.restart();
    }
}

void WIFI::Handle() {
    if (_reconnect_at != 0 && millis() >= _reconnect_at) {
        _reconnect_at = 0;
        Connect();
    }
}

void WIFI::Connect() {
    StatusBar::SetWiFiStatus(StatusBar::WIFI_STATUS::WS_CONNECTING);
    Display::Refresh();
    Serial.printf("Connecting to %s\n", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);
}

void WIFI::UpdateStatus() {
    StatusBar::SetWiFiStatus(MapWiFiStatus(WiFi.status()));
    Display::Refresh();
}

StatusBar::WIFI_STATUS WIFI::MapWiFiStatus(wl_status_t status) {
    switch (status) {
        case WL_CONNECTED: return StatusBar::WIFI_STATUS::WS_CONNECTED;
        default:           return StatusBar::WIFI_STATUS::WS_DISCONNECTED;
    }
}
