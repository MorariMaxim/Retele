#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

enum responseType
{
    REDIRECT,
    OK,
    UNRECOGNIZED
};

enum requestType
{
    SUCCESSOR,
    GET_PREDECESSOR,
    FIND_SUCCESSOR,
    NOTIFICATION,
    CLIENT_REQUEST,
    END_CONNECTION

};

#define HANDLE_EXIT(message) \
    do                       \
    {                        \
        perror(message);     \
        exit(errno);         \
    } while (0)
#define THREAD_HANDLE_EXIT(message) \
    do                              \
    {                               \
        perror(message);            \
        pthread_exit(NULL);         \
                                    \
    } while (0)

#define HANDLE_CONTINUE(message) \
    do                           \
    {                            \
        perror(message);         \
        continue;                \
    } while (0)
#define REQUEST_MAXLEN 4096
#define RESPONSE_MAXLEN 4096
#define KEEP 1
#define STOP_CONNECTION 0

using namespace std;

typedef struct threadInfo
{
    pthread_t id;
    int client;
} threadInfo;

/*void printBinary(unsigned int number)
{
    for (int i = sizeof(number) * 8 - 1; i >= 0; --i)
    {
        // Use bitwise AND to check the value of the ith bit
        std::cout << ((number >> i) & 1);

        // Optionally add spacing for better readability
        if (i % 8 == 0)
            std::cout << " ";
    }
    std::cout << std::endl;
}*/

vector<string> parseCommand(const string &command);
