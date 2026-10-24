#ifndef MESSAGE_HELPERS_H
#define MESSAGE_HELPERS_H

#include "Message.h"
#include <cstring>
#include <iostream>
#include <algorithm>

// Helper class for converting between Message and specific message structures
class MessageHelpers {
public:
    // Convert a specific message to the generic Message structure
    template<typename T>
    static Message toMessage(MessageType type, const T& data) {
        Message message(type);
        
        size_t dataSize = sizeof(T);
        
        if (dataSize <= MAX_MESSAGE_LENGTH) {
            memcpy(message.data, &data, dataSize);
        } else {
            std::cerr << "ERROR: Message data size (" << dataSize 
                      << " bytes) exceeds buffer size (" << MAX_MESSAGE_LENGTH 
                      << " bytes) for message type " << static_cast<int>(type)
                      << std::endl;
                      
            memcpy(message.data, &data, MAX_MESSAGE_LENGTH);
        }
        
        return message;
    }

    // Extract a specific message structure from a generic Message
    template<typename T>
    static T fromMessage(const Message& message) {
        T data;
        size_t copySize = std::min(sizeof(T), static_cast<size_t>(MAX_MESSAGE_LENGTH));
        memcpy(&data, message.data, copySize);
        return data;
    }

    // Create a registration message
    static Message createRegisterMessage(const std::string& username, UserRole role) {
        RegisterMessage regMsg;
        strncpy(regMsg.username, username.c_str(), MAX_USERNAME_LENGTH - 1);
        regMsg.username[MAX_USERNAME_LENGTH - 1] = '\0';
        regMsg.role = role;
        return toMessage(MessageType::REGISTER, regMsg);
    }

    // Create a registration response message
    static Message createRegisterResponseMessage(bool success, const std::string& message) {
        RegisterResponseMessage response;
        response.success = success;
        strncpy(response.message, message.c_str(), MAX_MESSAGE_FIELD_LENGTH - 1);
        response.message[MAX_MESSAGE_FIELD_LENGTH - 1] = '\0';
        return toMessage(MessageType::REGISTER_RESPONSE, response);
    }

    // Create a team formed message
    static Message createTeamFormedMessage(const std::string& teamName, 
                                         const std::string& coderName, 
                                         const std::string& navigatorName) {
        TeamFormedMessage teamMsg;
        strncpy(teamMsg.team_name, teamName.c_str(), MAX_USERNAME_LENGTH - 1);
        teamMsg.team_name[MAX_USERNAME_LENGTH - 1] = '\0';
        
        strncpy(teamMsg.coder_name, coderName.c_str(), MAX_USERNAME_LENGTH - 1);
        teamMsg.coder_name[MAX_USERNAME_LENGTH - 1] = '\0';
        
        strncpy(teamMsg.navigator_name, navigatorName.c_str(), MAX_USERNAME_LENGTH - 1);
        teamMsg.navigator_name[MAX_USERNAME_LENGTH - 1] = '\0';
        
        return toMessage(MessageType::TEAM_FORMED, teamMsg);
    }

    // Create a problem message
    static Message createProblemMessage(const std::string& problemId, 
                                      const std::string& description,
                                      int difficulty,
                                      int timeLimitSeconds) {
        ProblemMessage probMsg;
        strncpy(probMsg.problem_id, problemId.c_str(), MAX_PROBLEM_ID_LENGTH - 1);
        probMsg.problem_id[MAX_PROBLEM_ID_LENGTH - 1] = '\0';
        
        strncpy(probMsg.description, description.c_str(), MAX_MESSAGE_FIELD_LENGTH - 1);
        probMsg.description[MAX_MESSAGE_FIELD_LENGTH - 1] = '\0';
        
        probMsg.difficulty = difficulty;
        probMsg.time_limit_seconds = timeLimitSeconds;
        
        return toMessage(MessageType::PROBLEM, probMsg);
    }

    // Create a time update message
    static Message createTimeUpdateMessage(const std::string& problemId, int secondsRemaining) {
        TimeUpdateMessage timeMsg;
        strncpy(timeMsg.problem_id, problemId.c_str(), MAX_PROBLEM_ID_LENGTH - 1);
        timeMsg.problem_id[MAX_PROBLEM_ID_LENGTH - 1] = '\0';
        
        timeMsg.seconds_remaining = secondsRemaining;
        
        return toMessage(MessageType::TIME_UPDATE, timeMsg);
    }

    // Create a chat message
    static Message createChatMessage(const std::string& sender, const std::string& message) {
        ChatMessage chatMsg;
        strncpy(chatMsg.sender, sender.c_str(), MAX_USERNAME_LENGTH - 1);
        chatMsg.sender[MAX_USERNAME_LENGTH - 1] = '\0';
        
        strncpy(chatMsg.message, message.c_str(), MAX_CHAT_LENGTH - 1);
        chatMsg.message[MAX_CHAT_LENGTH - 1] = '\0';
        
        return toMessage(MessageType::CHAT, chatMsg);
    }

    // Create a code share message
    static Message createCodeShareMessage(const std::string& code) {
        CodeShareMessage codeMsg;
        strncpy(codeMsg.code, code.c_str(), MAX_CODE_LENGTH - 1);
        codeMsg.code[MAX_CODE_LENGTH - 1] = '\0';
        
        return toMessage(MessageType::CODE_SHARE, codeMsg);
    }

    // Create a code submit message
    static Message createCodeSubmitMessage(const std::string& teamName, 
                                         const std::string& problemId, 
                                         const std::string& code,
                                         int submissionTime) {
        CodeSubmitMessage submitMsg;
        strncpy(submitMsg.team_name, teamName.c_str(), MAX_USERNAME_LENGTH - 1);
        submitMsg.team_name[MAX_USERNAME_LENGTH - 1] = '\0';
        
        strncpy(submitMsg.problem_id, problemId.c_str(), MAX_PROBLEM_ID_LENGTH - 1);
        submitMsg.problem_id[MAX_PROBLEM_ID_LENGTH - 1] = '\0';
        
        strncpy(submitMsg.code, code.c_str(), MAX_CODE_LENGTH - 1);
        submitMsg.code[MAX_CODE_LENGTH - 1] = '\0';
        
        submitMsg.submission_time = submissionTime;
        
        return toMessage(MessageType::CODE_SUBMIT, submitMsg);
    }

    // Create an evaluation result message
    static Message createEvaluationResultMessage(const std::string& problemId, 
                                               bool passed,
                                               int score,
                                               const std::string& message) {
        EvaluationResultMessage resultMsg;
        strncpy(resultMsg.problem_id, problemId.c_str(), MAX_PROBLEM_ID_LENGTH - 1);
        resultMsg.problem_id[MAX_PROBLEM_ID_LENGTH - 1] = '\0';
        
        resultMsg.passed = passed;
        resultMsg.score = score;
        
        strncpy(resultMsg.message, message.c_str(), MAX_MESSAGE_FIELD_LENGTH - 1);
        resultMsg.message[MAX_MESSAGE_FIELD_LENGTH - 1] = '\0';
        
        return toMessage(MessageType::EVALUATION_RESULT, resultMsg);
    }

    // Create an empty message of specific type (for simple notifications)
    static Message createEmptyMessage(MessageType type) {
        return Message(type);
    }
};

#endif // MESSAGE_HELPERS_H 