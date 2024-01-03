#include "server_client_common.h"

using namespace std;
using u32 = uint32_t;

int send_request_to(char *request, u32 ip, u32 port);
int set_connection(u32 ip, u32 port);
int display_response(int sd);
bool check_exit(char *request);
void trim(char **str);
int send_string_to(int to, const char *from);
int read_res_type(int from, char &store);
int write_req_type(int from, char store);
int redirect_to(int &sd);
int prepare_request(char *request_buffer, char &empty_command);

static u32 ip;
static u32 port;

int main(int argc, char *argv[])
{

    std::signal(SIGPIPE, SIG_IGN);
    if (argc < 3)
    {
        printf("%s <server_aaddress> <port>\n", argv[0]);
        return -1;
    }

    port = htons(atoi(argv[2]));
    ip = inet_addr(argv[1]);

    char *request = (char *)malloc(REQUEST_MAXLEN);
    request[0] = 0;

    int sd = set_connection(ip, port);

    char running = 1;
    char empty_command = 0;
    while (running)
    {
        if (!empty_command)
            printf("Enter command : ");
        fflush(stdout);

        if (prepare_request(request, empty_command) != 1)
            continue;

        char type = responseType::REDIRECT;
        char reqtype = requestType::CLIENT_REQUEST;
        while (type == responseType::REDIRECT)
        {
            write_req_type(sd, reqtype);

            send_string_to(sd, request);

            read_res_type(sd, type);

            if (type == responseType::REDIRECT)
            {
                redirect_to(sd);
            }
            else if (type == responseType::OK)
            {
                display_response(sd);
            }
            else if (type == responseType::UNRECOGNIZED)
            {
                printf("Unrecognized command\n");
                fflush(stdout);
            }
        }
    }
}

int display_response(int sd)
{
    char response[RESPONSE_MAXLEN];
    read_string_from(sd, response, RESPONSE_MAXLEN);
    printf("%s\n", response);
    fflush(stdout);
    return 0;
}


bool check_exit(char *request)
{
    if (strcmp(request, "exit") == 0)
        exit(0);
    return 0;
}

void trim(char **str)
{
    char *start = *str;
    while (*start == ' ')
    {
        start++;
    }
    *str = start;

    char *end = *str + strlen(*str) - 1;
    while (end > start && *end == ' ')
    {
        end--;
    }

    *(end + 1) = 0;
}

int read_res_type(int from, char &store)
{
    if (read(from, &store, 1) < 0)
        HANDLE_EXIT("error when reading response type from server.\n");
    return 0;
}

int write_req_type(int to, char store)
{

    if (write(to, &store, 1) <= 0)
        HANDLE_EXIT("error when sending request to server.\n");

    return 0;
}

int redirect_to(int &sd)
{
    u32 redirect_ip, redirect_port;

    if (read(sd, &redirect_ip, 4) < 0)
        HANDLE_EXIT("error when reading redirect server address .\n");
    if (read(sd, &redirect_port, 4) < 0)
        HANDLE_EXIT("error when reading redirect server address .\n");

    struct in_addr addr;
    addr.s_addr = ip;
    char ipString[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr, ipString, sizeof(ipString));

    printf("Redirecting to %s:%d\n", ipString, ntohs(redirect_port));

    if (ntohl(redirect_ip) != ip || ntohs(redirect_port) != port)
    {
        close(sd);
        sd = set_connection(redirect_ip, redirect_port);
    }
    return 0;
}

int prepare_request(char *request, char &empty_command)
{
    bzero(request, REQUEST_MAXLEN);
    read(0, request, REQUEST_MAXLEN);
    if (request[strlen(request) - 1] == '\n')
        request[strlen(request) - 1] = 0;

    trim(&request);
    check_exit(request);

    int request_len = strlen(request);

    if (request_len == 0)
    {
        empty_command = 1;
        printf("\n");
        return 0;
    }
    else
        empty_command = 0;
    return 1;
}
