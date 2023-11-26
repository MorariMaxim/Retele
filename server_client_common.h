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
    FAIL,
    UNRECOGNIZED
};

enum requestType
{
    GET_SUCCESSOR,
    GET_PREDECESSOR,
    FIND_SUCCESSOR,
    NOTIFICATION,
    CLIENT_REQUEST,
    END_CONNECTION,
    PING

};

#define HANDLE_EXIT(message) \
    do                       \
    {                        \
        perror(message);     \
        exit(errno);         \
    } while (0)

#define CONT 0
#define EXIT 1
#define thread_handle_error_fn(act, message, ...)                                                                             \
    do                                                                                                                        \
    {                                                                                                                         \
        char buffer_x[300];                                                                                                   \
        snprintf(buffer_x, sizeof(buffer_x), "[%lu]error:(", pthread_self());                                                 \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x), message, ##__VA_ARGS__);                                      \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x), ") in function(%s); errno", function_name_buffer_for_handle); \
        perror(buffer_x);                                                                                                     \
        if (act == EXIT)                                                                                                      \
        {                                                                                                                     \
            pthread_exit(NULL);                                                                                               \
        }                                                                                                                     \
        else                                                                                                                  \
        {                                                                                                                     \
            continue;                                                                                                         \
        }                                                                                                                     \
    } while (0)

#define thread_notify(message, ...)                                                                                       \
    do                                                                                                                    \
    {                                                                                                                     \
        char buffer_x[300];                                                                                               \
        snprintf(buffer_x, sizeof(buffer_x), "[%lu]notify:(", pthread_self());                                            \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x), message, ##__VA_ARGS__);                                  \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x), ") in function(%s);\n", function_name_buffer_for_handle); \
    } while (0)

#define HANDLE_CONTINUE(message) \
    do                           \
    {                            \
        perror(message);         \
        continue;                \
    } while (0)
#define sd2str(sd) sd_to_address(sd).c_str()
#define ep2str(ep) ip_port_to_string(ep->ip, ep->port).c_str()
#define REQUEST_MAXLEN 4096
#define RESPONSE_MAXLEN 4096
#define MAX_LIST 20
#define KEEP 1
#define STOP_CONNECTION 0
#define NETWORK_BYTE_ORDER 0
#define HOST_BYTE_ORDER 1
#define STABILIZE_PERIOD 3
#define FIX_FINGERS_PERIOD 3
#define CHECK_PREDECESSOR_PERIOD 3

using namespace std;

typedef struct threadInfo
{
    pthread_t id;
    int client;
    void *chordnode;
} threadInfo;
