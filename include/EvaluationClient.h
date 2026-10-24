#ifndef EVALUATION_CLIENT_H
#define EVALUATION_CLIENT_H

#include "TcpSocket.h"
#include <string>
#include <json/json.h>

// Class for communicating with the evaluation server
class EvaluationClient {
private:
    static constexpr int EVALUATION_PORT = 65432;
    static constexpr const char* EVALUATION_HOST = "127.0.0.1";

    // Serializes the evaluation request to JSON
    static std::string serializeRequest(const std::string& problemId, const std::string& code) {
        Json::Value root;
        root["problem_id"] = problemId;
        root["code"] = code;
        
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, root);
    }
    
    // Deserializes the evaluation response from JSON
    static bool deserializeResponse(const std::string& response, bool& passed, int& score, std::string& message) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream iss(response);
        
        if (!Json::parseFromStream(builder, iss, &root, &errors)) {
            return false;
        }
        
        if (!root.isMember("passed") || !root.isMember("score") || !root.isMember("message")) {
            return false;
        }
        
        passed = root["passed"].asBool();
        score = root["score"].asInt();
        message = root["message"].asString();
        
        return true;
    }

public:
    // Sends code to the evaluation server and returns the result
    static bool evaluateCode(const std::string& problemId, 
                           const std::string& code, 
                           bool& passed,
                           int& score,
                           std::string& message) {
        try {
            // Create TCP socket
            TcpSocket socket;
            
            // Connect to evaluation server
            socket.connect(EVALUATION_HOST, EVALUATION_PORT);
            
            // Create JSON request
            std::string request = serializeRequest(problemId, code);
            
            // Send the request
            socket.send(request.c_str(), request.length());
            
            // Receive the response
            char buffer[4096] = {0};
            int bytesRead = socket.receive(buffer, sizeof(buffer) - 1);
            
            if (bytesRead <= 0) {
                message = "Failed to receive response from evaluation server";
                return false;
            }
            
            // Parse the response
            buffer[bytesRead] = '\0';
            std::string response(buffer);
            
            return deserializeResponse(response, passed, score, message);
            
        } catch (const std::exception& e) {
            message = "Error communicating with evaluation server: ";
            message += e.what();
            return false;
        }
    }
};

#endif // EVALUATION_CLIENT_H 