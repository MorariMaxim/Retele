CC = g++
CFLAGS = -Wall -std=c++11
LDFLAGS = -lssl -lcrypto

server = PeerServer
client = Client

# List of source files
server_src = PeerServer.cpp Chord_Server.cpp 
client_src = Client.cpp 

all: $(server) $(client)

$(server): $(server_src) Chord_Server.h server_client_common.h
	$(CC) $(CFLAGS) -o $(server) $(server_src) $(LDFLAGS)

$(client): $(client_src) Chord_Server.h server_client_common.h
	$(CC) $(CFLAGS) -o $(client) $(client_src) $(LDFLAGS)


clean:
	rm -f $(TARGET)
