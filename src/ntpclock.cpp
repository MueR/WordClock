#include <Arduino.h>
#include <ntpclock.h>
#include <config.h>
#include <statusbar.h>
#include <display.h>

static Timezone tz;
static time_t _last_time = 0;
static uint32_t _last_millis = 0;

void NTPClock::Initialize() {
    Serial.printf("Initializing NTP (server: %s, interval: %ds, timezone: %s)\n", NTP_HOST, NTP_SYNCINTERVAL, NTP_TIMEZONE);
    StatusBar::SetClockStatus(StatusBar::CLOCK_STATUS::CS_SYNCING);
    Display::Refresh();
    setServer(NTP_HOST);
    setInterval(NTP_SYNCINTERVAL);
    waitForSync();
    if (!tz.setCache(0))
        tz.setLocation(F(NTP_TIMEZONE));
    Serial.printf("NTP initialized\n");
    StatusBar::SetClockStatus(StatusBar::CLOCK_STATUS::CS_SYNCED);
    Display::Refresh();
}

long NTPClock::Now() {
    events();
    time_t t = tz.now();
    if (t > 0) {
        _last_time = t;
        _last_millis = millis();
        return t;
    }
    if (_last_time > 0)
        return _last_time + (millis() - _last_millis) / 1000;
    return 0;
}