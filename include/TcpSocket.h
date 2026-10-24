#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "Socket.h"
#include "Message.h"
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>

// TCP Socket class for reliable connection-oriented communication
class TcpSocket : public Socket {
public:
    // Constructor
    TcpSocket() : Socket() {
        // Create TCP socket
        socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd == -1) {
            throw std::runtime_error("Failed to create TCP socket");
        }

        // Set socket options to reuse address
        int opt = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set TCP socket options");
        }
    }

    // Constructor from existing socket descriptor (used for accepting connections)
    TcpSocket(int fd, struct sockaddr_in addr) : Socket() {
        socketFd = fd;
        address = addr;
    }

    // Bind socket to a specific port
    void bind(int port) {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (::bind(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind TCP socket to port " + std::to_string(port));
        }
    }

    // Start listening for connections
    void listen(int backlog = 10) {
        if (::listen(socketFd, backlog) < 0) {
            throw std::runtime_error("Failed to listen on TCP socket");
        }
    }

    // Accept a new connection
    TcpSocket accept() {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        
        int clientFd = ::accept(socketFd, (struct sockaddr*)&clientAddr, &addrLen);
        
        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking socket with no pending connections
                throw std::runtime_error("No pending connections");
            } else {
                throw std::runtime_error("Failed to accept connection");
            }
        }
        
        return TcpSocket(clientFd, clientAddr);
    }

    // Connect to a server
    void connect(const std::string& ip, int port) {
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address: " + ip);
        }
        
        if (::connect(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Failed to connect to " + ip + ":" + std::to_string(port));
        }
    }

    // Send data over TCP
    ssize_t send(const void* data, size_t length) {
        ssize_t bytesSent = ::send(socketFd, data, length, 0);
        if (bytesSent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Failed to send data over TCP socket");
        }
        return bytesSent;
    }

    // Send a structured message
    bool sendMessage(const Message& message) {
        const char* data = reinterpret_cast<const char*>(&message);
        size_t totalToSend = sizeof(Message);
        size_t totalSent = 0;
        
        while (totalSent < totalToSend) {
            ssize_t bytesSent = send(data + totalSent, totalToSend - totalSent);
            
            if (bytesSent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket would block, try again after a short delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                return false; // Error occurred
            } else if (bytesSent == 0) {
                return false; // Connection closed
            }
            
            totalSent += bytesSent;
        }
        
        return true;
    }

    // Receive data over TCP
    ssize_t receive(void* buffer, size_t length) {
        ssize_t bytesReceived = ::recv(socketFd, buffer, length, 0);
        if (bytesReceived < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to receive data from TCP socket");
            }
            return -1; // Would block
        }
        return bytesReceived;
    }

    // Receive a structured message
    bool receiveMessage(Message& message) {
        char* data = reinterpret_cast<char*>(&message);
        size_t totalToReceive = sizeof(Message);
        size_t totalReceived = 0;
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(3);
        
        while (totalReceived < totalToReceive) {
            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime - startTime > timeout) {
                std::cerr << "Timeout receiving message after " << totalReceived << " bytes" << std::endl;
                return false;
            }
            
            ssize_t bytesReceived = receive(data + totalReceived, totalToReceive - totalReceived);
            
            if (bytesReceived < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Socket would block, try again after a short delay
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                return false; // Error occurred
            } else if (bytesReceived == 0) {
                if (totalReceived > 0) {
                    std::cerr << "Connection closed after receiving " << totalReceived << " bytes" << std::endl;
                }
                return false; // Connection closed
            }
            
            totalReceived += bytesReceived;
        }
        
        return true;
    }

    // Get client IP address
    std::string getClientIp() const {
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), clientIp, INET_ADDRSTRLEN);
        return std::string(clientIp);
    }

    // Get client port
    int getClientPort() const {
        return ntohs(address.sin_port);
    }
};

#endif // TCP_SOCKET_H 