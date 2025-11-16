import os
import sys
from google import genai
import socket
import ssl

# --- Network Bypass Configuration ---
GEMINI_IP = "142.250.70.100" 
try:
    # Overriding DNS to bypass WSL instability
    context = ssl.create_default_context()
    socket.getaddrinfo = lambda host, port, family=0, socktype=0, proto=0, flags=0: [(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP, "", (GEMINI_IP, 443))]
    client = genai.Client()
except Exception as e:
    print(f"Error initializing client or overriding DNS: {e}", file=sys.stderr)
    sys.exit(1)

# --- REPL Initialization ---
model_name = "gemini-2.5-pro"
model_name = "gemini-2.5-flash"
chat = client.chats.create(model=model_name)

# --- Context Loading ---
try:
    with open("codebase_context.txt", "r") as f:
        code = f.read()
except FileNotFoundError:
    code = "No codebase_context.txt found. Ask general questions or run the 'find' command first."

# Start the initial chat session with the full codebase context
initial_prompt = f"You are an expert C++ code analyst. The following is the entire codebase context for a project called LogicPP:\n\n{code}\n\nAll subsequent questions are about this code. Confirm you have loaded the context."
chat.send_message(initial_prompt)

print("--- Gemini Interactive Code Analyst REPL ---")
print(f"Model: {model_name} | Context Loaded: {len(code.splitlines())} lines.")
print("Type 'exit' or 'quit' to end the session.")
print("------------------------------------------")

# --- Interactive Loop ---
while True:
    try:
        user_input = input(">>> ")
        if user_input.lower() in ["exit", "quit"]:
            break
        
        if not user_input.strip():
            continue

        response = chat.send_message(user_input)
        print(f"\n{response.text}\n")

    except KeyboardInterrupt:
        print("\nExiting session.")
        break
    except Exception as e:
        print(f"\n[Error]: {e}\n")
        break

