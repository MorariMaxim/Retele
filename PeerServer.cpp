#include "Chord_Server.h" 

pthread_t thread_counter = 0;
char act = 0;
char *ipString, *portString;

void *treat_client(void *arg);
int open_to_connection(int port, int *sd);

int main(int argc, char *argv[])
{
    std::signal(SIGPIPE, SIG_IGN);

    if (argc < 3)
        HANDLE_EXIT("Not enough aarguments");

    int sd;
    int port = htons(atoi(argv[1]));
    act = atoi(argv[2]);
    if (act == 0)
    {
        if (argc < 5)
            HANDLE_EXIT("Not enough aarguments");
        ipString = argv[3];
        portString = argv[4];
    }

    open_to_connection(port, &sd);

    struct sockaddr_in from;
    socklen_t length = sizeof(from);

    while (1)
    {
        int client = accept(sd, (struct sockaddr *)&from, &length);

        if (client < 0)
            HANDLE_EXIT("error at accept()\n");

        threadInfo *ti;

        ti = (struct threadInfo *)malloc(sizeof(threadInfo));
        ti->id = thread_counter++;
        ti->client = client;

        if (pthread_create(&ti->id, NULL, &treat_client, ti) != 0)
            HANDLE_CONTINUE("error at pthread_create(&thread_counter, NULL, &treat_client, td)");
    }
}

int open_to_connection(int port, int *sd)
{
    struct sockaddr_in server;
    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = port;

    if ((*sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        HANDLE_EXIT("error when calling socket()\n");

    if (bind(*sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
        HANDLE_EXIT("error at bind()\n");

    if (listen(*sd, 5) == -1)
        HANDLE_EXIT("error at listen()\n");
    return 0;
}

void *treat_client(void *arg)
{
    struct threadInfo ti = *(threadInfo *)arg;

    pthread_detach(pthread_self());

    char request[REQUEST_MAXLEN];
    int request_len;
    char response[RESPONSE_MAXLEN];
    int response_len;

    while (1)
    {

        char reqtype;
        if (read(ti.client, &reqtype, 1) < 0)
            HANDLE_EXIT("read(ti.client, &reqtype, 1) < 0");

        if (read(ti.client, &request_len, 4) < 0)
            HANDLE_EXIT("read(client,&request_len,4)");

        int bread = read(ti.client, request, request_len);
        request[bread] = 0;

        if (bread < 0)
            HANDLE_EXIT("read(client, request, request_len) < 0");

        auto args = vector<string>{"a"};


        
        printf("bread = %d\n",bread);
        if (act == 0)
        {
            char type = responseType::REDIRECT;
            if (write(ti.client, &type, 1) < 0)
                HANDLE_EXIT("write(client, &type, 1) < 0\n");

            u32 redirect_ip, redirect_port;
            redirect_ip = inet_addr((ipString));
            redirect_port = htons(atoi(portString));

            if (write(ti.client, &redirect_ip, 4) < 0)
                HANDLE_EXIT("write(client, &redirect_ip, 4) < 0");

            if (write(ti.client, &redirect_port, 4) < 0)
                HANDLE_EXIT("write(client, &redirect_port, 4) < 0");

            close(ti.client);

            printf("received\n%s\nfrom client, but had to redirect to %d(%s):%s\n", request, ntohl(redirect_ip), ipString, portString);
            return nullptr;
        }
        else
        {
            sprintf(response, "successfully received request");
            response_len = strlen(response);
            char type = responseType::OK;
            if (write(ti.client, &type, 1) < 0)
                HANDLE_EXIT("write(client, &type, 1) < 0\n");

            if (write(ti.client, &response_len, 4) < 0)
                HANDLE_EXIT("write(client, &response_len, 4) < 0\n");

            if (write(ti.client, response, response_len) < 0)
                HANDLE_EXIT("write(client, response, response_len) < 0\n");
            printf("Received following commands arguments\n");fflush(stdout);
            for (auto &s : args )
            {
                printf("%s\n",s.c_str());
            }
        }
    }
    return (NULL);
};


vector<string> parseCommand(const string &command)
{
    vector<string> arguments;
    std::istringstream iss(command);

    std::string argument;
    while (iss >> argument)
    {
        arguments.push_back(argument);
    }

    return arguments;
}