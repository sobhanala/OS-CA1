#include "../../include/Client.h"
#include <iostream>
#include <chrono>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// Constructor
Client::Client(const std::string& username, UserRole role, 
               const std::string& serverIp, int serverTcpPort, int serverUdpPort)
    : username(username), 
      role(role), 
      serverIp(serverIp), 
      serverTcpPort(serverTcpPort),
      serverUdpPort(serverUdpPort),
      running(false),
      isRegistered(false),
      isInTeam(false),
      maxfd(0) {
    
    // Initialize select sets
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
}

// Destructor
Client::~Client() {
    disconnect();
}

// Connect to server
bool Client::connect() {
    try {
        // Connect to server via TCP
        tcpSocket.connect(serverIp, serverTcpPort);
        std::cout << "Connected to server TCP port " << serverTcpPort << std::endl;
        
        // Add TCP socket to monitoring
        addToMonitor(tcpSocket.getSocketFd());
        
        // Bind local UDP socket to receive broadcasts
        udpSocket.bind(serverUdpPort);
        udpSocket.setNonBlocking(true);
        std::cout << "Listening for UDP broadcasts on port " << serverUdpPort << std::endl;
        
        // Add UDP socket to monitoring
        addToMonitor(udpSocket.getSocketFd());
        
        // Set running flag
        running = true;
        
        // Register with server
        return registerWithServer();
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to server: " << e.what() << std::endl;
        disconnect();
        return false;
    }
}

// Disconnect from server
void Client::disconnect() {
    running = false;
    
    // حذف سوکت‌ها از مجموعه پایش
    if (tcpSocket.isValid()) {
        removeFromMonitor(tcpSocket.getSocketFd());
    }
    
    if (udpSocket.isValid()) {
        removeFromMonitor(udpSocket.getSocketFd());
    }
    
    // Close sockets
    tcpSocket.close();
    udpSocket.close();
    
    // پاکسازی متغیرهای وضعیت
    isRegistered = false;
    isInTeam = false;
    
    // پاکسازی صف پیام‌ها
    while (!messageQueue.empty()) {
        messageQueue.pop();
    }
    
    // مقداردهی مجدد maxfd
    maxfd = 0;
    
    std::cout << "Disconnected from server" << std::endl;
}

