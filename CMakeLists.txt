cmake_minimum_required(VERSION 3.10)
project(OS_CA1 VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
include_directories(${JSONCPP_INCLUDE_DIRS})

# Add include directory
include_directories(include)

# Add executable for server
add_executable(server
    src/server/main_server.cpp
    src/server/Server.cpp
)

# Add executable for client
add_executable(client
    src/client/main_client.cpp
    src/client/Client.cpp
)

# Link libraries
target_link_libraries(server ${JSONCPP_LIBRARIES})
target_link_libraries(client ${JSONCPP_LIBRARIES})

# Add install targets
install(TARGETS server client
    RUNTIME DESTINATION bin
)

# Make evaluation server executable
add_custom_command(
    TARGET server POST_BUILD
    COMMAND chmod +x ${CMAKE_SOURCE_DIR}/src/evaluation_server.py
    COMMENT "Making evaluation server executable"
)

# Add custom target to run evaluation server
add_custom_target(run_eval_server
    COMMAND ${CMAKE_SOURCE_DIR}/src/evaluation_server.py
    DEPENDS server
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Compiler warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif() 