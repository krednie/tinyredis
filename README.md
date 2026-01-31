# TinyRedis

A lightweight, educational Redis implementation in C++ that demonstrates core concepts of in-memory data structures and network protocols.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Usage](#usage)
  - [Server Mode](#server-mode)
  - [CLI Mode](#cli-mode)
- [Supported Commands](#supported-commands)
  - [String Operations](#string-operations)
  - [Key Management](#key-management)
  - [List Operations](#list-operations)
  - [Set Operations](#set-operations)
  - [Hash Operations](#hash-operations)
- [Data Persistence](#data-persistence)
- [RESP Protocol](#resp-protocol)
- [Project Structure](#project-structure)
- [Implementation Details](#implementation-details)
- [Limitations](#limitations)
- [Contributing](#contributing)
- [License](#license)
- [Acknowledgments](#acknowledgments)

## Overview

TinyRedis is a minimalist implementation of Redis, the popular in-memory data structure store. Built from scratch in C++17, this project serves as an educational tool for understanding:

- In-memory key-value storage systems
- Network protocol implementation (RESP)
- Data structure design and implementation
- Concurrent client handling
- Data persistence mechanisms


## Features

### ✨ Core Capabilities

- **5 Data Types**: Strings, Integers, Lists (deque-based), Sets (hash-based), and Hashes with 50+ commands
- **RESP Protocol**: Full Redis Serialization Protocol implementation compatible with `redis-cli` and standard clients
- **TCP Network Server**: Multi-threaded server with configurable ports and concurrent client connections
- **Plain Text Fallback**: Human-readable command support for easy testing with telnet/netcat
- **Interactive CLI**: Standalone REPL interface for local development without network overhead
- **Dual Persistence**: RDB snapshots (JSON format) + AOF write-ahead logging for crash recovery
- **Auto-Save**: Configurable interval-based snapshots with manual `SAVE` command support
- **TTL & Expiration**: Per-key time-to-live with `EXPIRE`, `TTL`, and `PERSIST` commands
- **Lazy Deletion**: Automatic cleanup of expired keys on access for memory efficiency
- **Type Safety**: Modern C++17 with `std::variant` for zero-overhead polymorphic storage
- **Batch Operations**: Multi-get/set (`MGET`/`MSET`) and bulk list/set operations
- **Atomic Counters**: Thread-safe increment/decrement operations (`INCR`, `DECR`, `INCRBY`, `DECRBY`)
- **Range Operations**: Substring extraction (`GETRANGE`), list ranges (`LRANGE`), and partial updates (`SETRANGE`)
- **Type Introspection**: Runtime type checking with `TYPE` command and automatic error handling
- **Hash Fields**: Full nested key-value support with `HSET`, `HGET`, `HGETALL`, `HKEYS`, `HVALS`
- **O(1) Performance**: Hash table lookups, list push/pop at both ends, set membership tests
- **Error Handling**: Comprehensive RESP error messages with type mismatch detection
- **Graceful Shutdown**: Signal handling (SIGINT/SIGTERM) with clean resource cleanup
## Implementation Details

### Memory Management

- **Storage**: `std::unordered_map<std::string, Value>` for O(1) average-case lookups
- **Type Safety**: `std::variant` for type-safe polymorphic value storage
- **Expiration**: `std::optional<std::chrono::time_point>` for optional TTL

### Concurrency Model

- **Multi-threaded**: Each client connection spawns a new thread
- **Thread Safety**: Each thread operates on shared database (consider adding mutex for production)
- **Signal Handling**: Graceful shutdown on SIGINT/SIGTERM

### Error Handling

- Type mismatches return appropriate RESP errors
- Invalid commands return `-ERR unknown command`
- Malformed data handled with parsing fallbacks

## Architecture

TinyRedis follows a modular architecture:

```
┌─────────────────────────────────────────────┐
│           Client Applications               │
│  (redis-cli, telnet, custom clients)       │
└────────────────┬────────────────────────────┘
                 │ TCP/IP
                 │ RESP Protocol
┌────────────────▼────────────────────────────┐
│          Server Layer (server.cpp)          │
│  • Connection handling                      │
│  • Command parsing                          │
│  • Multi-threading                          │
└────────────────┬────────────────────────────┘
                 │
┌────────────────▼────────────────────────────┐
│       RESP Parser (resp.cpp)                │
│  • Protocol encoding/decoding               │
│  • Plain text fallback                      │
└────────────────┬────────────────────────────┘
                 │
┌────────────────▼────────────────────────────┐
│       Database Layer (db.cpp)               │
│  • Command execution                        │
│  • Data operations                          │
│  • Persistence management                   │
└────────────────┬────────────────────────────┘
                 │
┌────────────────▼────────────────────────────┐
│       Value Store (value.cpp)               │
│  • Type definitions                         │
│  • Expiration handling                      │
│  • Data accessors                           │
└─────────────────────────────────────────────┘
```

## Prerequisites

- **C++ Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **CMake** (optional): Version 3.10+
- **Operating System**: Linux, macOS, or WSL on Windows
- **Libraries**: POSIX threads (pthread)

## Installation

### Clone the Repository

```bash
git clone https://github.com/krednie/tinyredis.git
cd tinyredis
```

### Compile the Server

```bash
g++ -std=c++17 -o redis_server main_server.cpp server.cpp db.cpp value.cpp resp.cpp -lpthread
```

### Compile the CLI

```bash
g++ -std=c++17 -o redis_cli main.cpp db.cpp value.cpp
```

## Usage

### Server Mode

Start the TinyRedis server on the default port (6379):

```bash
./redis_server
```

Or specify a custom port:

```bash
./redis_server 6380
```

**Expected Output:**
```
# Starting TinyRedis Server...
# Server listening on port 6379
# Press Ctrl+C to stop the server
```

### Connecting to the Server

Use any Redis-compatible client or tools:

**Using telnet:**
```bash
telnet localhost 6379
```

**Using netcat:**
```bash
nc localhost 6379
```

### CLI Mode

For standalone testing without network:

```bash
./redis_cli
```

**Interactive Session:**
```
> SET greeting "Hello, TinyRedis!"
OK
> GET greeting
"Hello, TinyRedis!"
> INCR counter
1
> INCR counter
2
> QUIT
```

## Supported Commands

### String Operations

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `SET` | `SET key value` | Set string value | `SET name "Alice"` |
| `GET` | `GET key` | Get string value | `GET name` |
| `APPEND` | `APPEND key value` | Append to string | `APPEND name " Smith"` |
| `STRLEN` | `STRLEN key` | Get string length | `STRLEN name` |
| `GETRANGE` | `GETRANGE key start end` | Get substring | `GETRANGE name 0 4` |
| `SETRANGE` | `SETRANGE key offset value` | Overwrite part of string | `SETRANGE name 0 "Bob"` |
| `MGET` | `MGET key1 key2 ...` | Get multiple values | `MGET name age city` |
| `MSET` | `MSET key1 val1 key2 val2 ...` | Set multiple values | `MSET name "Alice" age "30"` |

### Integer Operations

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `INCR` | `INCR key` | Increment by 1 | `INCR counter` |
| `INCRBY` | `INCRBY key amount` | Increment by amount | `INCRBY counter 5` |
| `DECR` | `DECR key` | Decrement by 1 | `DECR counter` |
| `DECRBY` | `DECRBY key amount` | Decrement by amount | `DECRBY counter 3` |

### Key Management

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `DEL` | `DEL key` | Delete a key | `DEL name` |
| `EXISTS` | `EXISTS key` | Check if key exists | `EXISTS counter` |
| `TYPE` | `TYPE key` | Get value type | `TYPE mylist` |
| `EXPIRE` | `EXPIRE key seconds` | Set expiration | `EXPIRE session 3600` |
| `TTL` | `TTL key` | Get time to live | `TTL session` |
| `PERSIST` | `PERSIST key` | Remove expiration | `PERSIST session` |

**TTL Return Values:**
- `-1`: Key exists but has no expiration
- `-2`: Key does not exist or is expired
- `>0`: Remaining seconds until expiration

### List Operations

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `LPUSH` | `LPUSH key value [value ...]` | Push to list head | `LPUSH queue task1 task2` |
| `RPUSH` | `RPUSH key value [value ...]` | Push to list tail | `RPUSH queue task3` |
| `LPOP` | `LPOP key` | Pop from list head | `LPOP queue` |
| `RPOP` | `RPOP key` | Pop from list tail | `RPOP queue` |
| `LLEN` | `LLEN key` | Get list length | `LLEN queue` |
| `LRANGE` | `LRANGE key start stop` | Get range of elements | `LRANGE queue 0 -1` |
| `LINDEX` | `LINDEX key index` | Get element by index | `LINDEX queue 0` |
| `LSET` | `LSET key index value` | Set element by index | `LSET queue 0 "new task"` |

### Set Operations

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `SADD` | `SADD key member [member ...]` | Add to set | `SADD tags redis database` |
| `SREM` | `SREM key member` | Remove from set | `SREM tags database` |
| `SMEMBERS` | `SMEMBERS key` | Get all members | `SMEMBERS tags` |
| `SISMEMBER` | `SISMEMBER key member` | Check membership | `SISMEMBER tags redis` |
| `SCARD` | `SCARD key` | Get set size | `SCARD tags` |

### Hash Operations

| Command | Syntax | Description | Example |
|---------|--------|-------------|---------|
| `HSET` | `HSET key field value` | Set hash field | `HSET user:1 name "Alice"` |
| `HGET` | `HGET key field` | Get hash field | `HGET user:1 name` |
| `HDEL` | `HDEL key field` | Delete hash field | `HDEL user:1 temp` |
| `HGETALL` | `HGETALL key` | Get all fields | `HGETALL user:1` |
| `HKEYS` | `HKEYS key` | Get all field names | `HKEYS user:1` |
| `HVALS` | `HVALS key` | Get all values | `HVALS user:1` |
| `HLEN` | `HLEN key` | Get field count | `HLEN user:1` |
| `HEXISTS` | `HEXISTS key field` | Check field exists | `HEXISTS user:1 email` |

### Persistence Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| `SAVE` | `SAVE` | Save RDB snapshot and start new AOF |

## Data Persistence

TinyRedis implements two persistence mechanisms:

### RDB (Redis Database) Snapshots

- **File**: `dump.json`
- **Format**: JSON serialization of the entire database
- **Trigger**: Manual via `SAVE` command or auto-save interval (default: 60 seconds)
- **Loading**: Automatically loaded on server startup

**Example RDB content:**
```json
{
  "keys": {
    "user:1": {
      "type": "hash",
      "data": {
        "name": "Alice",
        "email": "alice@example.com"
      }
    },
    "counter": {
      "type": "integer",
      "data": 42
    }
  }
}
```

### AOF (Append-Only File)

- **File**: `dump.aof`
- **Format**: Sequential log of write commands
- **Behavior**: Every write operation is appended
- **Recovery**: Replays commands on startup
- **Reset**: New AOF started after `SAVE` command

**Example AOF content:**
```
SET greeting "Hello"
INCR counter
LPUSH queue task1
HSET user:1 name "Alice"
```

### Persistence Configuration

Modify in `Db` constructor (db.cpp):

```cpp
Db db(
    "dump.json",    // RDB filename
    "dump.aof",     // AOF filename
    60              // Auto-save interval (seconds)
);
```

## RESP Protocol

TinyRedis implements the Redis Serialization Protocol (RESP) for network communication.

### Supported RESP Types

| Type | Prefix | Example |
|------|--------|---------|
| Simple String | `+` | `+OK\r\n` |
| Error | `-` | `-ERR unknown command\r\n` |
| Integer | `:` | `:42\r\n` |
| Bulk String | `$` | `$5\r\nHello\r\n` |
| Array | `*` | `*2\r\n$3\r\nGET\r\n$3\r\nkey\r\n` |
| Null | `$-1` | `$-1\r\n` |

### Plain Text Fallback

For easier testing, the server also accepts plain text commands:

```bash
echo "SET mykey myvalue" | nc localhost 6379
echo "GET mykey" | nc localhost 6379
```

## Project Structure

```
tinyredis/
├── main_server.cpp    # Server entry point
├── main.cpp           # CLI entry point
├── server.h           # Server class declaration
├── server.cpp         # Server implementation
├── db.h               # Database class declaration
├── db.cpp             # Database implementation
├── value.h            # Value type definitions
├── value.cpp          # Value implementation
├── resp.h             # RESP protocol declaration
├── resp.cpp           # RESP protocol implementation
├── Makefile           # Build configuration
└── README.md          # This file
```


### Value Type System

```cpp
struct Value {
    ValueType type;  // STRING, INTEGER, LIST, SET, HASH
    
    std::variant<
        std::string,                              // STRING
        long long,                                // INTEGER
        std::deque<std::string>,                  // LIST
        std::unordered_set<std::string>,          // SET
        std::unordered_map<string, string>        // HASH
    > data;
    
    std::optional<std::chrono::time_point> expiration;
};
```


## Contributing

Contributions are welcome! Here are some ideas:

### Beginner-Friendly Tasks
- Add more string commands (GETSET, SETNX)
- Implement additional list commands (LINSERT, LREM)
- Add sorted set data type (ZSET)
- Improve error messages

### Intermediate Tasks
- Add mutex protection for thread safety
- Implement connection pooling
- Add command pipelining support
- Optimize RDB format (binary instead of JSON)
- Add benchmarking tools

### Advanced Tasks
- Implement pub/sub messaging
- Add Lua scripting support
- Create basic replication
- Implement memory eviction policies
- Add stream data type

**How to contribute:**

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see below for details:

```
MIT License

Copyright (c) 2024 krednie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Acknowledgments

- **Redis**: For the original design and RESP protocol specification
- **C++ Community**: For modern C++ best practices and standard library
- **Educational Resources**: Various tutorials on database and network programming

## Contact & Support

- **GitHub**: [github.com/krednie/tinyredis](https://github.com/krednie/tinyredis)
- **Issues**: Please report bugs via GitHub Issues
- **Discussions**: Use GitHub Discussions for questions and ideas

---

**⭐ If you find this project helpful, please consider giving it a star!**

Built with ❤️ as a learning project to understand Redis internals and C++ system programming.
