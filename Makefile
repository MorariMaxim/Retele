CC = g++
CFLAGS = -Wall -std=c++17
LDFLAGS = -lssl -lcrypto

server = ServerLauncher
client = Client

# List of source files
server_src = ServerLauncher.cpp Chord_Server.cpp server_client_common.cpp
client_src = Client.cpp server_client_common.cpp

all: $(server) $(client)

$(server): $(server_src) Chord_Server.h 
	$(CC) $(CFLAGS) -o $(server) $(server_src) $(LDFLAGS)

$(client): $(client_src) Chord_Server.h 
	$(CC) $(CFLAGS) -o $(client) $(client_src) $(LDFLAGS)
 
clean:
	rm -f $(TARGET)
