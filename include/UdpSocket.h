#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include "Socket.h"
#include "Message.h"
#include <string>

// UDP Socket class for connectionless communication
class UdpSocket : public Socket {
public:
    // Constructor
    UdpSocket() : Socket() {
        // Create UDP socket
        socketFd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socketFd == -1) {
            throw std::runtime_error("Failed to create UDP socket");
        }

        // Set socket options to reuse address
        int opt = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("Failed to set UDP socket options");
        }
    }

    // Bind socket to a specific port
    void bind(int port) {
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (::bind(socketFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw std::runtime_error("Failed to bind UDP socket to port " + std::to_string(port));
        }
    }

    // Enable broadcast mode
    void enableBroadcast() {
        int broadcastEnable = 1;
        if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
            throw std::runtime_error("Failed to enable broadcast mode for UDP socket");
        }
    }

    // Send data to a specific address and port
    ssize_t sendTo(const void* data, size_t length, const std::string& ip, int port) {
        struct sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ip.c_str(), &destAddr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid address: " + ip);
        }
        
        ssize_t bytesSent = sendto(socketFd, data, length, 0, 
                                  (struct sockaddr*)&destAddr, sizeof(destAddr));
                                  
        if (bytesSent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            throw std::runtime_error("Failed to send data over UDP socket");
        }
        
        return bytesSent;
    }

    // Send a broadcast message to a specific port
    ssize_t sendBroadcast(const void* data, size_t length, int port) {
        return sendTo(data, length, "255.255.255.255", port);
    }

    // Send a structured message to a specific address and port
    ssize_t sendMessageTo(const Message& message, const std::string& ip, int port) {
        return sendTo(&message, sizeof(Message), ip, port);
    }

    // Send a structured broadcast message to a specific port
    ssize_t sendBroadcastMessage(const Message& message, int port) {
        return sendBroadcast(&message, sizeof(Message), port);
    }

    // Receive data from any sender
    ssize_t receiveFrom(void* buffer, size_t length, std::string& senderIp, int& senderPort) {
        struct sockaddr_in senderAddr;
        socklen_t senderAddrLen = sizeof(senderAddr);
        
        ssize_t bytesReceived = recvfrom(socketFd, buffer, length, 0,
                                        (struct sockaddr*)&senderAddr, &senderAddrLen);
        
        if (bytesReceived < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("Failed to receive data from UDP socket");
            }
            return -1; // Would block
        }
        
        // Extract sender information
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(senderAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        senderIp = std::string(ipStr);
        senderPort = ntohs(senderAddr.sin_port);
        
        return bytesReceived;
    }

    // Receive a structured message from any sender
    bool receiveMessage(Message& message, std::string& senderIp, int& senderPort) {
        ssize_t bytesReceived = receiveFrom(&message, sizeof(Message), senderIp, senderPort);
        return bytesReceived == sizeof(Message);
    }
};

#endif // UDP_SOCKET_H 