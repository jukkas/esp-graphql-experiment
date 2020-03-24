#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

#include "settings.h" // Set your Graphql keys and hostnames here! (+WiFi credentials)

#include "fwsc.h"

bool shouldSetOnline = false;

/* Very limited JSON parsing routines */
bool parseBool(const char* json, const char *key);
char* parseText(const char* json, const char *key);
int parseInt(const char* json, const char *key);




#ifdef LED_BUILTIN
const int OutputPin = LED_BUILTIN; // ESP8266 D1 Mini
#else
const int OutputPin = 22; // ESP32 dev board without build in LED, connect LED+R b/w 3.3v and PIN
#endif
static bool outputState = HIGH; // Light/LED is off when GPIO high 


// Set GPIO high/low, for LED_BUILTIN low means light off
static void turnLightOn(bool isOn) {
    outputState = isOn ? LOW : HIGH;
    digitalWrite(OutputPin, outputState);
}

Fwsc ws;
void sendSetOnline() {
    ws.sendtxt("{\"id\":\"0\",\"type\":\"start\",\"payload\":{\"query\":\"mutation{update_devices(where:{serial:{_eq:\\\"123456\\\"}},_set:{online:true}){affected_rows\n}}\"}}");
}

void graphqlSubscribe() {
    auto cb = [&](WSEvent type, uint8_t * payload) {
        switch (type)
        {
        case WSEvent::error:
            Serial.printf_P(PSTR("Websocket error\n"));
            break;
        case WSEvent::disconnected:
            Serial.printf_P(PSTR("Websocket disconnected\n"));
            break;
        case WSEvent::connected:
            {
                Serial.printf_P(PSTR("Websocket connected\n"));

                ws.sendtxt("{\"type\":\"connection_init\",\"payload\":{}}");
                sendSetOnline();
                ws.sendtxt("{\"id\":\"1\",\"type\":\"start\",\"payload\":{\"query\":\"subscription{devices(where:{serial:{_eq:\\\"123456\\\"}}){serial online light}}\"}}");
            }
            break;
        case WSEvent::text:
            Serial.printf_P(PSTR("< From Websocket: "));
            Serial.println((const char *)payload);
            if (parseText((char *)payload, "serial")) { // "data"-message contains device
                bool light = parseBool((char *)payload, "light");
                Serial.printf("Light %s\n", light ? "On":"Off");
                turnLightOn(light);

                if (parseBool((char *)payload, "online") == false) {
                   shouldSetOnline = true;
                }
            }
            break;
        default:
            Serial.printf_P(PSTR("WS Got unimplemented\n"));
            break;
        }
    };
    ws.setCallback(cb);
    ws.connect(graphqlHost, 443, graphqlPath);
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

    Serial.println("WLAN Connected");
    //Serial.println(ESP.getChipId());  // TODO: Could use this ID as device name in server

  graphqlSubscribe();
}

void loop() {
  ws.loop();
  if (shouldSetOnline) {
    shouldSetOnline = false;
    if (ws.isConnected) {
      sendSetOnline();
    }

  }
}


/************* JSON parsing routines ******************/
static const char* findKey(const char* json, const char *key) {
    int keyLen = strlen(key);
    const char *p = json;
    do {
        p = strstr(p, key);
        if (p && (*(p-1) != '"' || *(p+keyLen) != '"')) { 
            p++;
        } else {
            break;
        }
    } while (p);
    return p;
}

bool parseBool(const char* json, const char *key) {
    int keyLen = strlen(key);
    const char *p = findKey(json, key);

    if (p) {
        return (*(p+keyLen+2) == 't');
    }
    //Serial.println("parseBool: key not found");
    return false;
}

char* parseText(const char* json, const char *key) {
    static char buf[41]={0};
    int keyLen = strlen(key);
    const char *p = findKey(json, key);
    if (!p) return NULL;

    const char *end = strchr(p+keyLen+3, '"');
    if (!end) return NULL;
    strncpy(buf, p+keyLen+3, end-(p+keyLen+3));
    buf[40]='\0';

    return buf;
}

int parseInt(const char* json, const char *key) {
    static char buf[11]={0};
    int keyLen = strlen(key);
    const char *p = findKey(json, key);
    if (!p) return -1;
    char *b = buf;
    p += keyLen + 2;
    while(isdigit(*p)) {
        *b = *p;
        p++;
        b++;
    }
    *b = '\0';
    return atoi(buf);
}
