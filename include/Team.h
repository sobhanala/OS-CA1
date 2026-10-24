#ifndef TEAM_H
#define TEAM_H

#include "User.h"
#include "Message.h"
#include <memory>
#include <map>
#include <string>

// Class representing a team of coder and navigator
class Team {
private:
    std::string teamName;
    std::shared_ptr<User> coder;
    std::shared_ptr<User> navigator;
    std::map<std::string, int> scores;  // Problem ID -> Score
    int totalScore;
    int problemsSolved;

public:
    // Constructor
    Team(const std::string& teamName, 
         std::shared_ptr<User> coder, 
         std::shared_ptr<User> navigator)
        : teamName(teamName), 
          coder(coder), 
          navigator(navigator),
          totalScore(0),
          problemsSolved(0) {
        
        // Set team name for both members
        coder->setTeamName(teamName);
        navigator->setTeamName(teamName);
    }

    // Get team name
    std::string getName() const {
        return teamName;
    }

    // Get coder
    std::shared_ptr<User> getCoder() const {
        return coder;
    }

    // Get navigator
    std::shared_ptr<User> getNavigator() const {
        return navigator;
    }

    // Get partner for a given user
    std::shared_ptr<User> getPartner(const std::string& username) const {
        if (coder->getUsername() == username) {
            return navigator;
        } else if (navigator->getUsername() == username) {
            return coder;
        }
        return nullptr;
    }

    // Check if a user is a member of this team
    bool hasMember(const std::string& username) const {
        return (coder->getUsername() == username || 
                navigator->getUsername() == username);
    }

    // Get coder's socket file descriptor
    int getCoderSocketFd() const {
        return coder->getSocketFd();
    }

    // Get navigator's socket file descriptor
    int getNavigatorSocketFd() const {
        return navigator->getSocketFd();
    }

    // Record a score for a problem
    void recordScore(const std::string& problemId, int score) {
        scores[problemId] = score;
        totalScore += score;
        problemsSolved++;
    }

    // Get score for a specific problem
    int getScore(const std::string& problemId) const {
        auto it = scores.find(problemId);
        if (it != scores.end()) {
            return it->second;
        }
        return 0;
    }

    // Get total score
    int getTotalScore() const {
        return totalScore;
    }

    // Get number of problems solved
    int getProblemsSolved() const {
        return problemsSolved;
    }
};

#endif // TEAM_H 