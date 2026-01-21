#include "db.h"
#include <iostream>
#include <sstream>
#include <vector>

static std::vector<std::string> split(const std::string &line)
{
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string word;

    while (iss >> word)
        tokens.push_back(word);

    return tokens;
}
int main()
{
    Db redis;

    std::string line;

    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
            break;

        auto tokens = split(line);
        if (tokens.empty())
            continue;

        const std::string &cmd = tokens[0];

        if (cmd == "SET" || cmd == "set")
        {
            if (tokens.size() < 3)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.set(tokens[1], tokens[2]);
        }
        else if (cmd == "GET" || cmd == "get")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.get(tokens[1]);
        }
        else if (cmd == "DEL" || cmd == "del")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.del(tokens[1]);
        }
        else if (cmd == "EXISTS" || cmd == "exists")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.exists(tokens[1]);
        }
        else if (cmd == "INCR" || cmd == "incr")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.incr(tokens[1]);
        }
        else if (cmd == "INCRBY" || cmd == "incrby")
        {
            if (tokens.size() < 3)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            long long amt = std::stoll(tokens[2]);
            redis.incrby(tokens[1], amt);
        }
        else if (cmd == "DECR" || cmd == "decr")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.decr(tokens[1]);
        }
        else if (cmd == "DECRBY" || cmd == "decrby")
        {
            if (tokens.size() < 3)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            long long amt = std::stoll(tokens[2]);
            redis.decrby(tokens[1], amt);
        }
        else if (cmd == "TYPE" || cmd == "type")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.type(tokens[1]);
        }
        else if (cmd == "EXPIRE" || cmd == "expire")
        {
            if (tokens.size() < 3)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            long long seconds = std::stoll(tokens[2]);
            redis.expire(tokens[1], seconds);
        }
        else if (cmd == "TTL" || cmd == "ttl")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.ttl(tokens[1]);
        }
        else if (cmd == "PERSIST" || cmd == "persist")
        {
            if (tokens.size() < 2)
            {
                std::cout << "(error) wrong number of arguments" << std::endl;
                continue;
            }
            redis.persist(tokens[1]);
        }

        else if (cmd == "QUIT" || cmd == "quit" || cmd == "EXIT" || cmd == "exit")
        {
            break;
        }
        else
        {
            std::cout << "(error) unknown command" << std::endl;
        }
    }
    return 0;
}
