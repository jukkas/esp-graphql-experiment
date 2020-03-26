# Feeble ESP Websocket Client (fwsc)

Implements parts of the Websocket client protocol
(https://tools.ietf.org/html/rfc6455) for ESP8266 and ESP32.

Designed to be light weight and work with parse-esp to implement ParseLiveQuery.
Surprisingly also worked against one GraphQL server.
Might be usable elsewhere, but...

Definitely not compliant with the RFC6455.
Some limitations vs full WS client:
- Can only connect to TLS servers
- Does not verify server TLS certificate
- Cheats in WS handshake: Always uses the same `Sec-WebSocket-Key`, does not check server response
- No Subprotocols
- Supports only text frames (no binary)
- No continuation frames, no masking keys support

So this probably only works with the one kind of server I have tested against, i.e. Parse Server.

TODO: Even basic documentation... Or more comments in code...