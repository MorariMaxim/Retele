#include "Chord_Server.h"

pthread_t thread_counter = 0;

void *treat_node(void *arg);
int open_to_connection(int port, int *sd);
char try_serve_client(int client, vector<string> args);
int redirect_to(int client, u32 red_ip, u32 red_port);
int serve_locally(int client, responseType type, char *response);

int main(int argc, char *argv[])
{
    std::signal(SIGPIPE, SIG_IGN);

    ChordNode cn(0,4);

    cn.printfInfo();

    //cout << cn.between(7,8,0);
    return 0;
    //if (argc < 2)
      //HANDLE_EXIT("Not enough aarguments");
        

    
    int sd;
    //int port = htons(atoi(argv[1]));
    int port = htons(2025); 

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

        if (pthread_create(&ti->id, NULL, &treat_node, ti) != 0)
            HANDLE_CONTINUE("error at pthread_create(&thread_counter, NULL, &treat_node, td)");
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

void *treat_node(void *arg)
{ 
    struct threadInfo ti = *(threadInfo *)arg;
    printf("Treating clinet on socket %d\n",ti.client);fflush(stdout);
    pthread_detach(pthread_self());

    char request[REQUEST_MAXLEN];
    int request_len;

    char keep_connection = KEEP;

    while (keep_connection == KEEP)
    {
        char reqtype;
        if (read(ti.client, &reqtype, 1) < 0)
            THREAD_HANDLE_EXIT("read(ti.client, &reqtype, 1) < 0");

        if (reqtype == requestType::CLIENT_REQUEST)
        {
            if (read(ti.client, &request_len, 4) < 0)
                THREAD_HANDLE_EXIT("read(client,&request_len,4)");

            int bread = read(ti.client, request, request_len);
            request[bread] = 0;

            if (bread < 0)
                THREAD_HANDLE_EXIT("read(client, request, request_len) < 0");

            auto args = parseCommand(request);

            keep_connection = try_serve_client(ti.client, args);
        }
        else  if (reqtype == requestType::END_CONNECTION){

        }
        else
        {
            // request from peer
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

int redirect_to(int client, u32 redirect_ip, u32 redirect_port)
{

    char type = responseType::REDIRECT;
    if (write(client, &type, 1) < 0)
        THREAD_HANDLE_EXIT("write(client, &type, 1) < 0 in redirect_to\n");

    if (write(client, &redirect_ip, 4) < 0)
        THREAD_HANDLE_EXIT("write(client, &redirect_ip, 4) < 0");

    if (write(client, &redirect_port, 4) < 0)
        THREAD_HANDLE_EXIT("write(client, &redirect_port, 4) < 0");

    close(client);

    return 0;
}
int serve_locally(int client, responseType type, char *response)
{
    int response_len = strlen(response);

    if (write(client, &type, 1) < 0)
        THREAD_HANDLE_EXIT("write(client, &type, 1) < 0 in serve_locally\n");

    if (write(client, &response_len, 4) < 0)
        THREAD_HANDLE_EXIT("write(client, &response_len, 4) < 0\n");

    if (write(client, response, response_len) < 0)
        THREAD_HANDLE_EXIT("write(client, response, response_len) < 0\n");

    fflush(stdout);
    return 0;
}
char try_serve_client(int client, vector<string> args)
{
    char resp_type;
    char response[RESPONSE_MAXLEN] = "";
    int len = args.size();
    string &command = args[0];


    if (len < 1)
    {
        resp_type = responseType::UNRECOGNIZED;
    }
    else
    {
        if (command.compare("get") == 0)
        {
            if (len < 2)
            {
                resp_type = responseType::UNRECOGNIZED;
            }
            sprintf(response, "Recognized request for fetching the value of a key(%s)", args[1].c_str());
        }
        else if (command.compare("insert") == 0)
        {
            if (len < 3)
            {
                resp_type = responseType::UNRECOGNIZED;
            }
            sprintf(response, "Recognized request for insertion of the (%s,%s) pair", args[1].c_str(), args[2].c_str());
        }
        else if (command.compare("delete") == 0)
          {
            if (len < 2)
            {
                resp_type = responseType::UNRECOGNIZED;
            }
            sprintf(response, "Recognized request for deleting the key(%s)", args[1].c_str());
        }
    }
    
    printf("Treating clinet on socket %d, response : %d, his args:\n",client,resp_type);fflush(stdout);
    for (auto &s : args)
    {
        printf("%s, ",s.c_str());
    }
    printf("\n");

    if (resp_type == responseType::UNRECOGNIZED)
    {
        if (write(client, &resp_type, 1) < 0)
            THREAD_HANDLE_EXIT("write(client, &type, 1) < 0 in try_Serve_client\n");
        return KEEP;
    }
    else
    {
        serve_locally(client, responseType::OK, response);
        return KEEP;
    }

    char redirect_decision = 0;
    u32 redirect_ip, redirect_port;

    if (redirect_decision == 1)
    {
        redirect_to(client, redirect_ip, redirect_port);
        printf("Can't serve locally, redirecting client to %s:%d\n", u32_to_string(redirect_ip).c_str(), ntohs(redirect_port));
        close(client);
        return STOP_CONNECTION;
    }
}