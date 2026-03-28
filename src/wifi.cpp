#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <wifi.h>
#include <webconfig.h>
#include <config.h>
#include <statusbar.h>
#include <display.h>

static bool _isAPMode = false;
static uint32_t _reconnect_at = 0;
static String _wifi_ssid;
static String _wifi_pass;

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

static bool loadSavedCredentials() {
    if (!LittleFS.begin()) return false;
    File f = LittleFS.open("/creds.txt", "r");
    if (!f) {
        LittleFS.end();
        return false;
    }
    _wifi_ssid = f.readStringUntil('\n');
    _wifi_pass = f.readStringUntil('\n');
    f.close();
    LittleFS.end();
    _wifi_ssid.trim();
    _wifi_pass.trim();
    return _wifi_ssid.length() > 0;
}

void WIFI::Initialize() {
    WiFi.mode(WIFI_STA);

    gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
    {
        Serial.printf("Station connected, IP: %s\n", WiFi.localIP().toString().c_str());
        WIFI::UpdateStatus();
    });

    disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
    {
        if (_isAPMode) return;
        Serial.println("Station disconnected");
        WIFI::UpdateStatus();
        _reconnect_at = millis() + 5000;
    });

    // Try saved credentials first (one attempt)
    if (loadSavedCredentials()) {
        Serial.printf("Trying saved credentials for '%s'\n", _wifi_ssid.c_str());
        Connect();
        if (WiFi.waitForConnectResult(WIFI_CONNECTTIMEOUT) == WL_CONNECTED)
            return;
        WiFi.disconnect();
        UpdateStatus();
        Serial.println("Saved credentials failed, trying built-in credentials");
    }

    // Try built-in (hardcoded) credentials
    _wifi_ssid = WIFI_SSID;
    _wifi_pass = WIFI_PASSWD;
    for (int attempt = 1; attempt <= WIFI_AP_MAXATTEMPTS; attempt++) {
        Serial.printf("WiFi connection attempt %d/%d\n", attempt, WIFI_AP_MAXATTEMPTS);
        Connect();
        if (WiFi.waitForConnectResult(WIFI_CONNECTTIMEOUT) == WL_CONNECTED)
            return;
        WiFi.disconnect();
        UpdateStatus();
    }

    Serial.println("All connection attempts failed, starting AP mode");
    StartAP();
}

void WIFI::StartAP() {
    _isAPMode = true;
    // WIFI_AP_STA needed so we can still scan for networks from the config page
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWD);
    Serial.printf("AP mode active. SSID: %s, IP: %s\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
    StatusBar::SetWiFiStatus(StatusBar::WIFI_STATUS::WS_AP);
    Display::Refresh();
    WebConfig::Initialize();
}

void WIFI::Handle() {
    if (_isAPMode) {
        WebConfig::Handle();
        return;
    }
    if (_reconnect_at != 0 && millis() >= _reconnect_at) {
        _reconnect_at = 0;
        Connect();
    }
}

bool WIFI::IsAPMode() {
    return _isAPMode;
}

void WIFI::Connect() {
    StatusBar::SetWiFiStatus(StatusBar::WIFI_STATUS::WS_CONNECTING);
    Display::Refresh();
    Serial.printf("Connecting to '%s'\n", _wifi_ssid.c_str());
    WiFi.begin(_wifi_ssid.c_str(), _wifi_pass.c_str());
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
