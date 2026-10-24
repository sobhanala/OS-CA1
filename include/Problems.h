#ifndef PROBLEMS_H
#define PROBLEMS_H

#include <string>
#include <vector>

// Structure representing a programming problem
struct Problem {
    std::string id;               // Unique identifier
    std::string title;            // Problem title
    std::string description;      // Problem description
    std::string functionSignature; // Expected function signature
    int difficulty;               // Difficulty level
    int baseScore;                // Base score for correct solution
    int timeLimit;                // Time limit in seconds
    
    Problem(const std::string& id,
            const std::string& title,
            const std::string& description,
            const std::string& functionSignature,
            int difficulty,
            int baseScore,
            int timeLimit)
        : id(id), title(title), description(description), 
          functionSignature(functionSignature), difficulty(difficulty),
          baseScore(baseScore), timeLimit(timeLimit) {}
};

// Class containing all competition problems
class Problems {
public:
    static std::vector<Problem> getAllProblems() {
        std::vector<Problem> problems;
        
        // Problem 1: Add two numbers
        problems.push_back(Problem(
            "add_numbers",
            "Add Two Numbers",
            "Write a function that takes two numbers as input and returns their sum.",
            "def add_numbers(a, b):",
            1, // Difficulty
            1, // Base score
            60  // Time limit (seconds)
        ));
        
        // Problem 2: Reverse a string
        problems.push_back(Problem(
            "reverse_string",
            "Reverse a String",
            "Write a function that takes a string as input and returns it reversed.",
            "def reverse_string(s):",
            2, // Difficulty
            3, // Base score
            60  // Time limit (seconds)
        ));
        
        // Problem 3: Check if a string is a palindrome
        problems.push_back(Problem(
            "is_palindrome",
            "Palindrome Check",
            "Write a function that checks if a given string is a palindrome (reads the same backward as forward).",
            "def is_palindrome(s):",
            3, // Difficulty
            5, // Base score
            60  // Time limit (seconds)
        ));
        
        return problems;
    }
    
    // Get a specific problem by ID
    static Problem getProblem(const std::string& id) {
        auto problems = getAllProblems();
        for (const auto& problem : problems) {
            if (problem.id == id) {
                return problem;
            }
        }
        
        // Return a default "not found" problem if ID doesn't exist
        return Problem(
            "not_found",
            "Problem Not Found",
            "The requested problem was not found.",
            "",
            0,
            0,
            0
        );
    }
};

#endif // PROBLEMS_H 