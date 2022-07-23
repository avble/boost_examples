# Introduction
It includes various examples based on boost.beast and boost.asio.

# http/rest server
## route registration
```c
// route definitions
beast_rest::route rs;
rs.add(http::verb::get, "/hello", std::move(h1));
```

## rest handler 
```c
    auto h1 = [](std::shared_ptr<beast_rest::request> req){
        req.get()->res_.body() = "this is response from handle 01";
        req.get()->res_.prepare_payload();
    };
```

# websocket client
--- command ---
```bash
./ws_client 127.0.0.1 8080 

```
--- console ---
```bash
one
Echo: one
two
Echo: two
three
Echo: three
four
Echo: four
```
