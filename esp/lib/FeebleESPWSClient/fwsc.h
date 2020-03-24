#pragma once

#include <cstdint>
#include <functional>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>

enum class WSEvent { error, disconnected, connected, text, unsupported };

const unsigned long initialWsReconnectInterval = 60000;

class Fwsc
{
private:
    uint8_t _buffer[2048] = {0}; // TODO: use buffer reserved by caller
    WiFiClientSecure _client;
    std::function<void(WSEvent type, uint8_t * payload)> callback = nullptr;
    void callCallback(WSEvent type, uint8_t * payload);
    int parse_ws_message(const uint8_t * data, int len);
    unsigned long wsLastDisconnect = 0;
    const char * _host;
    const char * _url;
    uint16_t _port;

public:
    void setCallback(std::function<void(WSEvent type, uint8_t * payload)> cb) { callback = cb; };
    int connect(const char * host, uint16_t port = 443, const char * url = "/");
    void disconnect();
    void loop(void);
    bool sendtxt(const char * payload);
    bool isConnected = false;
    bool tryReconnect = true;
    unsigned long wsReconnectInterval = initialWsReconnectInterval;
};
