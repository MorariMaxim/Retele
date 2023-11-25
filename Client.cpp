#include "server_client_common.h"

using namespace std;
using u32 = uint32_t;

int send_request_to(char *request, u32 ip, u32 port);
int set_connection_to(u32 ip, u32 port);
int display_response(int sd);

int main(int argc, char *argv[])
{

    std::signal(SIGPIPE, SIG_IGN);
    int v = 0;
    u32 port, ip;
    if (!v)
    {
        if (argc < 3)
        {
            printf("%s <server_aaddress> <port>\n", argv[0]);
            return -1;
        }
        port = htons(atoi(argv[2]));
        ip = inet_addr(argv[1]);
    }
    else
    {
        char ipstring[] = "127.0.0.1";
        char portstring[] = "2024";

        port = htons(atoi(portstring));
        ip = inet_addr(ipstring);
    }

    char request[REQUEST_MAXLEN];
    request[0] = 0;
    int request_len;

    int sd = set_connection_to(ip, port);
    while (1)
    {

        printf("Enter command : ");
        fflush(stdout);
        bzero(request, sizeof(request));
        read(0, request, REQUEST_MAXLEN);
        if (request[strlen(request) - 1] == '\n')
            request[strlen(request) - 1] = 0;

        request_len = strlen(request);

        if (request_len == 0)
        {
            printf("\n");
            continue;
        }

        printf("$%s$ len = %d\n", request, request_len);
        fflush(stdout);
        char type = responseType::REDIRECT;
        char reqtype = requestType::CLIENT_REQUEST;
        while (type == responseType::REDIRECT)
        {   

            if (write(sd, &reqtype, 1) <= 0)
                HANDLE_EXIT("error when sending request to server.\n");
            
            if (write(sd, &request_len, 4) <= 0)
                HANDLE_EXIT("error when sending request to server.\n");

            if (write(sd, request, request_len) <= 0)
                HANDLE_EXIT("error when sending request to server.\n");

            if (read(sd, &type, 1) < 0)
                HANDLE_EXIT("error when reading response type from server.\n");

            if (type == responseType::REDIRECT)
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
                if (ntohl(redirect_ip) != ip || ntohl(redirect_port) != port)
                {
                    close(sd);
                    sd = set_connection_to(redirect_ip, redirect_port);
                }
            }
            else
            {
                display_response(sd);
            }
        }
    }
}
int display_response(int sd)
{
    int response_length;
    char response[RESPONSE_MAXLEN];
    response[0] = 0;

    if (read(sd, &response_length, 4) < 0)
        HANDLE_EXIT("error when reading response length from server.\n");

    if (read(sd, response, response_length) < 0)
        HANDLE_EXIT("error when reading response body from server.\n");

    printf("%s\n", response);
    fflush(stdout);
    return 0;
}

int set_connection_to(u32 ip, u32 port)
{
    struct sockaddr_in server;
    int sd;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = ip;
    server.sin_port = port;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        HANDLE_EXIT("error when calling socket().\n");

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        HANDLE_EXIT("error when calling connect()\n");
    return sd;
}