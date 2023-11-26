#include "Chord_Server.h"
    
 
int main(int argc, char *argv[])
{       
    std::signal(SIGPIPE, SIG_IGN);
    if (argc < 2)
        HANDLE_EXIT("Not enough aarguments");

    string ipString = "127.0.0.1";
    u32 ip = string_to_u32(ipString, HOST_BYTE_ORDER);
    u32 port = atoi(argv[1]);

    if (argc == 2)
    {
        printf("Creating a new Chord network\n");

        ChordNode cn(ip, port);

        cn.printfInfo();
        cn.run();
    }
    else
    {
        u32 other_port = atoi(argv[1]);
        printf("Joining node %d to the node %d of an existent Chord network\n", port, other_port);

        endpoint other;
        other.ip = ip;
        other.port = other_port;
        ChordNode cn(ip, port, &other);
        cn.printfInfo();
        cn.run();
    }
}
