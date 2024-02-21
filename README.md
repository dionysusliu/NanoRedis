# My-Redis

## What is my-redis?
My-redis a learning project for sanity check on basic linux network programming and KV-store theory. This is a minimal deliverable mimicking part of functions of Redis.
It is also worth mentioning that it only support Linux environments with <epoll> library.
features included:
  - Non-blocking IO based on linux epoll
  - Data types: List, Set, Hashmap, Sorted Set
  - Support TTL timestamp


## Run the project 
go to the `build` dir and run cmake
```bash
cd ./build/ && cmake ..
```
in `build/`, build the executable
```bash
make
```
Run the server
```bash
./build/server
```
Run the client
```bash
./build/client GET k
./build/client SET k v
./build/client DEL k
./build/client ZADD k v
...
```
