#ifndef SERVER_H
#define SERVER_H

#include "TcpSocket.h"
#include "UdpSocket.h"
#include "User.h"
#include "Team.h"
#include "Message.h"
#include "MessageHelpers.h"
#include "Problems.h"
#include "EvaluationClient.h"
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <algorithm>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

// Main server class for the competition platform
class Server {
private:
    // Server configuration
    int tcpPort;
    int udpPort;
    static constexpr int MAX_CLIENTS = 100;
    
    // Sockets
    TcpSocket tcpSocket;
    UdpSocket udpSocket;
    
    // User and team management
    std::map<std::string, std::shared_ptr<User>> users;  // username -> user
    std::map<int, std::string> socketToUsername;         // socket fd -> username
    std::vector<std::shared_ptr<Team>> teams;
    std::vector<std::shared_ptr<User>> unassignedUsers;
    
    // Problem management
    std::map<std::string, std::chrono::steady_clock::time_point> problemStartTimes;
    std::string currentProblemId;
    
    // Server state
    bool running;
    
    // File descriptor sets for select()
    fd_set master_fds;
    fd_set read_fds;
    int maxfd;
    
    // Last time update
    std::chrono::steady_clock::time_point lastTimeUpdate;

    // Register a new user
    void registerUser(int clientFd, const Message& message);
    
    // Form teams from unassigned users
    void formTeams();
    
    // Start a new problem
    void startProblem(const std::string& problemId);
    
    // End current problem
    void endProblem();
    
    // Process submitted code
    void processSubmission(int clientFd, const Message& message);
    
    // Broadcast message to all users
    void broadcastMessage(const Message& message);
    
    // Send message to a specific user
    void sendMessageToUser(const std::string& username, const Message& message);
    
    // Send message to a specific team
    void sendMessageToTeam(const std::string& teamName, const Message& message);
    
    // Send chat message between team members
    void handleChatMessage(int clientFd, const Message& message);
    
    // Handle code sharing between coder and navigator
    void handleCodeShareMessage(int clientFd, const Message& message);
    
    // Calculate score for a submission
    int calculateScore(const std::string& problemId, int timeTaken, bool passed);
    
    // Update scoreboard
    void updateScoreboard();
    
    // Send time updates to all teams
    void sendTimeUpdates();
    
    // Check and process time updates
    void checkTimeUpdates();
    
    // Add socket to select monitoring
    void addToMonitor(int socketFd);
    
    // Remove socket from select monitoring
    void removeFromMonitor(int socketFd);
    
    // Repair file descriptors after a Bad file descriptor error
    void repairFileDescriptors();
    
    // Helper for getting a user by socket
    std::shared_ptr<User> getUserBySocket(int socketFd);
    
    // Find team by name
    std::shared_ptr<Team> findTeamByName(const std::string& teamName);
    
    // Find team by username
    std::shared_ptr<Team> findTeamByUsername(const std::string& username);
    
    // Handle client disconnection
    void handleClientDisconnect(int clientFd);
    
    // Check if username is already taken
    bool isUsernameTaken(const std::string& username);
    
    // Get user by username
    std::shared_ptr<User> getUserByUsername(const std::string& username);
    
    // Process incoming UDP message
    void processUdpMessage();

public:
    // Constructor
    Server(int tcpPort, int udpPort);
    
    // Destructor
    ~Server();
    
    // Start the server
    void start();
    
    // Stop the server
    void stop();
    
    // Run the server loop
    void run();
    
    // Start the competition
    void startCompetition();
    
    // End the competition
    void endCompetition();
};

#endif // SERVER_H 