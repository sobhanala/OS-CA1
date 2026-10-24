#include "../../include/Server.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>

// Global server instance for signal handling
Server* g_server = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
    }
}

// Print usage information
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <port>\n"
              << "  <port>: TCP port number for the server (UDP will use the same port)\n"
              << "Example: " << programName << " 8000" << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse port number
    int port;
    try {
        port = std::stoi(argv[1]);
        if (port <= 0 || port > 65535) {
            throw std::out_of_range("Port must be between 1 and 65535");
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << e.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        // Create and start server
        Server server(port, port);
        g_server = &server;
        
        std::cout << "Starting server on port " << port << "..." << std::endl;
        server.start();
        
        // Start the competition
        std::cout << "Press Enter to start the competition...";
        std::cin.get();
        
        server.startCompetition();
        
        // Run server main loop
        server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    
    g_server = nullptr;
    std::cout << "Server terminated." << std::endl;
    return 0;
} 