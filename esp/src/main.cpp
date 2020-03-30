#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "settings.h" // Set your GraphQL keys and hostnames here! (+WiFi credentials)

#include "graphql-esp.h"
GraphqlEsp gq; // GraphQL Websocket handler

bool shouldSetOnline = false;  // Global to notify loop() to mutate online=true
const char *mySerial = "123456"; // Id of this device (better would be to read from ESP.getChipId())

#ifdef LED_BUILTIN
const int OutputPin = LED_BUILTIN; // ESP8266 D1 Mini
#else
const int OutputPin = 22; // ESP32 dev board without build in LED, connect LED+R b/w 3.3v and PIN
#endif
static bool outputState = HIGH; // Light/LED in D1 Mini is off when GPIO high 


// Set GPIO high/low, for LED_BUILTIN low means light off
static void turnLightOn(bool isOn) {
    outputState = isOn ? LOW : HIGH;
    digitalWrite(OutputPin, outputState);
}

// Mutate my entry (serial:"123456") in 'devices' to set online to true
void sendSetOnline() {
    char buf[128];
    sprintf_P(buf, PSTR("update_devices(where:{serial:{_eq:\\\"%s\\\"}},_set:{online:true}){affected_rows}"), mySerial);
    gq.mutation(buf);
}

void graphqlConnect() {
    auto cb = [&](GQEvent type, char* payload) {
        switch (type)
        {
        case GQEvent::error:
            Serial.printf_P(PSTR("GQ: error:%s\n"), payload);
            break;
        case GQEvent::disconnected:
            Serial.printf_P(PSTR("GQ disconnected\n"));
            break;
        case GQEvent::connected:
        {
            char buf[128];

            Serial.printf_P(PSTR("GQ connected\n"));
            //sendSetOnline(); // Let subscription data and shouldSetOnline-global handle this later

            // Just for demo's sake:  Lets query something (that we do not confuse for our own data)
            gq.query("devices(where:{serial:{_eq:\\\"234567\\\"}}){name updated_at}");

            // Subscribe to changes in my entry (serial:"123456"). Get fields "serial", "online" and "light"
            sprintf_P(buf, PSTR("devices(where:{serial:{_eq:\\\"%s\\\"}}){serial online light}"), mySerial);
            gq.subscription(buf);
        }   break;
        case GQEvent::data:
            Serial.printf_P(PSTR("< From GQ Websocket: %s\n"), payload);
            if (parseText(payload, "serial")) { // "data"-message contains device
                bool light = parseBool((char *)payload, "light");
                Serial.printf_P(PSTR("Light %s\n"), light ? "On":"Off");
                turnLightOn(light);

                if (parseBool((char *)payload, "online") == false) {
                    // Someone (UI) has changed my online status...
                    shouldSetOnline = true;
                }
            }
            break;
        default:
            Serial.printf_P(PSTR("GQEvent Got unimplemented event type\n"));
            break;
        }
    };
    gq.setCallback(cb);
    gq.setExtraHeader("Hello: World"); // Use this for e.g. Authorization
    gq.connect(graphqlHost, 443, graphqlPath);
}

void setup() {
    Serial.begin(115200);
    pinMode(OutputPin, OUTPUT);
    digitalWrite(OutputPin, outputState);

    // Connect to a WiFi network (ssid and pass from settings.h)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    Serial.printf_P(PSTR("WLAN Connected\n"));
#ifdef ESP8266
    Serial.printf_P(PSTR("Chip id: %06x\n"), ESP.getChipId());  // TODO: Could use this as serial/ID
#endif
#ifdef ESP32
    uint32_t id = 0;
    for(int i=0; i<17; i=i+8) {
        id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    Serial.printf("Chip id: %08X\n", id);
#endif
    delay(1000);

    Serial.printf_P(PSTR("Connecting to GraphQL server\n"));
    graphqlConnect();
}

void loop() {
  gq.loop(); // GraphQL websocket polling
  if (shouldSetOnline) {
    shouldSetOnline = false;
    Serial.printf_P(PSTR("Setting online to true\n"));
    sendSetOnline();
  }
}
