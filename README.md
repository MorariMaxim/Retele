## Chord Protocol
This project is an attempt at implementing the chord protocol.
- [wikipedia](https://en.wikipedia.org/wiki/Chord_(peer-to-peer))
- [Original Paper](https://pdos.csail.mit.edu/papers/chord:sigcomm01/chord_sigcomm.pdf)

This is a distributed hash table. Each server is responsible for a set of keys, whose hash is assigned to the server.

## Results
- The main parts of the protocol are implemented: joining to an existing network, stabilizing (setting the right fingers, i.e. pointers to nodes in the network), efficient location of data (in log(n), where n = number of servers in the network)
- Didn't treat nodes leaving the system

## Getting it work
It works on **Linux**
Run **make**
You may need to to install some libraries: **lssl**, **lcrypto**

## Use
To create a new network run 
**./ServerLauncher \<port>**
To join a existing network
**./ServerLauncher <port_of_new_server> <port_of_existing_server>**

To connect to a network **as a client**
./Client 127.0.0.1 **\<port>**
Then, you can do the following
- **printinfo** - to get the information of the current server (address, id in the ring and its finger table)
- **insert** \<key> \<value> - to insert in the hash table
-  **get** \<key> - to get a pair, if it exists
- **delete** \<key> - to delete a pair, if it exists

