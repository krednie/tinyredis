#include "db.h"
#include "server.h"
#include <iostream>
#include <signal.h>

Server *global_server = nullptr;

void signalHandler(int signum)
{
    std::cout << "\n# Received signal " << signum << " (Ctrl+C)" << std::endl;
    std::cout << "# Shutting down server..." << std::endl;
    
    if (global_server)
    {
        global_server->stop();
    }
    
    exit(0);
}

int main(int argc, char *argv[])
{
    int port = 6379;
    
    // Allow custom port via command line
    if (argc > 1)
    {
        port = std::stoi(argv[1]);
    }

    std::cout << "# Starting TinyRedis Server..." << std::endl;

    // Setup signal handler for Ctrl+C
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create database
    Db db;

    // Create and start server
    Server server(db, port);
    global_server = &server;

    std::cout << "# Press Ctrl+C to stop the server" << std::endl;
    server.start();

    return 0;
}