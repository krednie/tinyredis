#include "server.h"
#include "resp.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>

Server::Server(Db &db, int port) : db_(db), port_(port), running_(false)
{
    server_socket_ = -1;
}

Server::~Server()
{
    stop();
}

void Server::start()
{
    // Step 1: Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    std::cout << "# Socket created: " << server_socket_ << std::endl;

    // Step 2: Set socket options (allow reuse of address)
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket_);
        return;
    }

    std::cout << "# Socket options set (SO_REUSEADDR)" << std::endl;

    // Step 3: Bind socket to address and port
    struct sockaddr_in address;
    address.sin_family = AF_INET;           // IPv4
    address.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces (0.0.0.0)
    address.sin_port = htons(port_);        // Convert port to network byte order

    if (bind(server_socket_, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(server_socket_);
        return;
    }

    std::cout << "# Socket bound to 0.0.0.0:" << port_ << std::endl;

    // Step 4: Listen for connections (backlog = 10)
    if (listen(server_socket_, 10) < 0)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket_);
        return;
    }

    std::cout << "# Socket listening (backlog: 10)" << std::endl;

    running_ = true;
    std::cout << "# TinyRedis server started on port " << port_ << std::endl;
    std::cout << "# Ready to accept connections" << std::endl;
    std::cout << "# Waiting for clients..." << std::endl;

    // Step 5: Accept connections in a loop
    while (running_)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        std::cout << "# Calling accept() - blocking until client connects..." << std::endl;
        
        int client_socket = accept(server_socket_, (struct sockaddr *)&client_addr, &client_len);

        if (client_socket < 0)
        {
            if (running_)
                std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "# Client connected from " << client_ip 
                  << ":" << ntohs(client_addr.sin_port) 
                  << " (socket: " << client_socket << ")" << std::endl;

        // Handle client in a new thread
        std::thread client_thread(&Server::handleClient, this, client_socket);
        client_thread.detach();  // Let it run independently
        
        std::cout << "# Spawned thread for client, going back to accept()..." << std::endl;
    }
}

void Server::handleClient(int client_socket)
{
    std::cout << "# [Thread " << std::this_thread::get_id() << "] Handling client on socket " << client_socket << std::endl;
    
    char buffer[4096];

    while (running_)
    {
        memset(buffer, 0, sizeof(buffer));
        
        std::cout << "# [Thread " << std::this_thread::get_id() << "] Waiting for data (recv)..." << std::endl;
        
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0)
        {
            if (bytes_read == 0)
                std::cout << "# [Thread " << std::this_thread::get_id() << "] Client disconnected cleanly" << std::endl;
            else
                std::cout << "# [Thread " << std::this_thread::get_id() << "] recv() error: " << strerror(errno) << std::endl;
            break;
        }

        std::cout << "# [Thread " << std::this_thread::get_id() << "] Received " << bytes_read << " bytes" << std::endl;
        
        std::string data(buffer, bytes_read);
        
        // Parse RESP command
        std::vector<std::string> tokens = RESP::parse(data);
        
        if (tokens.empty())
        {
            std::cout << "# [Thread " << std::this_thread::get_id() << "] Failed to parse RESP, sending error" << std::endl;
            std::string response = RESP::encodeError("ERR invalid command format");
            send(client_socket, response.c_str(), response.length(), 0);
            continue;
        }

        std::cout << "# [Thread " << std::this_thread::get_id() << "] Command: " << tokens[0];
        if (tokens.size() > 1)
            std::cout << " (with " << (tokens.size() - 1) << " arguments)";
        std::cout << std::endl;

        // Execute command
        std::string response = executeCommand(tokens);
        
        std::cout << "# [Thread " << std::this_thread::get_id() << "] Sending response (" << response.length() << " bytes)" << std::endl;
        
        send(client_socket, response.c_str(), response.length(), 0);
    }

    close(client_socket);
    std::cout << "# [Thread " << std::this_thread::get_id() << "] Socket closed, thread exiting" << std::endl;
}

