#ifndef sc_common
#define sc_common
#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <algorithm>
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
#include <thread>
#include <mutex>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>


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
    PING,
    REQUEST_KEYS

};

extern int debug_option;
#define HANDLE_EXIT(message) \
    do                       \
    {                        \
        perror(message);     \
        exit(errno);         \
    } while (0)

#define CONT 0
#define EXIT 1
#define thread_handle_error_fn(act, message, ...)                                                                                                \
    do                                                                                                                                           \
    {                                                                                                                                            \
        char buffer_x[300];                                                                                                                      \
        snprintf(buffer_x, sizeof(buffer_x), "[%lu]error:(", pthread_self());                                                                    \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x) - strlen(buffer_x), message, ##__VA_ARGS__);                                      \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x) - strlen(buffer_x), ") in function(%s); errno", function_name_buffer_for_handle); \
        perror(buffer_x);                                                                                                                        \
        if (act == EXIT)                                                                                                                         \
        {                                                                                                                                        \
            pthread_exit(NULL);                                                                                                                  \
        }                                                                                                                                        \
        else                                                                                                                                     \
        {                                                                                                                                        \
            continue;                                                                                                                            \
        }                                                                                                                                        \
    } while (0)

#define thread_notify(message, ...)                                                                                                          \
    do                                                                                                                                       \
    {                                                                                                                                        \
        if (!debug_option)                                                                                                                   \
            continue;                                                                                                                        \
        char buffer_x[300 + RESPONSE_MAXLEN];                                                                                                \
        snprintf(buffer_x, sizeof(buffer_x), "[%lu]notify:(", pthread_self());                                                               \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x) - strlen(buffer_x), message, ##__VA_ARGS__);                                  \
        snprintf(buffer_x + strlen(buffer_x), sizeof(buffer_x) - strlen(buffer_x), ") in function(%s);\n", function_name_buffer_for_handle); \
        printf("%s", buffer_x);                                                                                                              \
    } while (0)

#define HANDLE_CONTINUE(message) \
    do                           \
    {                            \
        perror(message);         \
        continue;                \
    } while (0)

#define KEY_MAX_LEN 100
#define VALUE_MAX_LEN 1000
#define REQUEST_MAXLEN 4096
#define RESPONSE_MAXLEN 4096
#define MAX_LIST 20

#define KEEP 1
#define STOP_CONNECTION 0
#define NETWORK_BYTE_ORDER 0
#define HOST_BYTE_ORDER 1

#define PERIOD 1
#define STABILIZE_PERIOD PERIOD
#define FIX_FINGERS_PERIOD PERIOD
#define CHECK_PREDECESSOR_PERIOD PERIOD
#define REQ_KEYS_PERIOD 5

int send_string_to(int to, const char *from);
int read_string_from(int from, char *into, int size);
using std::string;
using u32 = unsigned int;
string u32_to_string(u32 ipAddressInteger, char byte_order = NETWORK_BYTE_ORDER);

u32 string_to_u32(string &ip, char byte_order);
int set_connection(u32 ip, u32 port); 
string ip_port_to_string(u32 ip, u32 port, char byte_order = HOST_BYTE_ORDER); 
string sd_to_address(int sd);

#define sd2str(sd) sd_to_address(sd).c_str()
#define ep2str(ep) (ip_port_to_string(ep->ip, ep->port) +", id = " +to_string(ep->id)).c_str()

using namespace std;

#endif