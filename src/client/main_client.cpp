#include "../../include/Client.h"
#include "../../include/Message.h"
#include "../../include/MessageHelpers.h"
#include <iostream>
#include <string>
#include <csignal>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

// Global client instance for signal handling
Client* g_client = nullptr;
bool g_running = true;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down client..." << std::endl;
        g_running = false;
        if (g_client) {
            g_client->disconnect();
        }
    }
}

// Print usage information
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <username> <server_ip> <port> <role>\n"
              << "  <username>: User's name for registration\n"
              << "  <server_ip>: Server IP address\n"
              << "  <port>: Server port number\n"
              << "  <role>: User role (coder or navigator)\n"
              << "Example: " << programName << " alice 127.0.0.1 8000 coder" << std::endl;
}

// Parse role from string
UserRole parseRole(const std::string& roleStr) {
    if (roleStr == "coder") {
        return UserRole::CODER;
    } else if (roleStr == "navigator") {
        return UserRole::NAVIGATOR;
    }
    
    throw std::invalid_argument("Role must be 'coder' or 'navigator'");
}

// Message handler callbacks
void handleProblem(const Message& message) {
    ProblemMessage probMsg = MessageHelpers::fromMessage<ProblemMessage>(message);
    
    std::cout << "\n===== NEW PROBLEM =====\n"
              << "Problem ID: " << probMsg.problem_id << "\n"
              << "Description: " << probMsg.description << "\n"
              << "Difficulty: " << probMsg.difficulty << "\n"
              << "Time Limit: " << probMsg.time_limit_seconds << " seconds\n"
              << "=======================\n" << std::endl;
}

void handleTimeUpdate(const Message& message) {
    TimeUpdateMessage timeMsg = MessageHelpers::fromMessage<TimeUpdateMessage>(message);
    
    std::cout << "Time remaining for problem " << timeMsg.problem_id 
              << ": " << timeMsg.seconds_remaining << " seconds" << std::endl;
}

void handleChat(const Message& message) {
    ChatMessage chatMsg = MessageHelpers::fromMessage<ChatMessage>(message);
    
    std::cout << "[" << chatMsg.sender << "]: " << chatMsg.message << std::endl;
}

void handleCodeShare(const Message& message) {
    CodeShareMessage codeMsg = MessageHelpers::fromMessage<CodeShareMessage>(message);
    
    std::cout << "\n===== RECEIVED CODE =====\n"
              << codeMsg.code << "\n"
              << "=========================\n" << std::endl;
}

void handleEvaluationResult(const Message& message) {
    EvaluationResultMessage resultMsg = MessageHelpers::fromMessage<EvaluationResultMessage>(message);
    
    std::cout << "\n===== EVALUATION RESULT =====\n"
              << "Problem: " << resultMsg.problem_id << "\n"
              << "Result: " << (resultMsg.passed ? "PASSED" : "FAILED") << "\n"
              << "Score: " << resultMsg.score << "\n"
              << "Message: " << resultMsg.message << "\n"
              << "==============================\n" << std::endl;
}

void handleScoreboard(const Message& message) {
    ScoreboardMessage scoreMsg = MessageHelpers::fromMessage<ScoreboardMessage>(message);
    
    std::cout << "\n===== SCOREBOARD =====\n";
    for (int i = 0; i < scoreMsg.team_count; i++) {
        std::cout << (i + 1) << ". " << scoreMsg.teams[i].team_name
                  << " - Score: " << scoreMsg.teams[i].total_score
                  << " - Problems Solved: " << scoreMsg.teams[i].problems_solved << "\n";
    }
    std::cout << "=====================\n" << std::endl;
}

// Make stdin non-blocking
void makeStdinNonBlocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

