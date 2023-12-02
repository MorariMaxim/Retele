#include "Chord_Server.h"

int debug_option = 1;
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
        cout << cn.printInfo();
        cn.run();
    }
    else
    {
        u32 other_port = atoi(argv[2]);
        printf("Joining node %d to the node %d of an existent Chord network\n", port, other_port);

        endpoint other(ip, other_port, 0);
        ChordNode cn(ip, port, &other);
        cout << cn.printInfo();
        cn.run();
    }
}
