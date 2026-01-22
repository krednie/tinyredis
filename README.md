# TinyRedis - A Lightweight Redis-like In-Memory Database

A high-performance, Redis-compatible in-memory key-value store implemented in C++ with persistence, expiration support, and upcoming network capabilities.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Status](https://img.shields.io/badge/status-active-success.svg)

## 🚀 Features

### Current Features (v1.0)
- **String Operations**: SET, GET, APPEND, STRLEN, GETRANGE, SETRANGE
- **Integer Operations**: INCR, DECR, INCRBY, DECRBY
- **Key Management**: DEL, EXISTS, TYPE
- **Expiration**: EXPIRE, TTL, PERSIST with absolute timestamp persistence
- **Batch Operations**: MGET, MSET
- **Persistence**:
  - RDB snapshots (JSON format)
  - AOF (Append-Only File) logging
  - Auto-save every 60 seconds
  - JSON escape/unescape for special characters
- **Memory Management**: Automatic cleanup of expired keys

### 🔜 Coming Soon (v2.0)
- **Network Server**: TCP/IP server with RESP protocol
- **Python Client**: Full-featured Python client library
- **Data Structures**:
  - Lists (LPUSH, RPUSH, LPOP, RPOP, LRANGE, LLEN)
  - Hashes (HSET, HGET, HDEL, HGETALL, HKEYS, HVALS)
- **Advanced Features**:
  - Pub/Sub messaging
  - Transactions (MULTI/EXEC)
  - Connection pooling

## 📋 Table of Contents
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Commands](#commands)
- [Architecture](#architecture)
- [Persistence](#persistence)
- [Python Client (Coming Soon)](#python-client-coming-soon)
- [Contributing](#contributing)
- [Roadmap](#roadmap)

## 🔧 Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Make (optional)

### Build from Source
```bash
# Clone the repository
git clone https://github.com/yourusername/tinyredis.git
cd tinyredis

# Compile
g++ -std=c++17 main.cpp db.cpp value.cpp -o tinyredis

# Run
./tinyredis
```

### Using Make (optional)
```bash
make
./tinyredis
```

## 🎯 Quick Start
```bash
# Start TinyRedis
./tinyredis

# Basic operations
> SET mykey "Hello World"
OK
> GET mykey
Hello World
> INCR counter
(integer) 1
> EXPIRE mykey 10
(integer) 1
> TTL mykey
(integer) 10
> SAVE
OK
```

## 📖 Commands

### String Commands
| Command | Description | Example |
|---------|-------------|---------|
| `SET key value` | Set key to hold string value | `SET name "John"` |
| `GET key` | Get the value of key | `GET name` |
| `APPEND key value` | Append value to key | `APPEND name " Doe"` |
| `STRLEN key` | Get length of value | `STRLEN name` |
| `GETRANGE key start end` | Get substring | `GETRANGE name 0 3` |
| `SETRANGE key offset value` | Overwrite part of string | `SETRANGE name 0 "Jane"` |

### Integer Commands
| Command | Description | Example |
|---------|-------------|---------|
| `INCR key` | Increment by 1 | `INCR counter` |
| `DECR key` | Decrement by 1 | `DECR counter` |
| `INCRBY key amount` | Increment by amount | `INCRBY counter 5` |
| `DECRBY key amount` | Decrement by amount | `DECRBY counter 3` |

### Key Management
| Command | Description | Example |
|---------|-------------|---------|
| `DEL key` | Delete a key | `DEL mykey` |
| `EXISTS key` | Check if key exists | `EXISTS mykey` |
| `TYPE key` | Get type of value | `TYPE mykey` |
| `EXPIRE key seconds` | Set expiration | `EXPIRE mykey 60` |
| `TTL key` | Get time to live | `TTL mykey` |
| `PERSIST key` | Remove expiration | `PERSIST mykey` |

### Batch Operations
| Command | Description | Example |
|---------|-------------|---------|
| `MGET key1 key2 ...` | Get multiple values | `MGET name age city` |
| `MSET key1 val1 key2 val2 ...` | Set multiple values | `MSET name "John" age 30` |

### Persistence
| Command | Description | Example |
|---------|-------------|---------|
| `SAVE` | Manual save to disk | `SAVE` |

### System
| Command | Description | Example |
|---------|-------------|---------|
| `QUIT` / `EXIT` | Exit TinyRedis | `QUIT` |

## 🏗️ Architecture
```
tinyredis/
├── main.cpp          # CLI interface and command parser
├── db.h/db.cpp       # Core database logic, persistence
├── value.h/value.cpp # Value type with union (string/integer)
├── dump.json         # RDB snapshot file
└── dump.aof          # Append-only file
```

### Key Components

#### Value Structure
```cpp
struct Value {
    ValueType type;              // STRING or INTEGER
    union {
        std::string str;
        long long integer;
    };
    std::optional expiration;
};
```

#### Database Class
- **Storage**: `std::unordered_map<std::string, Value>`
- **Persistence**: Dual-layer (RDB + AOF)
- **Auto-save**: Configurable interval (default: 60s)

## 💾 Persistence

### RDB (Redis Database)
- **Format**: JSON
- **Timing**: Auto-save every 60 seconds + on shutdown
- **Features**: 
  - JSON escape/unescape for special characters
  - Absolute timestamp storage for expiration
  - Skips expired keys during save

Example RDB file:
```json
{
  "user:1": {
    "type": "string",
    "value": "John Doe",
    "expiration": 1737676800000
  },
  "counter": {
    "type": "integer",
    "value": 42,
    "expiration": null
  }
}
```

### AOF (Append-Only File)
- **Format**: Plain text commands
- **Timing**: After every write operation
- **Replay**: On startup, after loading RDB

Example AOF file:
```
SET user:1 John
INCR counter
EXPIRE user:1 3600
```

## 🐍 Python Client (Coming Soon)

### Installation
```bash
pip install tinyredis-py
```

### Usage Example
```python
import tinyredis

# Connect to TinyRedis server
client = tinyredis.TinyRedis(host='localhost', port=6379)

# String operations
client.set('mykey', 'Hello World')
value = client.get('mykey')

# Integer operations
client.incr('counter')
client.incrby('counter', 5)

# Expiration
client.expire('mykey', 60)
ttl = client.ttl('mykey')

# Lists (v2.0)
client.lpush('mylist', 'item1', 'item2')
items = client.lrange('mylist', 0, -1)

# Hashes (v2.0)
client.hset('user:1', 'name', 'John Doe')
client.hset('user:1', 'age', 30)
user = client.hgetall('user:1')
```

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Development Setup
```bash
# Clone your fork
git clone https://github.com/yourusername/tinyredis.git
cd tinyredis

# Create a branch
git checkout -b feature/my-feature

# Make changes and test
g++ -std=c++17 -Wall -Wextra main.cpp db.cpp value.cpp -o tinyredis
./tinyredis

# Run tests (when available)
make test
```

## 🗺️ Roadmap

### Phase 1: Core Database ✅ (Completed)
- [x] String operations
- [x] Integer operations
- [x] Key expiration
- [x] RDB persistence
- [x] AOF persistence
- [x] Auto-save mechanism

### Phase 2: Network Layer 🚧 (In Progress)
- [ ] TCP/IP server with RESP protocol
- [ ] Multi-client support
- [ ] Connection pooling
- [ ] Authentication

### Phase 3: Advanced Data Structures 📋 (Planned)
- [ ] Lists (LPUSH, RPUSH, LPOP, RPOP, LRANGE, LLEN, LINDEX)
- [ ] Hashes (HSET, HGET, HDEL, HGETALL, HKEYS, HVALS, HINCRBY)
- [ ] Sets (SADD, SREM, SMEMBERS, SISMEMBER)
- [ ] Sorted Sets (ZADD, ZRANGE, ZRANK, ZSCORE)

### Phase 4: Python Client 📋 (Planned)
- [ ] Basic connection handling
- [ ] All string/integer commands
- [ ] List operations
- [ ] Hash operations
- [ ] Connection pooling
- [ ] Async support

### Phase 5: Production Features 📋 (Future)
- [ ] Replication (Master-Slave)
- [ ] Pub/Sub messaging
- [ ] Transactions (MULTI/EXEC)
- [ ] Lua scripting
- [ ] Clustering
- [ ] Monitoring and stats
- [ ] Benchmark tools

## 📊 Performance

Current benchmarks (single-threaded, local):
- **SET**: ~100K ops/sec
- **GET**: ~150K ops/sec
- **INCR**: ~120K ops/sec
- **Memory**: ~50 bytes per key-value pair (avg)

*Benchmarks will be updated with network layer implementation*

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by [Redis](https://redis.io/)
- Built with modern C++17, Python
- Community feedback and contributions

## 📧 Contact

- **Author**: Yash kumar
- **Email**: krednie@gmail.com
- **GitHub**: [@krednie](https://github.com/krednie)
- **Issues**: [GitHub Issues](https://github.com/krednie/tinyredis/issues)

## ⭐ Star History

If you find this project useful, please consider giving it a star!

---

**Note**: This is an educational project demonstrating database internals and network programming. For production use, consider using [Redis](https://redis.io/) or [KeyDB](https://keydb.dev/).