// Process user input
bool processUserInput(Client& client) {
    static std::string inputBuffer;
    static bool codeInputMode = false;
    static std::string codeBuffer;
    static std::string problemId;
    
    char buffer[256];
    ssize_t nread = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    
    if (nread <= 0) {
        return false;  // No input available or error
    }
    
    buffer[nread] = '\0';
    
    // Process each line in the buffer
    for (int i = 0; i < nread; i++) {
        char c = buffer[i];
        
        if (c == '\n') {
            // Process the complete line
            if (!codeInputMode) {
                // Normal command mode
                if (inputBuffer == "exit" || inputBuffer == "quit") {
                    g_running = false;
                    return true;
                } else if (inputBuffer.substr(0, 5) == "chat ") {
                    std::string message = inputBuffer.substr(5);
                    client.sendChatMessage(message);
                } else if (inputBuffer == "code") {
                    std::cout << "Enter code (type 'END' on a new line to finish):" << std::endl;
                    codeInputMode = true;
                    codeBuffer.clear();
                } else if (inputBuffer == "help") {
                    std::cout << "Available commands:\n"
                              << "  chat <message> - Send chat message to your teammate\n"
                              << "  code - Share/submit code\n"
                              << "  help - Show this help\n"
                              << "  exit/quit - Exit the program\n" << std::endl;
                } else if (!inputBuffer.empty()) {
                    std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
                }
            } else {
                // Code input mode
                if (inputBuffer == "END") {
                    codeInputMode = false;
                    
                    if (client.getUserRole() == UserRole::CODER) {
                        client.shareCode(codeBuffer);
                        std::cout << "Code shared with navigator." << std::endl;
                    } else {
                        std::cout << "Enter problem ID: ";
                        // Switch to problem ID input mode
                        codeInputMode = false;
                        problemId.clear();
                        // Wait for problem ID input
                        std::string pidInput;
                        std::getline(std::cin, pidInput);
                        problemId = pidInput;
                        
                        client.submitCode(problemId, codeBuffer);
                        std::cout << "Code submitted for evaluation." << std::endl;
                    }
                } else {
                    // Add line to code buffer
                    codeBuffer += inputBuffer + "\n";
                }
            }
            
            // Reset input buffer
            inputBuffer.clear();
        } else {
            // Add character to input buffer
            inputBuffer += c;
        }
    }
    
    return true;
}

// Main function
int main(int argc, char* argv[]) {
    // Check command-line arguments
    if (argc != 5) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string username = argv[1];
    std::string serverIp = argv[2];
    
    // Parse port number
    int port;
    try {
        port = std::stoi(argv[3]);
        if (port <= 0 || port > 65535) {
            throw std::out_of_range("Port must be between 1 and 65535");
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid port number: " << e.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse role
    UserRole role;
    try {
        role = parseRole(argv[4]);
    } catch (const std::exception& e) {
        std::cerr << "Invalid role: " << e.what() << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // Register signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        // Create and connect client
        Client client(username, role, serverIp, port, port);
        g_client = &client;
        
        // Register message handlers
        client.registerCallback(MessageType::PROBLEM, handleProblem);
        client.registerCallback(MessageType::TIME_UPDATE, handleTimeUpdate);
        client.registerCallback(MessageType::CHAT, handleChat);
        client.registerCallback(MessageType::CODE_SHARE, handleCodeShare);
        client.registerCallback(MessageType::EVALUATION_RESULT, handleEvaluationResult);
        client.registerCallback(MessageType::SCOREBOARD, handleScoreboard);
        
        std::cout << "Connecting to server " << serverIp << ":" << port << "..." << std::endl;
        if (!client.connect()) {
            std::cerr << "Failed to connect to server." << std::endl;
            return 1;
        }
        
        std::cout << "Connected as " << username << " (" << (role == UserRole::CODER ? "Coder" : "Navigator") << ")" << std::endl;
        std::cout << "Waiting to be matched with a teammate..." << std::endl;
        std::cout << "Type 'help' for available commands." << std::endl;
        
        // Make stdin non-blocking for input processing
        makeStdinNonBlocking();
        
        // Main event loop
        while (g_running) {
            // Process network events
            client.processEvents(100);  // 100ms timeout
            
            // Process user input (non-blocking)
            processUserInput(client);
            
            // Process messages in queue
            Message message;
            while (client.getNextMessage(message)) {
                // Messages are already processed by callbacks, nothing to do here
            }
            
            // Short sleep to reduce CPU usage
            usleep(10000);  // 10ms
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    g_client = nullptr;
    std::cout << "Client terminated." << std::endl;
    return 0;
} 