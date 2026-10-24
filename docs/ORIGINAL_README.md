# Team Programming Competition Platform

A client-server platform for team programming competitions, focusing on socket programming and system calls in Linux.

## Overview

This platform enables team-based programming competitions where pairs of programmers (a coder and a navigator) collaborate to solve programming problems. The system uses TCP for reliable connections and UDP for broadcasts, implementing both unicast and broadcast communication methods.

## Features

- Real-time team formation between coders and navigators
- Distribution of programming problems to teams
- Team collaboration through private chat
- Code sharing between team members
- Code submission and automated evaluation
- Real-time scoreboard and time tracking
- Error handling and connection management

## Architecture

The system follows a client-server architecture:

- **Server**: Central coordinator that manages team formation, distributes problems, processes submissions, and maintains the scoreboard
- **Client**: Interface for users (coders and navigators) to communicate with their teammates and interact with the server
- **Evaluation Server**: Python service that tests submitted code against predefined test cases

## Technical Implementation Details

- **Single-Threaded Event-Based Design**: The platform uses select() for event-driven I/O multiplexing instead of multi-threading, focusing on Linux system calls and socket programming fundamentals.
- **TCP for Reliable Communication**: Used for registration, code sharing, chat messages, and code submission.
- **UDP for Broadcast Messages**: Used for problem distribution, time updates, and scoreboard announcements.
- **Non-Blocking I/O**: Both server and client use non-blocking sockets for efficient event processing.
- **Efficient File Descriptor Management**: The select() system call monitors multiple socket file descriptors for activity.

## Requirements

- Linux-based operating system
- C++17 compatible compiler (g++)
- Python 3.6 or higher (for evaluation server)
- jsoncpp library (`sudo apt-get install libjsoncpp-dev`)

## Building the Project

### Using Makefile

```bash
make
```

This will generate two executable files: `server` and `client`.

For other available targets:
```bash
make help
```

### Using CMake

```bash
mkdir build
cd build
cmake ..
make
```

## Running the Platform

### 1. Start the Evaluation Server

```bash
make run-eval-server
# or
python3 src/evaluation_server.py
```

The evaluation server runs on localhost port 65432.

### 2. Start the Server

```bash
./server <port>
```

Example:
```bash
./server 8000
```

### 3. Start the Clients

For each participant, run:

```bash
./client <username> <server_ip> <port> <role>
```

Example for a coder:
```bash
./client alice 127.0.0.1 8000 coder
```

Example for a navigator:
```bash
./client bob 127.0.0.1 8000 navigator
```

## Client Commands

Once connected, clients can use the following commands:

- `chat <message>` - Send a chat message to your teammate
- `code` - Enter code to share (coder) or submit (navigator)
- `help` - Display available commands
- `exit` or `quit` - Exit the program

## Competition Workflow

1. Coders and navigators connect to the server
2. Server automatically forms teams and notifies team members
3. Server starts the competition and sends the first problem to all teams
4. Coders write the solution and share with navigators
5. Navigators review the code and submit for evaluation
6. Evaluation results and scores are sent back to the team
7. Scoreboard is updated and broadcast to all participants
8. Next problem begins after time expires or all teams submit

## Problems

The competition includes three Python programming problems:

1. Add Two Numbers (Difficulty 1, 1 point)
2. Reverse a String (Difficulty 2, 3 points)
3. Palindrome Check (Difficulty 3, 5 points)

Time bonuses are awarded for quick solutions:
- 50% bonus for solutions in the first half of the time limit
- 20% bonus for solutions in the first 3/4 of the time limit

## Implementation Details

- **Socket Programming**: Direct use of system calls for network communication
- **Event-Based Architecture**: Using select() for I/O multiplexing
- **Non-Blocking I/O**: For responsive user interaction
- **Linux System Calls**: Focus on POSIX-compliant socket APIs
- **JSON-based communication** with the evaluation server
- **Custom message protocol** for client-server communication

## System Calls Used

- **socket()**: Create socket endpoints
- **bind()**: Bind a socket to an address
- **listen()**: Listen for connections on a socket
- **accept()**: Accept a connection on a socket
- **connect()**: Connect to a remote socket
- **send()/recv()**: Send/receive data on a connected socket
- **sendto()/recvfrom()**: Send/receive data on a connectionless socket
- **select()**: Monitor multiple file descriptors for I/O availability
- **fcntl()**: Manipulate file descriptor properties (for non-blocking I/O)
- **setsockopt()**: Set socket options
- **close()**: Close a file descriptor

## Project Structure

```
.
├── include/               # Header files
│   ├── Client.h           # Client class
│   ├── EvaluationClient.h # Evaluation client
│   ├── Message.h          # Message structures
│   ├── MessageHelpers.h   # Message utilities
│   ├── Problems.h         # Problem definitions
│   ├── Server.h           # Server class
│   ├── Socket.h           # Base socket class
│   ├── Team.h             # Team class 
│   ├── TcpSocket.h        # TCP socket implementation
│   ├── UdpSocket.h        # UDP socket implementation
│   └── User.h             # User class
├── src/                   # Source files
│   ├── client/            # Client implementation
│   │   ├── Client.cpp     # Client class
│   │   └── main_client.cpp # Client entry point
│   ├── server/            # Server implementation
│   │   ├── Server.cpp     # Server class
│   │   └── main_server.cpp # Server entry point
│   └── evaluation_server.py # Python evaluation server
├── CMakeLists.txt         # CMake build configuration
├── Makefile               # Makefile build configuration
└── README.md              # Project documentation
```

## Error Handling

The platform includes comprehensive error handling:
- Connection failures
- Invalid usernames
- Message transmission errors 
- Code execution failures
- Unexpected disconnections

## Authors

This project was created as part of the Operating Systems course programming assignment.

## License

This project is for educational purposes only.
