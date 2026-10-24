#include "../../include/Server.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <csignal>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <fcntl.h>

// Constructor
Server::Server(int tcpPort, int udpPort)
    : tcpPort(tcpPort), udpPort(udpPort), running(false), maxfd(0) {
    
    currentProblemId = "";
    
    // Initialize select sets
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    
    // Initialize last time update
    lastTimeUpdate = std::chrono::steady_clock::now();
}

// Destructor
Server::~Server() {
    stop();
}

// Start the server
void Server::start() {
    try {
        // Initialize TCP socket
        tcpSocket.bind(tcpPort);
        tcpSocket.listen();
        tcpSocket.setNonBlocking(true);
        std::cout << "TCP server listening on port " << tcpPort << std::endl;
        
        // Initialize UDP socket
        udpSocket.bind(udpPort);
        udpSocket.enableBroadcast();
        udpSocket.setNonBlocking(true);
        std::cout << "UDP server listening on port " << udpPort << std::endl;
        
        // Add sockets to monitoring
        addToMonitor(tcpSocket.getSocketFd());
        addToMonitor(udpSocket.getSocketFd());
        
        running = true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to start server: " << e.what() << std::endl;
        throw;
    }
}

// Stop the server
void Server::stop() {
    running = false;
    
    // Clear all data structures
    users.clear();
    socketToUsername.clear();
    teams.clear();
    unassignedUsers.clear();
    
    // Close sockets
    tcpSocket.close();
    udpSocket.close();
    
    std::cout << "Server stopped" << std::endl;
}

