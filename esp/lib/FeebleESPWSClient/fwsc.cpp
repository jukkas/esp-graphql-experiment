#include <Arduino.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include "fwsc.h"

/* Decodes WS message. Returns amount of parsed bytes */
int Fwsc::parse_ws_message(const uint8_t *data, int len) {

    static int incompleteRemaining = 0; // If WS text frame comes in multiple read():s, how many bytes still expected
    /* If last Client.read did not return full WS text frame, lets assume this next one is rest of it.
        TODO/FIXME: is that assumption always true? */
    if (incompleteRemaining>0) {
        int lastMsgLen = strnlen((char *)_buffer, FWSCBUFLEN-len-1);
        //Serial.printf_P(PSTR("@incompleteRemaining=%d, len=%d lastMsgLen=%d\n"), incompleteRemaining, len, lastMsgLen);
        Serial.printf_P(PSTR(" f:%d/%d\n"), lastMsgLen, len);
        memcpy(_buffer+lastMsgLen, data, len);
        _buffer[lastMsgLen+len] = 0;
        incompleteRemaining -= len;
        if (incompleteRemaining < 1) {
            callCallback(WSEvent::text, _buffer);
        }
        return len;
    }

    /* "Normal" handling of beginning of WS message */

    uint8_t opcode = data[0] & 0b00001111;

    _buffer[0] = 0;

    /* Most frame types can be handled before decoding rest */
    if (opcode == 0x08) { // connection close
        _client.stop();
        isConnected = false;
        callCallback(WSEvent::disconnected, _buffer);
        Serial.printf_P(PSTR("fwsc: connection close\n"));
        return len;
    }

    if (opcode != 0x01 && opcode != 0x09) { // Not supported / do not care
        Serial.printf_P(PSTR("fwsc: not supported / do not care (opcode %02x)\n"), opcode);
        return len;
    }

    int payload_length = (data[1] & 0b01111111);
    uint8_t const *p = data + 2;

    if (payload_length > 126) {
        Serial.printf_P(PSTR("fwsc: Error: unsupported message length\n"));
        return len;
    }

    if (payload_length > 125) {
        uint16_t ext_len = ((uint16_t)p[0] << 8) | p[1];
        payload_length = ext_len;
        p += 2;  
    }

    int msgLen = p-data+payload_length; // Actual amount of bytes handled: headers+payload
    if (msgLen > len) { // WS text longer than what we received
        incompleteRemaining = msgLen-len;
        int charcount = len-(p-data);
        Serial.printf_P(PSTR("F:%d/%d "), charcount, msgLen-len);
        memcpy(_buffer, p, charcount);
        _buffer[charcount] = 0;
        return len;
    }

    if (opcode == 0x9) { // ping (FIXME: this is not robust. Assumes len<126)
        memcpy(_buffer+4, data, msgLen);
        _buffer[0] = (data[0] & 0b11110000) | 0x0a; // Make opcode "pong"
        _buffer[1] = data[1] | 0x80; // len+MASK
        memset(_buffer+2, 0, 4);
        _client.write(_buffer, msgLen+4);
        Serial.printf(".");
        return msgLen;
    }

    // Text frame: copy payload text to _buffer
    memcpy(_buffer, p, payload_length);
    _buffer[payload_length] = 0;
    callCallback(WSEvent::text, _buffer);
    return msgLen;
}

// Encode text frame
static size_t encode_ws_message(uint8_t *buf, const char *text) {
    unsigned int len = strlen(text);
    buf[0] = 0b10000001; // Final fragment, opcode:'text frame'
    uint8_t *p = buf+2;
    size_t ret_len = len + 2;
    if (len < 126) {
        buf[1] = 0x80 | len; // MASK bit and text length;
    } else {
        buf[1] = 0x80 | 126;
        buf[2] = len >> 8;
        buf[3] = len & 0xff;
        p += 2;
        ret_len += 2;
    }
    unsigned int i;

    // Add Masking-key (insecurely just zeros...)
    memset(p, 0, 4);
    p += 4;
    ret_len += 4;

    // Add text data
    for (i = 0; i < len; i++) {
        p[i] = text[i];
    }

    return ret_len;
}

void Fwsc::callCallback(WSEvent type, uint8_t * payload) {
    if (callback) {
        callback(type, payload);
    }
}

// Ćreate WS connection to TLS server/URL implementing WS service
int Fwsc::connect(const char * host, uint16_t port, const char * url) {

    //Serial.printf_P(PSTR("fwsc:TLS connecting to %s:%d%s ...\n"), host, port, url);

    // Save for reconnect
    _host = host;
    _port = port;
    _url = url;
    wsLastDisconnect = millis();
    tryReconnect = true;
    wsReconnectInterval = initialWsReconnectInterval;

#ifdef wificlientbearssl_h
    _client.setInsecure();
#endif
    _client.setNoDelay(true);
    if (!_client.connect(host, port)) {
        Serial.printf_P(PSTR("fwsc:TLS connection to %s:%d failed:"), host, port);
        tryReconnect = false;
        return -1;
    }
    Serial.printf_P(PSTR("WS:TLS connected to server!\n"));

    int valread; 
    char buf[1024];
    sprintf_P(buf, PSTR("GET %s HTTP/1.1\r\n"
                        "Host: %s\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                        "Sec-WebSocket-Version: 13\r\n"),
            url, host);
    if (_extraHeader != nullptr) {
        strcat(buf, _extraHeader);
        strcat_P(buf, PSTR("\r\n"));
    }
    strcat_P(buf, PSTR("\r\n"));

    //Serial.println(buf);

    _client.write((uint8_t*)buf , strlen(buf));
    _client.flush();

    valread = _client.readBytes((uint8_t*)buf, sizeof(buf));
    buf[valread > 0 ? valread:0] = 0; 
    //Serial.printf("DEBUG::fwsc: Got: %d bytes ------\n%s\n------\n", valread, buf);

    isConnected = true;
    callCallback(WSEvent::connected, _buffer);
    return 1;
}

// Call this in main loop. Handles incoming WS frames. Handles reconnects
void Fwsc::loop(void) {

    if (!_client.connected()) {
        if (isConnected) {
            isConnected = false;
            _client.stop();
            callCallback(WSEvent::disconnected, _buffer);
        }

        if (tryReconnect && (millis() - wsLastDisconnect) > wsReconnectInterval) {
            wsLastDisconnect = millis();
            wsReconnectInterval += initialWsReconnectInterval;
            connect(_host, _port, _url); 
        }

        return;
    }
    while (_client.available()) {
        uint8_t buffer[2048];
        uint8_t *p = buffer;
        int bytesread = _client.read(buffer, sizeof(buffer));
        // Serial.printf_P(PSTR("fwsc: *** Read bytes: %d\n"), bytesread);
        while (bytesread > 0) {
            int parsedBytes = parse_ws_message(p, bytesread); 
            p += parsedBytes;
            bytesread -= parsedBytes;
        }
    }
}

// Send text to server
bool Fwsc::sendtxt(const char * payload) {
    // FIXME: check size
    size_t len = encode_ws_message(_buffer, payload);
    _client.write(_buffer, len);
    return true;
}

// Disconnect WS connection
void Fwsc::disconnect() {
    //Serial.printf_P(PSTR("DEBUG::fwsc: Sending WS disconnect message\n"));
    uint8_t buf[2];
    buf[0] = 0b10001000;
    buf[1] = 0;
    _client.write(buf, 2);
    _client.stop();
    tryReconnect = false;
    callCallback(WSEvent::disconnected, _buffer);
}