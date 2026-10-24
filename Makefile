CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -pedantic
LDFLAGS = -ljsoncpp

INCLUDE_DIR = include
SERVER_SRC_DIR = src/server
CLIENT_SRC_DIR = src/client

SERVER_SRC = $(SERVER_SRC_DIR)/Server.cpp $(SERVER_SRC_DIR)/main_server.cpp
CLIENT_SRC = $(CLIENT_SRC_DIR)/Client.cpp $(CLIENT_SRC_DIR)/main_client.cpp

SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

SERVER_TARGET = server
CLIENT_TARGET = client

# Check if we're on Linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    # Linux-specific flags and libs
    CXXFLAGS += -D_LINUX
else
    # Other OS (likely not supported)
    $(warning This Makefile is optimized for Linux systems)
endif

.PHONY: all clean help run-eval-server

all: $(SERVER_TARGET) $(CLIENT_TARGET)

help:
	@echo "Available targets:"
	@echo "  all               - Build server and client"
	@echo "  server            - Build only the server"
	@echo "  client            - Build only the client"
	@echo "  run-eval-server   - Run the evaluation server"
	@echo "  clean             - Remove all built files"
	@echo "  help              - Show this help message"

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -o $@ $^ $(LDFLAGS)
	@echo "Server built successfully"

$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -o $@ $^ $(LDFLAGS)
	@echo "Client built successfully"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_TARGET) $(CLIENT_TARGET)
	@echo "Cleaned build files"

run-eval-server:
	chmod +x src/evaluation_server.py
	python3 src/evaluation_server.py 