# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -O3

# Targets
CLIENT = udp-ping-client
SERVER = udp-ping-server

# Source files
CLIENT_SRC = client.cpp
SERVER_SRC = server.cpp

# Object files
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

# Default target
all: $(CLIENT) $(SERVER)

# Build client
$(CLIENT): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(CLIENT_OBJ)

# Build server
$(SERVER): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(SERVER_OBJ)

# Compile client source
client.o: $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -c $(CLIENT_SRC)

# Compile server source
server.o: $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -c $(SERVER_SRC)

# Clean up build artifacts
clean:
	rm -f $(CLIENT) $(SERVER) *.o