// Run the server loop
void Server::run() {
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
    
    while (running) {
        try {
            if (maxfd <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Copy master set to read set
            read_fds = master_fds;
            
            // Set timeout for select (100ms)
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000; // 100ms
            
            // Wait for activity on sockets
            int activity = select(maxfd + 1, &read_fds, NULL, NULL, &timeout);
            
            if (activity < 0) {
                if (errno == EINTR) {
                    continue; // Interrupted by signal
                }
                
                std::cerr << "Error in server loop: select() failed: " << strerror(errno) << std::endl;
                
                if (errno == EBADF) {
                    repairFileDescriptors();
                    continue;
                }
                
                throw std::runtime_error("select() failed: " + std::string(strerror(errno)));
            }
            
            // Check time updates periodically, even if no socket activity
            checkTimeUpdates();
            
            if (activity == 0) {
                continue; // Timeout, no socket activity
            }
            
            // Check for activity on TCP server socket
            if (FD_ISSET(tcpSocket.getSocketFd(), &read_fds)) {
                try {
                    TcpSocket clientSocket = tcpSocket.accept();
                    std::cout << "New connection from " << clientSocket.getClientIp() 
                              << ":" << clientSocket.getClientPort() << std::endl;
                    
                    // Add to monitoring
                    addToMonitor(clientSocket.getSocketFd());
                } catch (const std::exception& e) {
                    if (std::string(e.what()) != "No pending connections") {
                        std::cerr << "Accept error: " << e.what() << std::endl;
                    }
                }
            }
            
            // Check for activity on UDP socket
            if (FD_ISSET(udpSocket.getSocketFd(), &read_fds)) {
                processUdpMessage();
            }
            
            // Check for activity on client sockets
            for (const auto& pair : socketToUsername) {
                int clientFd = pair.first;
                
                // Skip server sockets
                if (clientFd == tcpSocket.getSocketFd() || clientFd == udpSocket.getSocketFd()) {
                    continue;
                }
                
                if (FD_ISSET(clientFd, &read_fds)) {
                    Message message;
                    
                    // Get client socket
                    TcpSocket clientSocket(clientFd, {});
                    
                    // Try to receive message
                    if (!clientSocket.receiveMessage(message)) {
                        // Client disconnected or error
                        handleClientDisconnect(clientFd);
                        continue;
                    }
                    
                    // Process the message based on its type
                    switch (message.type) {
                        case MessageType::REGISTER:
                            registerUser(clientFd, message);
                            break;
                        
                        case MessageType::CHAT:
                            handleChatMessage(clientFd, message);
                            break;
                        
                        case MessageType::CODE_SHARE:
                            handleCodeShareMessage(clientFd, message);
                            break;
                        
                        case MessageType::CODE_SUBMIT:
                            processSubmission(clientFd, message);
                            break;
                        
                        case MessageType::DISCONNECT:
                            handleClientDisconnect(clientFd);
                            break;
                        
                        default:
                            std::cerr << "Unknown message type received: " 
                                      << static_cast<int>(message.type) << std::endl;
                            break;
                    }
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in server loop: " << e.what() << std::endl;
        }
    }
}

// Process incoming UDP message
void Server::processUdpMessage() {
    try {
        std::string senderIp;
        int senderPort;
        Message message;
        
        if (udpSocket.receiveMessage(message, senderIp, senderPort)) {
            std::cout << "UDP message received from " << senderIp << ":" << senderPort << std::endl;
            // Handle UDP message if needed (currently just logs the message)
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing UDP message: " << e.what() << std::endl;
    }
}

// Register a new user
void Server::registerUser(int clientFd, const Message& message) {
    // Extract registration info
    RegisterMessage regMsg = MessageHelpers::fromMessage<RegisterMessage>(message);
    std::string username(regMsg.username);
    UserRole role = regMsg.role;
    
    std::cout << "Registration request from user: " << username 
              << ", role: " << (role == UserRole::CODER ? "Coder" : "Navigator") << std::endl;
    
    // Check if username is already taken
    bool success = false;
    std::string responseMsg;
    
    if (isUsernameTaken(username)) {
        responseMsg = "Username already taken";
    } else {
        // Create new user
        auto user = std::make_shared<User>(username, role, clientFd);
        users[username] = user;
        socketToUsername[clientFd] = username;
        unassignedUsers.push_back(user);
        
        success = true;
        responseMsg = "Registration successful";
        
        std::cout << "User registered: " << username << std::endl;
        
        // Try to form teams if possible
        formTeams();
    }
    
    // Send response to client
    Message response = MessageHelpers::createRegisterResponseMessage(success, responseMsg);
    TcpSocket clientSocket(clientFd, {});
    clientSocket.sendMessage(response);
}

// Form teams from unassigned users
void Server::formTeams() {
    // Check for pairs of unassigned users with complementary roles
    while (unassignedUsers.size() >= 2) {
        auto coderIt = std::find_if(unassignedUsers.begin(), unassignedUsers.end(),
                                  [](const std::shared_ptr<User>& user) { 
                                      return user->getRole() == UserRole::CODER; 
                                  });
        
        auto navigatorIt = std::find_if(unassignedUsers.begin(), unassignedUsers.end(),
                                      [](const std::shared_ptr<User>& user) { 
                                          return user->getRole() == UserRole::NAVIGATOR; 
                                      });
        
        // If we found a coder and a navigator, form a team
        if (coderIt != unassignedUsers.end() && navigatorIt != unassignedUsers.end()) {
            auto coder = *coderIt;
            auto navigator = *navigatorIt;
            
            // Generate team name
            std::string teamName = "Team_" + coder->getUsername() + "_" + navigator->getUsername();
            
            // Create new team
            auto team = std::make_shared<Team>(teamName, coder, navigator);
            teams.push_back(team);
            
            std::cout << "Team formed: " << teamName << " (Coder: " << coder->getUsername() 
                      << ", Navigator: " << navigator->getUsername() << ")" << std::endl;
            
            // Remove users from unassigned list
            unassignedUsers.erase(coderIt);
            // Recompute navigatorIt as coderIt erasure invalidates iterators
            navigatorIt = std::find_if(unassignedUsers.begin(), unassignedUsers.end(),
                                     [&navigator](const std::shared_ptr<User>& user) { 
                                         return user->getUsername() == navigator->getUsername(); 
                                     });
            if (navigatorIt != unassignedUsers.end()) {
                unassignedUsers.erase(navigatorIt);
            }
            
            // Notify team members
            Message teamFormedMsg = MessageHelpers::createTeamFormedMessage(
                teamName, coder->getUsername(), navigator->getUsername());
            
            TcpSocket coderSocket(coder->getSocketFd(), {});
            coderSocket.sendMessage(teamFormedMsg);
            
            TcpSocket navigatorSocket(navigator->getSocketFd(), {});
            navigatorSocket.sendMessage(teamFormedMsg);
        } else {
            // No matching pair found
            break;
        }
    }
}

// Start a new problem
void Server::startProblem(const std::string& problemId) {
    Problem problem = Problems::getProblem(problemId);
    if (problem.id == "not_found") {
        std::cerr << "Problem not found: " << problemId << std::endl;
        return;
    }
    
    std::cout << "Starting problem: " << problem.title << std::endl;
    
    // Set current problem
    currentProblemId = problemId;
    
    // Record start time
    problemStartTimes[problemId] = std::chrono::steady_clock::now();
    lastTimeUpdate = std::chrono::steady_clock::now();
    
    // Create problem message
    Message problemMsg = MessageHelpers::createProblemMessage(
        problem.id, 
        problem.title + "\n\n" + problem.description + "\n\nFunction signature:\n" + problem.functionSignature,
        problem.difficulty,
        problem.timeLimit
    );
    
    // Broadcast to all teams
    broadcastMessage(problemMsg);
}

// End current problem
void Server::endProblem() {
    if (currentProblemId.empty()) {
        return;
    }
    
    std::cout << "Ending problem: " << currentProblemId << std::endl;
    
    // Broadcast end problem message
    Message endMsg = MessageHelpers::createEmptyMessage(MessageType::COMPETITION_END);
    broadcastMessage(endMsg);
    
    // Clear current problem
    currentProblemId = "";
    
    // Update scoreboard
    updateScoreboard();
}

// Process submitted code
void Server::processSubmission(int clientFd, const Message& message) {
    if (currentProblemId.empty()) {
        std::cerr << "No active problem. Ignoring submission." << std::endl;
        return;
    }
    
    // Extract submission
    CodeSubmitMessage submitMsg = MessageHelpers::fromMessage<CodeSubmitMessage>(message);
    std::string teamName = submitMsg.team_name;
    std::string problemId = submitMsg.problem_id;
    std::string code = submitMsg.code;
    
    // Verify team name
    auto team = findTeamByName(teamName);
    if (!team) {
        std::cerr << "Unknown team: " << teamName << std::endl;
        return;
    }
    
    // Get user who submitted
    auto user = getUserBySocket(clientFd);
    if (!user) {
        std::cerr << "Unknown user socket: " << clientFd << std::endl;
        return;
    }
    
    // Only navigators can submit code
    if (user->getRole() != UserRole::NAVIGATOR) {
        std::cerr << "Only navigators can submit code. User: " << user->getUsername() << std::endl;
        
        // Send error message to user
        Message errorMsg = MessageHelpers::createRegisterResponseMessage(false, "Only navigators can submit code");
        TcpSocket clientSocket(clientFd, {});
        clientSocket.sendMessage(errorMsg);
        return;
    }
    
    std::cout << "Code submission received from team: " << teamName << " for problem: " << problemId << std::endl;
    
    // Calculate submission time
    auto now = std::chrono::steady_clock::now();
    auto startTime = problemStartTimes[problemId];
    int timeTaken = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    
    // Evaluate code
    bool passed = false;
    int score = 0;
    std::string evalMessage;
    
    if (EvaluationClient::evaluateCode(problemId, code, passed, score, evalMessage)) {
        // Calculate final score based on time taken
        if (passed) {
            score = calculateScore(problemId, timeTaken, true);
            
            // Record score for team
            team->recordScore(problemId, score);
            
            std::cout << "Team " << teamName << " solved problem " << problemId 
                      << " in " << timeTaken << " seconds. Score: " << score << std::endl;
        } else {
            std::cout << "Team " << teamName << " failed to solve problem " << problemId << std::endl;
        }
        
        // Send result to team members
        Message resultMsg = MessageHelpers::createEvaluationResultMessage(
            problemId, passed, score, evalMessage);
        
        sendMessageToTeam(teamName, resultMsg);
        
        // Update scoreboard
        updateScoreboard();
    } else {
        std::cerr << "Failed to evaluate code: " << evalMessage << std::endl;
        
        // Send error message to team
        Message errorMsg = MessageHelpers::createEvaluationResultMessage(
            problemId, false, 0, "Evaluation failed: " + evalMessage);
        
        sendMessageToTeam(teamName, errorMsg);
    }
}

// Broadcast message to all users
void Server::broadcastMessage(const Message& message) {
    // Send via UDP broadcast
    udpSocket.sendBroadcastMessage(message, udpPort);
    
    // Also send via TCP to ensure delivery
    for (const auto& pair : users) {
        TcpSocket clientSocket(pair.second->getSocketFd(), {});
        clientSocket.sendMessage(message);
    }
}

// Send message to a specific user
void Server::sendMessageToUser(const std::string& username, const Message& message) {
    auto it = users.find(username);
    if (it != users.end()) {
        TcpSocket clientSocket(it->second->getSocketFd(), {});
        clientSocket.sendMessage(message);
    }
}

// Send message to a specific team
void Server::sendMessageToTeam(const std::string& teamName, const Message& message) {
    auto team = findTeamByName(teamName);
    if (team) {
        TcpSocket coderSocket(team->getCoderSocketFd(), {});
        coderSocket.sendMessage(message);
        
        TcpSocket navigatorSocket(team->getNavigatorSocketFd(), {});
        navigatorSocket.sendMessage(message);
    }
}

// Handle chat message between team members
void Server::handleChatMessage(int clientFd, const Message& message) {
    // Extract chat message
    ChatMessage chatMsg = MessageHelpers::fromMessage<ChatMessage>(message);
    
    // Get sender
    auto sender = getUserBySocket(clientFd);
    if (!sender) {
        std::cerr << "Unknown sender for chat message" << std::endl;
        return;
    }
    
    std::cout << "Chat message from " << sender->getUsername() << ": " << chatMsg.message << std::endl;
    
    // Find sender's team
    auto team = findTeamByUsername(sender->getUsername());
    if (!team) {
        std::cerr << "User not in a team: " << sender->getUsername() << std::endl;
        return;
    }
    
    // Forward message to partner
    auto partner = team->getPartner(sender->getUsername());
    if (partner) {
        TcpSocket partnerSocket(partner->getSocketFd(), {});
        partnerSocket.sendMessage(message);
    }
}

// Handle code sharing between coder and navigator
void Server::handleCodeShareMessage(int clientFd, const Message& message) {
    // Get sender
    auto sender = getUserBySocket(clientFd);
    if (!sender) {
        std::cerr << "Unknown sender for code share message" << std::endl;
        return;
    }
    
    // Only coders can share code
    if (sender->getRole() != UserRole::CODER) {
        std::cerr << "Only coders can share code. User: " << sender->getUsername() << std::endl;
        return;
    }
    
    std::cout << "Code shared by " << sender->getUsername() << std::endl;
    
    // Find sender's team
    auto team = findTeamByUsername(sender->getUsername());
    if (!team) {
        std::cerr << "User not in a team: " << sender->getUsername() << std::endl;
        return;
    }
    
    // Forward code to navigator
    auto navigator = team->getNavigator();
    if (navigator) {
        TcpSocket navigatorSocket(navigator->getSocketFd(), {});
        navigatorSocket.sendMessage(message);
    }
}

// Calculate score for a submission
int Server::calculateScore(const std::string& problemId, int timeTaken, bool passed) {
    if (!passed) {
        return 0;
    }
    
    Problem problem = Problems::getProblem(problemId);
    int baseScore = problem.baseScore;
    int timeLimit = problem.timeLimit;
    
    // Calculate time bonus
    double timeBonus = 1.0; // Default: no bonus
    
    if (timeTaken <= timeLimit / 2) {
        // Completed in first half of time: 50% bonus
        timeBonus = 1.5;
    } else if (timeTaken <= (timeLimit * 3) / 4) {
        // Completed in first 3/4 of time: 20% bonus
        timeBonus = 1.2;
    }
    
    return static_cast<int>(baseScore * timeBonus);
}

// Update scoreboard
void Server::updateScoreboard() {
    // Sort teams by total score (descending)
    std::vector<std::shared_ptr<Team>> sortedTeams = teams;
    std::sort(sortedTeams.begin(), sortedTeams.end(), 
              [](const std::shared_ptr<Team>& a, const std::shared_ptr<Team>& b) {
                  return a->getTotalScore() > b->getTotalScore();
              });
    
    // Create scoreboard message
    ScoreboardMessage scoreMsg;
    scoreMsg.team_count = std::min(static_cast<int>(sortedTeams.size()), 10);
    
    for (int i = 0; i < scoreMsg.team_count; i++) {
        auto team = sortedTeams[i];
        strncpy(scoreMsg.teams[i].team_name, team->getName().c_str(), MAX_USERNAME_LENGTH - 1);
        scoreMsg.teams[i].team_name[MAX_USERNAME_LENGTH - 1] = '\0';
        scoreMsg.teams[i].total_score = team->getTotalScore();
        scoreMsg.teams[i].problems_solved = team->getProblemsSolved();
    }
    
    // Broadcast scoreboard
    Message message = MessageHelpers::toMessage(MessageType::SCOREBOARD, scoreMsg);
    broadcastMessage(message);
}

// Send time updates to all teams
void Server::sendTimeUpdates() {
    if (currentProblemId.empty()) {
        return;
    }
    
    Problem problem = Problems::getProblem(currentProblemId);
    
    // Calculate time remaining
    auto now = std::chrono::steady_clock::now();
    auto startTime = problemStartTimes[currentProblemId];
    int elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    int remainingSeconds = problem.timeLimit - elapsedSeconds;
    
    // Create time update message
    Message timeMsg = MessageHelpers::createTimeUpdateMessage(currentProblemId, remainingSeconds);
    
    // Broadcast time update
    broadcastMessage(timeMsg);
    
    // Check if time is up
    if (remainingSeconds <= 0) {
        endProblem();
    }
}

// Check and process time updates
void Server::checkTimeUpdates() {
    // Only send updates if there's an active problem
    if (currentProblemId.empty()) {
        return;
    }
    
    // Check if it's time to send an update (every 5 seconds)
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTimeUpdate).count();
    
    if (elapsed >= 5) {
        sendTimeUpdates();
        lastTimeUpdate = now;
    }
}

// Add socket to select monitoring
void Server::addToMonitor(int socketFd) {
    FD_SET(socketFd, &master_fds);
    
    // Update max file descriptor for select()
    if (socketFd > maxfd) {
        maxfd = socketFd;
    }
    
    std::cout << "Added socket " << socketFd << " to monitoring" << std::endl;
}

// Remove socket from select monitoring
void Server::removeFromMonitor(int socketFd) {
    // Check if the socket is valid and in the set before removing
    if (socketFd > 0 && FD_ISSET(socketFd, &master_fds)) {
        FD_CLR(socketFd, &master_fds);
        
        // Recalculate maxfd if this was the highest
        if (socketFd == maxfd) {
            // Find the new highest fd
            maxfd = 0;
            for (const auto& pair : socketToUsername) {
                if (pair.first > 0 && FD_ISSET(pair.first, &master_fds) && pair.first > maxfd) {
                    maxfd = pair.first;
                }
            }
            
            // Also check server sockets
            if (tcpSocket.getSocketFd() > 0 && FD_ISSET(tcpSocket.getSocketFd(), &master_fds) && tcpSocket.getSocketFd() > maxfd) {
                maxfd = tcpSocket.getSocketFd();
            }
            if (udpSocket.getSocketFd() > 0 && FD_ISSET(udpSocket.getSocketFd(), &master_fds) && udpSocket.getSocketFd() > maxfd) {
                maxfd = udpSocket.getSocketFd();
            }
        }
        
        std::cout << "Removed socket " << socketFd << " from monitoring" << std::endl;
    } else {
        std::cerr << "Warning: Attempted to remove invalid socket " << socketFd << " from monitoring" << std::endl;
    }
}

// Find team by name
std::shared_ptr<Team> Server::findTeamByName(const std::string& teamName) {
    for (const auto& team : teams) {
        if (team->getName() == teamName) {
            return team;
        }
    }
    return nullptr;
}

// Find team by username
std::shared_ptr<Team> Server::findTeamByUsername(const std::string& username) {
    for (const auto& team : teams) {
        if (team->hasMember(username)) {
            return team;
        }
    }
    return nullptr;
}

// Handle client disconnection
void Server::handleClientDisconnect(int clientFd) {
    if (clientFd <= 0) {
        std::cerr << "Invalid socket handle in handleClientDisconnect: " << clientFd << std::endl;
        return;
    }

    // Find username for socket
    auto socketIt = socketToUsername.find(clientFd);
    if (socketIt == socketToUsername.end()) {
        std::cerr << "Unknown socket disconnected: " << clientFd << std::endl;
        removeFromMonitor(clientFd);
        close(clientFd);
        return;
    }
    
    std::string username = socketIt->second;
    std::cout << "User disconnected: " << username << std::endl;
    
    // Remove from socket maps before doing other operations to prevent re-entrancy issues
    socketToUsername.erase(socketIt);
    
    // Now handle team-related cleanup
    
    // Find user's team
    auto team = findTeamByUsername(username);
    if (team) {
        // Notify partner
        auto partner = team->getPartner(username);
        if (partner) {
            Message disconnectMsg = MessageHelpers::createRegisterResponseMessage(
                false, "Your partner " + username + " has disconnected");
            
            TcpSocket partnerSocket(partner->getSocketFd(), {});
            partnerSocket.sendMessage(disconnectMsg);
        }
    }
    
    // Remove from unassigned users if present
    auto unassignedIt = std::find_if(unassignedUsers.begin(), unassignedUsers.end(),
                                   [&username](const std::shared_ptr<User>& user) {
                                       return user->getUsername() == username;
                                   });
    
    if (unassignedIt != unassignedUsers.end()) {
        unassignedUsers.erase(unassignedIt);
    }
    
    // Remove from users map
    users.erase(username);
    
    // Remove from monitoring
    removeFromMonitor(clientFd);
    
    // Close socket
    close(clientFd);
}

// Check if username is already taken
bool Server::isUsernameTaken(const std::string& username) {
    return users.find(username) != users.end();
}

// Get user by socket file descriptor
std::shared_ptr<User> Server::getUserBySocket(int socketFd) {
    auto it = socketToUsername.find(socketFd);
    if (it != socketToUsername.end()) {
        return getUserByUsername(it->second);
    }
    return nullptr;
}

// Get user by username
std::shared_ptr<User> Server::getUserByUsername(const std::string& username) {
    auto it = users.find(username);
    if (it != users.end()) {
        return it->second;
    }
    return nullptr;
}

// Start the competition
void Server::startCompetition() {
    std::cout << "Starting competition" << std::endl;
    
    // Broadcast competition start message
    Message startMsg = MessageHelpers::createEmptyMessage(MessageType::COMPETITION_START);
    broadcastMessage(startMsg);
    
    // Get all problems
    auto problems = Problems::getAllProblems();
    
    // Start with the first problem
    if (!problems.empty()) {
        startProblem(problems[0].id);
    }
}

// End the competition
void Server::endCompetition() {
    std::cout << "Competition ended" << std::endl;
    
    // End current problem if any
    endProblem();
    
    // Broadcast competition end message
    Message endMsg = MessageHelpers::createEmptyMessage(MessageType::COMPETITION_END);
    broadcastMessage(endMsg);
    
    // Update final scoreboard
    updateScoreboard();
}

// Repair file descriptors
void Server::repairFileDescriptors() {
    std::cout << "Repairing file descriptors..." << std::endl;
    
    FD_ZERO(&master_fds);
    
    maxfd = 0;
    
    if (tcpSocket.isValid()) {
        int fd = tcpSocket.getSocketFd();
        if (fd > 0) {
            FD_SET(fd, &master_fds);
            maxfd = std::max(maxfd, fd);
            std::cout << "Added TCP server socket " << fd << " to monitoring" << std::endl;
        }
    }
    
    if (udpSocket.isValid()) {
        int fd = udpSocket.getSocketFd();
        if (fd > 0) {
            FD_SET(fd, &master_fds);
            maxfd = std::max(maxfd, fd);
            std::cout << "Added UDP server socket " << fd << " to monitoring" << std::endl;
        }
    }
    
    std::vector<int> invalidSockets;
    
    for (const auto& pair : socketToUsername) {
        int fd = pair.first;
        
        if (fd > 0) {
            int result = fcntl(fd, F_GETFD);
            if (result != -1) {
                FD_SET(fd, &master_fds);
                maxfd = std::max(maxfd, fd);
                std::cout << "Added client socket " << fd << " to monitoring" << std::endl;
            } else {
                std::cerr << "Invalid socket detected: " << fd << " for user " << pair.second << std::endl;
                invalidSockets.push_back(fd);
            }
        } else {
            invalidSockets.push_back(fd);
        }
    }
    
    for (int fd : invalidSockets) {
        handleClientDisconnect(fd);
    }
    
    std::cout << "File descriptor repair complete. maxfd = " << maxfd << std::endl;
} 