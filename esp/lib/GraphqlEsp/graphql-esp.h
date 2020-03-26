#pragma once

#include "fwsc.h"

enum class GQEvent { error, disconnected, connected, data };

class GraphqlEsp {
  public:
    void setExtraHeader(const char *headerLine) { _ws.setExtraHeader(headerLine); };
    void setCallback(std::function<void(GQEvent type, char* payload)> cb) { _callback = cb; };
    void connect(const char * host, uint16_t port = 443, const char * url = "/graphql");
    void mutation(const char *data);
    // TODO: void query(const char* data);
    void subscription(const char *data);
    void disconnect();
    void loop(void);
    bool isConnected = false;

  private:
    std::function<void(GQEvent type, char* payload)> _callback = nullptr;
    Fwsc _ws;
    void callCallback(GQEvent type, char* payload);
};

/* Very limited JSON parsing routines */
extern bool parseBool(const char* json, const char *key);
extern char* parseText(const char* json, const char *key);
extern int parseInt(const char* json, const char *key);

