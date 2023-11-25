CC = g++
CFLAGS = -Wall -std=c++11
LDFLAGS = -lssl -lcrypto

server = PeerServer
client = Client

# List of source files
server_src = PeerServer.cpp Chord_Server.cpp 
client_src = Client.cpp 

all: $(server) $(client)

$(server): $(server_src)
	$(CC) $(CFLAGS) -o $(server) $(server_src) $(LDFLAGS)

$(client): $(client_src)
	$(CC) $(CFLAGS) -o $(client) $(client_src) $(LDFLAGS)


clean:
	rm -f $(TARGET)
