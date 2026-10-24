#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstring>

// Maximum message sizes
constexpr int MAX_USERNAME_LENGTH = 50;
constexpr int MAX_PROBLEM_ID_LENGTH = 20;
constexpr int MAX_CODE_LENGTH = 2000;
constexpr int MAX_CHAT_LENGTH = 1000;
constexpr int MAX_MESSAGE_FIELD_LENGTH = 4000;
constexpr int MAX_MESSAGE_LENGTH = 16384;

// Message types
enum class MessageType {
    REGISTER,               // User registration
    REGISTER_RESPONSE,      // Registration response
    TEAM_FORMED,            // Team formation announcement
    PROBLEM,                // Problem delivery
    TIME_UPDATE,            // Remaining time update
    CHAT,                   // Chat message between team members
    CODE_SHARE,             // Code sharing from coder to navigator
    CODE_SUBMIT,            // Code submission to server
    EVALUATION_RESULT,      // Evaluation result
    COMPETITION_START,      // Competition start
    COMPETITION_END,        // Competition end
    SCOREBOARD,             // Score announcement
    DISCONNECT,             // Disconnection
    ERROR                   // Error message
};

// User roles
enum class UserRole {
    CODER,
    NAVIGATOR
};

// Main message structure
struct Message {
    MessageType type;
    char data[MAX_MESSAGE_LENGTH];

    Message() : type(MessageType::ERROR) {
        memset(data, 0, MAX_MESSAGE_LENGTH);
    }

    Message(MessageType type) : type(type) {
        memset(data, 0, MAX_MESSAGE_LENGTH);
    }
};

// User registration structure
struct RegisterMessage {
    char username[MAX_USERNAME_LENGTH];
    UserRole role;
};

// Registration response structure
struct RegisterResponseMessage {
    bool success;
    char message[MAX_MESSAGE_FIELD_LENGTH];
};

// Team formation announcement structure
struct TeamFormedMessage {
    char team_name[MAX_USERNAME_LENGTH];
    char coder_name[MAX_USERNAME_LENGTH];
    char navigator_name[MAX_USERNAME_LENGTH];
};

// Problem structure
struct ProblemMessage {
    char problem_id[MAX_PROBLEM_ID_LENGTH];
    char description[MAX_MESSAGE_FIELD_LENGTH];
    int difficulty;
    int time_limit_seconds;
};

// Time update structure
struct TimeUpdateMessage {
    char problem_id[MAX_PROBLEM_ID_LENGTH];
    int seconds_remaining;
};

// Chat message structure
struct ChatMessage {
    char sender[MAX_USERNAME_LENGTH];
    char message[MAX_CHAT_LENGTH];
};

// Code sharing structure
struct CodeShareMessage {
    char code[MAX_CODE_LENGTH];
};

// Code submission structure
struct CodeSubmitMessage {
    char team_name[MAX_USERNAME_LENGTH];
    char problem_id[MAX_PROBLEM_ID_LENGTH];
    char code[MAX_CODE_LENGTH];
    int submission_time; // Time of submission from problem start (seconds)
};

// Evaluation result structure
struct EvaluationResultMessage {
    char problem_id[MAX_PROBLEM_ID_LENGTH];
    bool passed;
    int score;
    char message[MAX_MESSAGE_FIELD_LENGTH];
};

// Scoreboard structure
struct ScoreboardMessage {
    struct TeamScore {
        char team_name[MAX_USERNAME_LENGTH];
        int total_score;
        int problems_solved;
    };
    int team_count;
    TeamScore teams[10]; // We assume a maximum of 10 teams
};

#endif // MESSAGE_H 