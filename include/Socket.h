#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

// Base Socket class for handling common socket operations
class Socket {
protected:
    int socketFd;
    struct sockaddr_in address;
    bool isNonBlocking;

public:
    // Constructor
    Socket() : socketFd(-1), isNonBlocking(false) {
        memset(&address, 0, sizeof(address));
    }

    // Destructor
    virtual ~Socket() {
        close();
    }

    // Close socket if open
    void close() {
        if (socketFd != -1) {
            ::close(socketFd);
            socketFd = -1;
        }
    }

    // Set socket to non-blocking mode
    void setNonBlocking(bool nonBlocking) {
        if (socketFd != -1) {
            int flags = fcntl(socketFd, F_GETFL, 0);
            if (flags == -1) {
                throw std::runtime_error("Failed to get socket flags");
            }

            if (nonBlocking) {
                flags |= O_NONBLOCK;
            } else {
                flags &= ~O_NONBLOCK;
            }

            if (fcntl(socketFd, F_SETFL, flags) == -1) {
                throw std::runtime_error("Failed to set socket non-blocking mode");
            }
            
            isNonBlocking = nonBlocking;
        }
    }

    // Get socket file descriptor
    int getSocketFd() const {
        return socketFd;
    }

    // Check if socket is valid
    bool isValid() const {
        return socketFd != -1;
    }
};

#endif // SOCKET_H 