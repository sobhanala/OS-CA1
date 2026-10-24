#!/usr/bin/env python3

import socket
import json
import sys
import traceback
import select
import time

# Evaluation server configuration
HOST = '127.0.0.1'
PORT = 65432
MAX_CONNECTIONS = 10

# Test cases for each problem
TEST_CASES = {
    "add_numbers": [
        {"inputs": [1, 2], "expected": 3},
        {"inputs": [0, 0], "expected": 0},
        {"inputs": [-1, 1], "expected": 0},
        {"inputs": [100, 200], "expected": 300},
        {"inputs": [-50, -25], "expected": -75}
    ],
    "reverse_string": [
        {"inputs": ["hello"], "expected": "olleh"},
        {"inputs": ["python"], "expected": "nohtyp"},
        {"inputs": [""], "expected": ""},
        {"inputs": ["a"], "expected": "a"},
        {"inputs": ["12345"], "expected": "54321"}
    ],
    "is_palindrome": [
        {"inputs": ["radar"], "expected": True},
        {"inputs": ["level"], "expected": True},
        {"inputs": ["hello"], "expected": False},
        {"inputs": [""], "expected": True},
        {"inputs": ["a"], "expected": True},
        {"inputs": ["ab"], "expected": False}
    ]
}

# Score mappings for each problem
SCORE_MAPPING = {
    "add_numbers": 1,
    "reverse_string": 3,
    "is_palindrome": 5
}

def evaluate_code(problem_id, code):
    """
    Evaluate submitted code using test cases for the given problem.
    Returns a tuple of (passed, score, message).
    """
    if problem_id not in TEST_CASES:
        return False, 0, f"Unknown problem ID: {problem_id}"
    
    # Check code length limits
    if len(code) > 2000:
        return False, 0, "Code exceeds maximum length of 2000 characters"
    
    # Prepare the environment
    namespace = {}
    
    # Run the code
    try:
        exec(code, namespace)
    except Exception as e:
        return False, 0, f"Code execution error: {str(e)}"
    
    # Check that the function exists
    function_name = problem_id
    if function_name not in namespace:
        return False, 0, f"Function '{function_name}' not found in submitted code"
    
    # Run test cases
    test_cases = TEST_CASES[problem_id]
    passed_count = 0
    failed_cases = []
    
    for i, case in enumerate(test_cases):
        try:
            # Get function from namespace
            func = namespace[function_name]
            
            # Call function with inputs
            result = func(*case["inputs"])
            
            # Check result
            if result == case["expected"]:
                passed_count += 1
            else:
                failed_cases.append({
                    "case": i + 1,
                    "inputs": case["inputs"],
                    "expected": case["expected"],
                    "got": result
                })
        except Exception as e:
            # Exception during test case execution
            failed_cases.append({
                "case": i + 1,
                "inputs": case["inputs"],
                "error": str(e)
            })
    
    # Determine if all tests passed
    all_passed = (passed_count == len(test_cases))
    
    # Calculate score
    score = SCORE_MAPPING[problem_id] if all_passed else 0
    
    # Create message
    if all_passed:
        message = f"All test cases passed! ({passed_count}/{len(test_cases)})"
    else:
        message = f"Failed {len(failed_cases)}/{len(test_cases)} test cases.\n"
        for i, case in enumerate(failed_cases):
            if "error" in case:
                message += f"Case {case['case']}: Error - {case['error']}\n"
            else:
                message += f"Case {case['case']}: Expected {case['expected']}, got {case['got']}\n"
    
    return all_passed, score, message

def handle_client_data(client_socket, addr, data):
    """Process received client data."""
    try:
        print(f"[{addr}] Received data: {data[:100]}...")
        
        # Parse JSON request
        try:
            request = json.loads(data)
        except json.JSONDecodeError:
            response = {
                "passed": False,
                "score": 0,
                "message": "Invalid JSON format"
            }
            client_socket.sendall(json.dumps(response).encode('utf-8'))
            return
        
        # Extract problem ID and code
        if "problem_id" not in request or "code" not in request:
            response = {
                "passed": False,
                "score": 0,
                "message": "Missing required fields: problem_id, code"
            }
            client_socket.sendall(json.dumps(response).encode('utf-8'))
            return
        
        problem_id = request["problem_id"]
        code = request["code"]
        
        print(f"[{addr}] Evaluating code for problem: {problem_id}")
        
        # Evaluate code
        passed, score, message = evaluate_code(problem_id, code)
        
        # Send response
        response = {
            "passed": passed,
            "score": score,
            "message": message
        }
        
        print(f"[{addr}] Evaluation result: {'PASSED' if passed else 'FAILED'}, Score: {score}")
        
        client_socket.sendall(json.dumps(response).encode('utf-8'))
        
    except Exception as e:
        print(f"[{addr}] Error: {e}")
        traceback.print_exc()

def main():
    """Main function to start the evaluation server."""
    # Create server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # Make socket non-blocking
    server_socket.setblocking(0)
    
    try:
        # Bind to address and start listening
        server_socket.bind((HOST, PORT))
        server_socket.listen(MAX_CONNECTIONS)
        
        print(f"Evaluation server running on {HOST}:{PORT}")
        
        # Socket lists for select
        inputs = [server_socket]
        outputs = []
        
        # Client data buffers
        client_data = {}
        
        while True:
            # Wait for at least one socket to be ready
            readable, writable, exceptional = select.select(inputs, outputs, inputs)
            
            # Handle readable sockets
            for sock in readable:
                # New connection
                if sock is server_socket:
                    client_socket, addr = server_socket.accept()
                    client_socket.setblocking(0)
                    inputs.append(client_socket)
                    client_data[client_socket] = {"buffer": b"", "addr": addr}
                    print(f"[{addr}] Connected")
                # Data from existing connection
                else:
                    data = sock.recv(4096)
                    if data:
                        # Add data to client buffer
                        client_data[sock]["buffer"] += data
                        # Check if we have a complete message
                        if len(data) < 4096:  # Assuming message is complete
                            addr = client_data[sock]["addr"]
                            handle_client_data(sock, addr, client_data[sock]["buffer"].decode('utf-8'))
                            # Clean up after handling
                            inputs.remove(sock)
                            del client_data[sock]
                            sock.close()
                            print(f"[{addr}] Connection closed")
                    else:
                        # Connection closed by client
                        addr = client_data[sock]["addr"]
                        print(f"[{addr}] Connection closed by client")
                        inputs.remove(sock)
                        del client_data[sock]
                        sock.close()
            
            # Handle exceptional sockets
            for sock in exceptional:
                addr = client_data[sock]["addr"] if sock in client_data else "Unknown"
                print(f"[{addr}] Exception on socket")
                inputs.remove(sock)
                if sock in client_data:
                    del client_data[sock]
                sock.close()
            
    except KeyboardInterrupt:
        print("\nShutting down server...")
    except Exception as e:
        print(f"Server error: {e}")
        traceback.print_exc()
    finally:
        server_socket.close()
        print("Server stopped")

if __name__ == "__main__":
    main() 