std::string Server::executeCommand(const std::vector<std::string> &tokens)
{
    if (tokens.empty())
        return RESP::encodeError("ERR empty command");

    std::string cmd = tokens[0];
    
    // Convert to uppercase
    for (char &c : cmd)
        c = toupper(c);

    std::cout << "# [executeCommand] Processing: " << cmd << std::endl;

    // Handle PING command
    if (cmd == "PING")
    {
        if (tokens.size() == 1)
            return RESP::encodeSimpleString("PONG");
        else
            return RESP::encodeBulkString(tokens[1]);
    }
    
    // Handle ECHO command
    else if (cmd == "ECHO" && tokens.size() >= 2)
    {
        return RESP::encodeBulkString(tokens[1]);
    }

    // Handle SET command
    else if (cmd == "SET" && tokens.size() >= 3)
    {
        // Join remaining tokens as value (for values with spaces)
        std::string value;
        for (size_t i = 2; i < tokens.size(); i++)
        {
            if (i > 2) value += " ";
            value += tokens[i];
        }
        db_.set(tokens[1], value);
        return RESP::encodeSimpleString("OK");
    }

    // Handle GET command
    else if (cmd == "GET" && tokens.size() >= 2)
    {
        // We need to capture output - for now, use the db method and return appropriate RESP
        // This is a workaround since db_.get() prints to stdout
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool found = db_.get(tokens[1]);
        std::cout.rdbuf(old);
        
        if (!found)
            return RESP::encodeNullBulkString();
        
        std::string result = buffer.str();
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n')
            result.pop_back();
        
        return RESP::encodeBulkString(result);
    }

    // Handle DEL command
    else if (cmd == "DEL" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool deleted = db_.del(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(deleted ? 1 : 0);
    }

    // Handle EXISTS command
    else if (cmd == "EXISTS" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool exists = db_.exists(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(exists ? 1 : 0);
    }

    // Handle INCR command
    else if (cmd == "INCR" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        db_.incr(tokens[1]);
        std::cout.rdbuf(old);
        
        std::string result = buffer.str();
        // Parse the integer from "(integer) N"
        size_t pos = result.find(") ");
        if (pos != std::string::npos)
        {
            std::string numStr = result.substr(pos + 2);
            if (!numStr.empty() && numStr.back() == '\n')
                numStr.pop_back();
            try {
                return RESP::encodeInteger(std::stoll(numStr));
            } catch (...) {
                return RESP::encodeError("ERR value is not an integer");
            }
        }
        return RESP::encodeError("ERR WRONGTYPE");
    }

    // Handle INCRBY command
    else if (cmd == "INCRBY" && tokens.size() >= 3)
    {
        long long amount = std::stoll(tokens[2]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        db_.incrby(tokens[1], amount);
        std::cout.rdbuf(old);
        
        std::string result = buffer.str();
        size_t pos = result.find(") ");
        if (pos != std::string::npos)
        {
            std::string numStr = result.substr(pos + 2);
            if (!numStr.empty() && numStr.back() == '\n')
                numStr.pop_back();
            try {
                return RESP::encodeInteger(std::stoll(numStr));
            } catch (...) {
                return RESP::encodeError("ERR value is not an integer");
            }
        }
        return RESP::encodeError("ERR WRONGTYPE");
    }

    // Handle DECR command
    else if (cmd == "DECR" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        db_.decr(tokens[1]);
        std::cout.rdbuf(old);
        
        std::string result = buffer.str();
        size_t pos = result.find(") ");
        if (pos != std::string::npos)
        {
            std::string numStr = result.substr(pos + 2);
            if (!numStr.empty() && numStr.back() == '\n')
                numStr.pop_back();
            try {
                return RESP::encodeInteger(std::stoll(numStr));
            } catch (...) {
                return RESP::encodeError("ERR value is not an integer");
            }
        }
        return RESP::encodeError("ERR WRONGTYPE");
    }

    // Handle DECRBY command
    else if (cmd == "DECRBY" && tokens.size() >= 3)
    {
        long long amount = std::stoll(tokens[2]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        db_.decrby(tokens[1], amount);
        std::cout.rdbuf(old);
        
        std::string result = buffer.str();
        size_t pos = result.find(") ");
        if (pos != std::string::npos)
        {
            std::string numStr = result.substr(pos + 2);
            if (!numStr.empty() && numStr.back() == '\n')
                numStr.pop_back();
            try {
                return RESP::encodeInteger(std::stoll(numStr));
            } catch (...) {
                return RESP::encodeError("ERR value is not an integer");
            }
        }
        return RESP::encodeError("ERR WRONGTYPE");
    }

    // Handle EXPIRE command
    else if (cmd == "EXPIRE" && tokens.size() >= 3)
    {
        long long seconds = std::stoll(tokens[2]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool success = db_.expire(tokens[1], seconds);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(success ? 1 : 0);
    }

    // Handle TTL command
    else if (cmd == "TTL" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long ttl = db_.ttl(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(ttl);
    }

    // Handle PERSIST command
    else if (cmd == "PERSIST" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool success = db_.persist(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(success ? 1 : 0);
    }

    // Handle TYPE command
    else if (cmd == "TYPE" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::string type = db_.type(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeSimpleString(type);
    }

    // Handle APPEND command
    else if (cmd == "APPEND" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long newLen = db_.append(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(newLen);
    }

    // Handle STRLEN command
    else if (cmd == "STRLEN" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long len = db_.strlen(tokens[1]);
        std::cout.rdbuf(old);
        
        return RESP::encodeInteger(len);
    }

    // ============== List Commands ==============
    
    // Handle LPUSH command
    else if (cmd == "LPUSH" && tokens.size() >= 3)
    {
        std::vector<std::string> values(tokens.begin() + 2, tokens.end());
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long len = db_.lpush(tokens[1], values);
        std::cout.rdbuf(old);
        
        if (len < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(len);
    }

    // Handle RPUSH command
    else if (cmd == "RPUSH" && tokens.size() >= 3)
    {
        std::vector<std::string> values(tokens.begin() + 2, tokens.end());
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long len = db_.rpush(tokens[1], values);
        std::cout.rdbuf(old);
        
        if (len < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(len);
    }

    // Handle LPOP command
    else if (cmd == "LPOP" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::string result = db_.lpop(tokens[1]);
        std::cout.rdbuf(old);
        
        if (result.empty() && buffer.str().find("nil") != std::string::npos)
            return RESP::encodeNullBulkString();
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeBulkString(result);
    }

    // Handle RPOP command
    else if (cmd == "RPOP" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::string result = db_.rpop(tokens[1]);
        std::cout.rdbuf(old);
        
        if (result.empty() && buffer.str().find("nil") != std::string::npos)
            return RESP::encodeNullBulkString();
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeBulkString(result);
    }

    // Handle LLEN command
    else if (cmd == "LLEN" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long len = db_.llen(tokens[1]);
        std::cout.rdbuf(old);
        
        if (len < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(len);
    }

    // Handle LRANGE command
    else if (cmd == "LRANGE" && tokens.size() >= 4)
    {
        long long start = std::stoll(tokens[2]);
        long long stop = std::stoll(tokens[3]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::vector<std::string> result = db_.lrange(tokens[1], start, stop);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeArray(result);
    }

    // Handle LINDEX command
    else if (cmd == "LINDEX" && tokens.size() >= 3)
    {
        long long index = std::stoll(tokens[2]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::string result = db_.lindex(tokens[1], index);
        std::cout.rdbuf(old);
        
        if (result.empty() && buffer.str().find("nil") != std::string::npos)
            return RESP::encodeNullBulkString();
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeBulkString(result);
    }

    // Handle LSET command
    else if (cmd == "LSET" && tokens.size() >= 4)
    {
        long long index = std::stoll(tokens[2]);
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool success = db_.lset(tokens[1], index, tokens[3]);
        std::cout.rdbuf(old);
        
        if (!success)
        {
            if (buffer.str().find("WRONGTYPE") != std::string::npos)
                return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
            if (buffer.str().find("no such key") != std::string::npos)
                return RESP::encodeError("ERR no such key");
            return RESP::encodeError("ERR index out of range");
        }
        return RESP::encodeSimpleString("OK");
    }

    // ============== Set Commands ==============
    
    // Handle SADD command
    else if (cmd == "SADD" && tokens.size() >= 3)
    {
        std::vector<std::string> members(tokens.begin() + 2, tokens.end());
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long added = db_.sadd(tokens[1], members);
        std::cout.rdbuf(old);
        
        if (added < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(added);
    }

    // Handle SREM command
    else if (cmd == "SREM" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long removed = db_.srem(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        if (removed < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(removed);
    }

    // Handle SMEMBERS command
    else if (cmd == "SMEMBERS" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::vector<std::string> members = db_.smembers(tokens[1]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeArray(members);
    }

    // Handle SISMEMBER command
    else if (cmd == "SISMEMBER" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool exists = db_.sismember(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(exists ? 1 : 0);
    }

    // Handle SCARD command
    else if (cmd == "SCARD" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long size = db_.scard(tokens[1]);
        std::cout.rdbuf(old);
        
        if (size < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(size);
    }

    // ============== Hash Commands ==============
    
    // Handle HSET command
    else if (cmd == "HSET" && tokens.size() >= 4)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool isNew = db_.hset(tokens[1], tokens[2], tokens[3]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        // Parse isNew from buffer output "(integer) N"
        return RESP::encodeInteger(buffer.str().find("1") != std::string::npos ? 1 : 0);
    }

    // Handle HGET command
    else if (cmd == "HGET" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::string result = db_.hget(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("nil") != std::string::npos)
            return RESP::encodeNullBulkString();
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeBulkString(result);
    }

    // Handle HDEL command
    else if (cmd == "HDEL" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool deleted = db_.hdel(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(deleted ? 1 : 0);
    }

    // Handle HGETALL command
    else if (cmd == "HGETALL" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        auto pairs = db_.hgetall(tokens[1]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        
        // Flatten pairs into array
        std::vector<std::string> result;
        for (const auto& p : pairs)
        {
            result.push_back(p.first);
            result.push_back(p.second);
        }
        return RESP::encodeArray(result);
    }

    // Handle HKEYS command
    else if (cmd == "HKEYS" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::vector<std::string> keys = db_.hkeys(tokens[1]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeArray(keys);
    }

    // Handle HVALS command
    else if (cmd == "HVALS" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        std::vector<std::string> vals = db_.hvals(tokens[1]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeArray(vals);
    }

    // Handle HLEN command
    else if (cmd == "HLEN" && tokens.size() >= 2)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        long long len = db_.hlen(tokens[1]);
        std::cout.rdbuf(old);
        
        if (len < 0) return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(len);
    }

    // Handle HEXISTS command
    else if (cmd == "HEXISTS" && tokens.size() >= 3)
    {
        std::stringstream buffer;
        std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
        bool exists = db_.hexists(tokens[1], tokens[2]);
        std::cout.rdbuf(old);
        
        if (buffer.str().find("WRONGTYPE") != std::string::npos)
            return RESP::encodeError("WRONGTYPE Operation against a key holding the wrong kind of value");
        return RESP::encodeInteger(exists ? 1 : 0);
    }

    return RESP::encodeError("ERR unknown command '" + tokens[0] + "'");
}

void Server::stop()
{
    running_ = false;
    if (server_socket_ >= 0)
    {
        std::cout << "# Closing server socket..." << std::endl;
        close(server_socket_);
        server_socket_ = -1;
    }
}