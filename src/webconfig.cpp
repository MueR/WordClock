#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <webconfig.h>

static ESP8266WebServer server(80);
static DNSServer dnsServer;

static String getContentType(const String& path) {
    if (path.endsWith(".html")) return F("text/html");
    if (path.endsWith(".css"))  return F("text/css");
    if (path.endsWith(".js"))   return F("application/javascript");
    if (path.endsWith(".json")) return F("application/json");
    return F("application/octet-stream");
}

static bool serveFromLittleFS(const String& path) {
    if (!LittleFS.begin()) return false;
    if (!LittleFS.exists(path)) {
        LittleFS.end();
        return false;
    }
    File f = LittleFS.open(path, "r");
    if (!f) {
        LittleFS.end();
        return false;
    }
    server.streamFile(f, getContentType(path));
    f.close();
    LittleFS.end();
    return true;
}

void WebConfig::Initialize() {
    dnsServer.start(53, "*", WiFi.softAPIP());

    server.on("/api/networks", HTTP_GET, []() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; i++) {
            if (i > 0) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\\", "\\\\");
            ssid.replace("\"", "\\\"");
            json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + WiFi.RSSI(i)
                 + ",\"encrypted\":" + (WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false") + "}";
        }
        json += "]";
        server.send(200, F("application/json"), json);
    });

    server.on("/connect", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        if (ssid.length() == 0) {
            server.send(400, F("application/json"), F("{\"ok\":false,\"error\":\"SSID required\"}"));
            return;
        }

        LittleFS.begin();
        File f = LittleFS.open("/creds.txt", "w");
        if (f) {
            f.println(ssid);
            f.println(pass);
            f.close();
        }
        LittleFS.end();

        String ssidJson = ssid;
        ssidJson.replace("\\", "\\\\");
        ssidJson.replace("\"", "\\\"");
        server.send(200, F("application/json"), "{\"ok\":true,\"ssid\":\"" + ssidJson + "\"}");
        delay(500);
        ESP.restart();
    });

    server.on("/reset", HTTP_POST, []() {
        LittleFS.begin();
        LittleFS.remove("/creds.txt");
        LittleFS.end();
        server.send(200, F("application/json"), F("{\"ok\":true}"));
        delay(500);
        ESP.restart();
    });

    server.onNotFound([]() {
        String path = server.uri();

        // Block access to internal files
        if (path == "/creds.txt") {
            server.send(403, F("text/plain"), F("Forbidden"));
            return;
        }

        // Map / to index.html
        if (path == "/") path = "/index.html";

        // Web interface files live under /web/ on LittleFS
        if (serveFromLittleFS("/web" + path)) return;

        // Captive portal fallback: redirect everything else to the config page
        IPAddress apIP = WiFi.softAPIP();
        String portalUrl = String("http://") + apIP.toString() + "/";
        server.sendHeader("Location", portalUrl);
        server.send(302);
    });

    server.begin();
    IPAddress apIP = WiFi.softAPIP();
    String portalUrl = String("http://") + apIP.toString() + "/";
    Serial.println("Web config server started at " + portalUrl);
}

void WebConfig::Handle() {
    dnsServer.processNextRequest();
    server.handleClient();
}