// Register with the server
bool Client::registerWithServer() {
    try {
        // Create registration message
        Message regMsg = MessageHelpers::createRegisterMessage(username, role);
        
        std::cout << "Sending registration request to server..." << std::endl;
        
        // Send to server
        if (!tcpSocket.sendMessage(regMsg)) {
            std::cerr << "Failed to send registration message to server" << std::endl;
            return false;
        }
        
        // Wait for response with timeout
        int retries = 10;
        bool received = false;
        Message response;
        
        while (retries > 0 && !received) {
            fd_set read_set;
            FD_ZERO(&read_set);
            
            if (tcpSocket.getSocketFd() <= 0) {
                std::cerr << "Invalid TCP socket in registerWithServer" << std::endl;
                return false;
            }
            
            FD_SET(tcpSocket.getSocketFd(), &read_set);
            
            struct timeval timeout;
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
            
            std::cout << "Waiting for registration response (retry " << (11 - retries) << "/10)..." << std::endl;
            
            int ready = select(tcpSocket.getSocketFd() + 1, &read_set, NULL, NULL, &timeout);
            
            if (ready > 0) {
                if (FD_ISSET(tcpSocket.getSocketFd(), &read_set)) {
                    if (tcpSocket.receiveMessage(response)) {
                        received = true;
                        break;
                    } else {
                        std::cerr << "Failed to receive registration response" << std::endl;
                    }
                }
            } else if (ready == 0) {
                std::cout << "Timeout waiting for registration response" << std::endl;
            } else if (ready < 0) {
                if (errno != EINTR) {
                    throw std::runtime_error("select() failed in registration: " + std::string(strerror(errno)));
                }
            }
            
            retries--;
        }
        
        if (!received) {
            std::cerr << "Error registering with server: Failed to receive data from TCP socket" << std::endl;
            return false;
        }
        
        // Check response
        if (response.type == MessageType::REGISTER_RESPONSE) {
            RegisterResponseMessage responseData = MessageHelpers::fromMessage<RegisterResponseMessage>(response);
            
            if (responseData.success) {
                std::cout << "Registration successful: " << responseData.message << std::endl;
                isRegistered = true;
                return true;
            } else {
                std::cerr << "Registration failed: " << responseData.message << std::endl;
                return false;
            }
        } else {
            std::cerr << "Unexpected response type: " << static_cast<int>(response.type) << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error registering with server: " << e.what() << std::endl;
        return false;
    }
}

// Process network events and return true if a message was processed
bool Client::processEvents(int timeout_ms) {
    if (!running) {
        return false;
    }
    
    // Copy master set to read set
    read_fds = master_fds;
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Wait for activity
    int activity = select(maxfd + 1, &read_fds, NULL, NULL, &tv);
    
    if (activity < 0) {
        if (errno == EINTR) {
            return false; // Interrupted by signal
        }
        throw std::runtime_error("select() failed: " + std::string(strerror(errno)));
    }
    
    if (activity == 0) {
        return false; // Timeout, no socket activity
    }
    
    // Check for activity on TCP socket
    if (FD_ISSET(tcpSocket.getSocketFd(), &read_fds)) {
        processTcpMessage();
        return true;
    }
    
    // Check for activity on UDP socket
    if (FD_ISSET(udpSocket.getSocketFd(), &read_fds)) {
        processUdpMessage();
        return true;
    }
    
    return false;
}

// Process received TCP message
void Client::processTcpMessage() {
    try {
        Message message;
        
        if (tcpSocket.receiveMessage(message)) {
            // Process message
            processMessage(message);
        } else {
            // Connection lost
            std::cerr << "Lost connection to server" << std::endl;
            running = false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in TCP receive: " << e.what() << std::endl;
    }
}

// Process received UDP message
void Client::processUdpMessage() {
    try {
        Message message;
        std::string senderIp;
        int senderPort;
        
        if (udpSocket.receiveMessage(message, senderIp, senderPort)) {
            // Process message
            processMessage(message);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in UDP receive: " << e.what() << std::endl;
    }
}

// Process received message
void Client::processMessage(const Message& message) {
    // Add to message queue for UI
    queueMessage(message);
    
    // Process by message type
    switch (message.type) {
        case MessageType::TEAM_FORMED: {
            TeamFormedMessage teamMsg = MessageHelpers::fromMessage<TeamFormedMessage>(message);
            teamName = teamMsg.team_name;
            
            // Determine partner name
            if (username == teamMsg.coder_name) {
                partnerName = teamMsg.navigator_name;
            } else {
                partnerName = teamMsg.coder_name;
            }
            
            isInTeam = true;
            
            std::cout << "Team formed: " << teamName << " with partner " << partnerName << std::endl;
            break;
        }
        
        case MessageType::PROBLEM:
        case MessageType::TIME_UPDATE:
        case MessageType::CHAT:
        case MessageType::CODE_SHARE:
        case MessageType::EVALUATION_RESULT:
        case MessageType::COMPETITION_START:
        case MessageType::COMPETITION_END:
        case MessageType::SCOREBOARD:
            // These messages are handled by the callbacks or UI
            break;
            
        default:
            std::cout << "Received message of type " << static_cast<int>(message.type) << std::endl;
            break;
    }
    
    // Call registered callback if exists
    auto callbackIt = callbacks.find(message.type);
    if (callbackIt != callbacks.end()) {
        callbackIt->second(message);
    }
}

// Send a chat message to teammate
bool Client::sendChatMessage(const std::string& message) {
    if (!isUserRegistered() || !isUserInTeam()) {
        std::cerr << "Cannot send chat message - not registered or not in a team" << std::endl;
        return false;
    }
    
    try {
        // Create chat message
        Message chatMsg = MessageHelpers::createChatMessage(username, message);
        
        // Send to server (which will forward to partner)
        tcpSocket.sendMessage(chatMsg);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to send chat message: " << e.what() << std::endl;
        return false;
    }
}

// Share code with navigator (coder only)
bool Client::shareCode(const std::string& code) {
    if (!isUserRegistered() || !isUserInTeam() || role != UserRole::CODER) {
        std::cerr << "Cannot share code - not registered, not in a team, or not a coder" << std::endl;
        return false;
    }
    
    try {
        // Create code share message
        Message codeMsg = MessageHelpers::createCodeShareMessage(code);
        
        // Send to server (which will forward to navigator)
        tcpSocket.sendMessage(codeMsg);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to share code: " << e.what() << std::endl;
        return false;
    }
}

// Submit code to server (navigator only)
bool Client::submitCode(const std::string& problemId, const std::string& code) {
    if (!isUserRegistered() || !isUserInTeam() || role != UserRole::NAVIGATOR) {
        std::cerr << "Cannot submit code - not registered, not in a team, or not a navigator" << std::endl;
        return false;
    }
    
    try {
        // Create code submit message
        Message submitMsg = MessageHelpers::createCodeSubmitMessage(
            teamName, problemId, code, 0);  // Server will calculate actual submission time
        
        // Send to server
        tcpSocket.sendMessage(submitMsg);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to submit code: " << e.what() << std::endl;
        return false;
    }
}

// Add message to queue
void Client::queueMessage(const Message& message) {
    messageQueue.push(message);
}

// Get next message from queue
bool Client::getNextMessage(Message& message) {
    if (messageQueue.empty()) {
        return false;
    }
    
    message = messageQueue.front();
    messageQueue.pop();
    return true;
}

// Add socket to select monitoring
void Client::addToMonitor(int socketFd) {
    FD_SET(socketFd, &master_fds);
    
    // Update max file descriptor for select()
    if (socketFd > maxfd) {
        maxfd = socketFd;
    }
}

// Remove socket from select monitoring
void Client::removeFromMonitor(int socketFd) {
    // Check if the socket is valid and in the set before removing
    if (socketFd > 0 && FD_ISSET(socketFd, &master_fds)) {
        FD_CLR(socketFd, &master_fds);
        
        // Recalculate maxfd if this was the highest
        if (socketFd == maxfd) {
            // Find the new highest fd
            maxfd = 0;
            
            // Check TCP socket
            if (tcpSocket.isValid() && tcpSocket.getSocketFd() > maxfd) {
                maxfd = tcpSocket.getSocketFd();
            }
            
            // Check UDP socket
            if (udpSocket.isValid() && udpSocket.getSocketFd() > maxfd) {
                maxfd = udpSocket.getSocketFd();
            }
        }
    }
}

// Register callback for a specific message type
void Client::registerCallback(MessageType type, MessageCallback callback) {
    callbacks[type] = callback;
}

// Check if client is running
bool Client::isRunning() const {
    return running;
}

// Check if client is registered with server
bool Client::isUserRegistered() const {
    return isRegistered;
}

// Check if client is in a team
bool Client::isUserInTeam() const {
    return isInTeam;
}

// Get username
std::string Client::getUsername() const {
    return username;
}

// Get user role
UserRole Client::getUserRole() const {
    return role;
}

// Get team name
std::string Client::getTeamName() const {
    return teamName;
}

// Get partner name
std::string Client::getPartnerName() const {
    return partnerName;
} 