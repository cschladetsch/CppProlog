import os
import sys
from google import genai
import ssl
import socket

# Hardcoded IP address to bypass faulty DNS
GEMINI_IP = "142.250.70.100" 

if not os.getenv("GEMINI_API_KEY"):
    print("Error: GEMINI_API_KEY environment variable not set.", file=sys.stderr)
    sys.exit(1)

try:
    # Use standard library socket setup to override DNS
    context = ssl.create_default_context()
    socket.getaddrinfo = lambda host, port, family=0, socktype=0, proto=0, flags=0: [(socket.AF_INET, socktype, proto, "", (GEMINI_IP, 443))]
    
    client = genai.Client()

except Exception as e:
    print(f"Error initializing client or overriding DNS: {e}", file=sys.stderr)
    sys.exit(1)

try:
    with open("codebase_context.txt", "r") as f:
        code = f.read()
except FileNotFoundError:
    print("Error: codebase_context.txt not found. Run the find command first.", file=sys.stderr)
    sys.exit(1)

# Get prompt from environment variable or default
PROMPT = os.getenv("PROMPT") or "Analyze the provided codebase."
model_name = "gemini-2.5-pro"
full_content = f"{PROMPT}\n\n{code}"

# Generate the content
response = client.models.generate_content(
    model=model_name,
    contents=full_content
)
print(response.text)

