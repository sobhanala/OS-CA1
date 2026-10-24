#ifndef CLIENT_H
#define CLIENT_H

#include "TcpSocket.h"
#include "UdpSocket.h"
#include "Message.h"
#include "MessageHelpers.h"
#include <string>
#include <functional>
#include <queue>
#include <map>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

// Client class for connecting to the competition server
class Client {
private:
    // Client configuration
    std::string username;
    UserRole role;
    std::string serverIp;
    int serverTcpPort;
    int serverUdpPort;
    
    // Sockets
    TcpSocket tcpSocket;
    UdpSocket udpSocket;
    
    // Team information
    std::string teamName;
    std::string partnerName;
    
    // State
    bool running;
    bool isRegistered;
    bool isInTeam;
    
    // Message queue for UI updates
    std::queue<Message> messageQueue;
    
    // Callbacks for message types
    typedef std::function<void(const Message&)> MessageCallback;
    std::map<MessageType, MessageCallback> callbacks;
    
    // Helper methods
    bool registerWithServer();
    void processTcpMessage();
    void processUdpMessage();
    void processMessage(const Message& message);
    void queueMessage(const Message& message);
    
    // Add socket to select monitoring
    void addToMonitor(int socketFd);
    
    // Remove socket from select monitoring
    void removeFromMonitor(int socketFd);
    
    // Socket set for select() 
    fd_set master_fds;
    fd_set read_fds;
    int maxfd;

public:
    // Constructor
    Client(const std::string& username, UserRole role, 
           const std::string& serverIp, int serverTcpPort, int serverUdpPort);
    
    // Destructor
    ~Client();
    
    // Connect to server
    bool connect();
    
    // Disconnect from server
    void disconnect();
    
    // Send a chat message to teammate
    bool sendChatMessage(const std::string& message);
    
    // Share code with navigator (coder only)
    bool shareCode(const std::string& code);
    
    // Submit code to server (navigator only)
    bool submitCode(const std::string& problemId, const std::string& code);
    
    // Process network events and return true if a message was processed
    bool processEvents(int timeout_ms = 0);
    
    // Get next message from queue
    bool getNextMessage(Message& message);
    
    // Register callback for a specific message type
    void registerCallback(MessageType type, MessageCallback callback);
    
    // Check if client is running
    bool isRunning() const;
    
    // Check if client is registered with server
    bool isUserRegistered() const;
    
    // Check if client is in a team
    bool isUserInTeam() const;
    
    // Get username
    std::string getUsername() const;
    
    // Get user role
    UserRole getUserRole() const;
    
    // Get team name
    std::string getTeamName() const;
    
    // Get partner name
    std::string getPartnerName() const;
};

#endif // CLIENT_H 