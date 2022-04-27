# Event Driven Chat
This is an event-driven chat server made in C. Its purpose is to serve as the server that received messages, handle their queue, and relays them between clients, using an event-driven architecture and a single thread with the `select` function. This allows us to save the overhead of context switching and thread synchronization.

## Usage
Compile with:
```
gcc chat_server.c -o chat_server
```

Run with:
```
./chat_server <port>
```
