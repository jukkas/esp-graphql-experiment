# ESP (ESP8266/ESP32) controlled by GraphQL -experiment

This is an another IoT experiment of connecting an ESP microcontroller to a BaaS/"Backend as a Service",
namely a GraphQL API. (first was with Parse platform https://github.com/jukkas/parse-esp-example).

GraphQL API server must support websocket transport (TLS only) and support subscriptions. The only server
I have tested against is [Hasura](https://hasura.io/) running in Heroku.

## ESP8266/ESP32 implementation

See directory/folder `ESP`. The project assumes you are using PlatformIO and uses Arduino libraries/runtime.
`src` contains an example main.c. `lib` has GraphqlEsp C++ class that implements raw GraphQL/Websocket connectivity.
GraphqlEsp in turn uses Fwsc (FeebleESPWSClient) websocket implementation.

Assumptions:

- Possibly only works against hasura.io
- Only websocket over TLS
- Assumes ESP subscribes to its own corresponding entry (e.g. device with "serial": "123456")
  - `gq.subscription("devices(where:{serial:{_eq:\\\"123456\\\"}}){serial online light}");`
- ESP can mutate its own entry
  - `gq.mutation("update_devices(where:{serial:{_eq:\\\"123456\\\"}},_set:{online:true}){affected_rows}");`
- 'query' is not yet implemented
- any kind of authentication is not implemented (though you can add an extra header line into
  Websocket HTTP connection request, e.g. "Authorization: Bearer 123456790abcdef")
- Example Postgres/Hasura database schema:
```
                         Table "public.devices"
   Column    |           Type           | Collation | Nullable | Default 
-------------+--------------------------+-----------+----------+---------
 serial      | text                     |           | not null | 
 name        | text                     |           |          | 
 online      | boolean                  |           | not null | false
 light       | boolean                  |           | not null | false
 created_at  | timestamp with time zone |           | not null | now()
 updated_at  | timestamp with time zone |           | not null | now()
 temperature | numeric                  |           |          | 
Indexes:
    "devices_pkey" PRIMARY KEY, btree (serial)
    "devices_name_key" UNIQUE CONSTRAINT, btree (name)
Triggers:
    set_public_devices_updated_at BEFORE UPDATE ON devices FOR EACH ROW EXECUTE FUNCTION set_current_timestamp_updated_at()
```

## Todo
- Try with other GraphQL server implementations
- Add some access control
- Add ESP pushing some data to server, e.g. temperature (should just work with gq.mutation(...))

- - -
A blog post about this: https://mcuer.sikamo.net/blog/esp-and-graphql-experiment