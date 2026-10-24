#ifndef USER_H
#define USER_H

#include "Message.h"
#include <string>

// Base class for all users (coder and navigator)
class User {
protected:
    std::string username;
    UserRole role;
    std::string teamName;
    int socketFd;
    bool inTeam;

public:
    // Constructor
    User(const std::string& username, UserRole role, int socketFd)
        : username(username), role(role), socketFd(socketFd), inTeam(false) {}

    // Virtual destructor
    virtual ~User() {}

    // Get username
    std::string getUsername() const {
        return username;
    }

    // Get user role
    UserRole getRole() const {
        return role;
    }

    // Get socket file descriptor
    int getSocketFd() const {
        return socketFd;
    }

    // Set team name
    void setTeamName(const std::string& name) {
        teamName = name;
        inTeam = true;
    }

    // Get team name
    std::string getTeamName() const {
        return teamName;
    }

    // Check if user is in a team
    bool isInTeam() const {
        return inTeam;
    }

    // Check if user is a coder
    bool isCoder() const {
        return role == UserRole::CODER;
    }

    // Check if user is a navigator
    bool isNavigator() const {
        return role == UserRole::NAVIGATOR;
    }

    // String representation of role
    std::string getRoleString() const {
        return (role == UserRole::CODER) ? "Coder" : "Navigator";
    }
};

#endif // USER_H 