#include "Chord_Server.h"

endpoint *ChordNode::find_successor(u32 key, endpoint *original)
{

    if (this->id == key)
        return &this->address;

    requestParams params;
    params.from = original;
    params.key = key;
    params.type = requestType::FIND_SUCCESSOR;

    endpoint *to = nullptr;

    if (fingers[0].address && between(key, id, fingers[0].address->id))
    {
        to = fingers[0].address;
    }
    else
    {
        to = closest_preceding_node(key);
    }
    if (!to || this->address == *to)
        return to;

    return (endpoint *)request(to, params);
}

// incomplete
endpoint *ChordNode::closest_preceding_node(u32 key)
{
    for (u8 i = m_bits - 1; i >= 0; i--)
    {
        if (fingers[i].address != nullptr && between(fingers[i].address->id, this->id, key))
        {
            return fingers[i].address;
        }
    }

    return nullptr;
}

ChordNode::ChordNode(u32 ip_, u32 port_) : id(sha1_hash(port_, ip_)), fingers(fingerTable(id))
{
    this->address.port = port_;
    this->address.ip = ip_;
    this->address.id = this->id;
    predecessor = nullptr;
    fingers[0].address = &this->address;
}

ChordNode::ChordNode(u32 ip_, u32 port_, endpoint *other) : id(sha1_hash(port_, ip_)), fingers(fingerTable(id))
{
    this->address.port = port_;
    this->address.ip = ip_;
    this->address.id = this->id;
    predecessor = nullptr;

    requestParams params;
    params.from = &this->address;
    params.key = this->id;
    params.type = requestType::FIND_SUCCESSOR;
    fingers[0].address = (endpoint *)request(other, params);
}

void *ChordNode::request(endpoint *to, requestParams params)
{
    char type = params.type;
    void *result = nullptr;
    if (type == FIND_SUCCESSOR)
    {
        u32 key = params.key;

        result = (endpoint *)send_find_successor_request(to, key);
    }
    else if (type == GET_PREDECESSOR)
    {
        result = (endpoint *)send_get_predecessor_request(to);
    }
    else if (type == NOTIFICATION)
    {
        send_notification_request(to);
    }

    return result;
}

bool ChordNode::between(u32 key, u32 start, u32 end)
{
    if (start <= end)
    {
        return key >= start && key <= end;
    }
    else
    {
        return key >= start || key <= end;
    }
}

u32 ChordNode::clockw_dist(u32 key1, u32 key2)
{
    return u32();
}

u32 ChordNode::cclockw_dist(u32 key1, u32 key2)
{
    return u32();
}

void *stabilize(void *arg)
{
    while (1)
    {

        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        requestParams params;
        params.type = requestType::GET_PREDECESSOR;
        params.from = &(cn->address);

        auto s = cn->fingers[0].address;
        if (s == nullptr)
            continue;

        auto x = (endpoint *)cn->request(s, params);

        if (x != nullptr && cn->between(x->id, cn->id, s->id) && x->id != cn->id && x->id != s->id)
        {
            s->id = x->id;
            s->port = x->port;
            s->ip = x->ip;
        }

        cn->notify(s);

        sleep(STABILIZE_PERIOD);
    }
}

void *fix_fingers(void *arg)
{
    while (1)
    {
        static u32 next = 0;

        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        next = (next + 1) & mod;

        endpoint *res = cn->find_successor(cn->fingers[next].start, &cn->address);

        if (cn->fingers[next].address == nullptr)
        {
            cn->fingers[next].address = (endpoint *)malloc(sizeof(endpoint));
        }

        cn->fingers[next].address->id = res->id;
        cn->fingers[next].address->ip = res->ip;
        cn->fingers[next].address->port = res->port;

        sleep(FIX_FINGERS_PERIOD);
    }
}

void *check_predecessor(void *arg)
{
    char function_name_buffer_for_handle[] = "check_predecessor";
    while (1)
    {
        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        if (cn->predecessor == nullptr)
            continue;

        char rtype = PING;
        int sd = set_connection(htonl(cn->predecessor->ip), htons(cn->predecessor->port));

        if ((sd == -2) && cn->predecessor != nullptr)
        {
            free(cn->predecessor);
        }
        if (sd == -1 || sd == -3)
            continue;

        if (write(sd, &rtype, 1) < 0)
            thread_handle_error_fn(0, "pinging to %s\n", ip_port_to_string(cn->predecessor->ip, cn->predecessor->port).c_str());

        sleep(CHECK_PREDECESSOR_PERIOD);
    }
    return nullptr;
}
int set_connection(u32 ip, u32 port)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
        return -2;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = port;
    serverAddress.sin_addr.s_addr = ip;

    // Attempt to connect
    if (connect(sd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        if (errno == ECONNREFUSED)
            return -2;
        else
            return -3;
    }

    return sd;
}

string print_sockaddr_in(sockaddr_in &addr)
{

    u32 port = addr.sin_port;
    u32 ip = addr.sin_addr.s_addr;

    return u32_to_string(ip, NETWORK_BYTE_ORDER) + ":" + to_string(ntohs(port));
}

string ip_port_to_string(u32 ip, u32 port, char byte_order)
{
    port = byte_order == HOST_BYTE_ORDER ? htons(port) : port;
    return u32_to_string(ip, byte_order) + ":" + to_string(port);
}
string sd_to_address(int sd)
{
    char clientIP[INET_ADDRSTRLEN];
    int clientPort;
    struct sockaddr_in peerAddr;
    socklen_t peerAddrLen = sizeof(peerAddr);

    if (getpeername(sd, (struct sockaddr *)&peerAddr, &peerAddrLen) == 0)
    {
        inet_ntop(AF_INET, &peerAddr.sin_addr, clientIP, sizeof(clientIP));
        clientPort = ntohs(peerAddr.sin_port);
        string resp = clientIP;
        return resp + ":" + to_string(clientPort);
    }
    else
    {
        return "";
    }
}

void ChordNode::printfInfo()
{

    printf("ChorNode info:\nid=%d, ip=%s, port=%d\n", this->id, u32_to_string(this->address.ip, HOST_BYTE_ORDER).c_str(), this->address.port);

    printf("Its finger table:\n");
    for (size_t i = 0; i < m_bits; i++)
    {
        printf("%d, %d\n", fingers[i].start, fingers[i].end);
    }
}
void *treat_node(void *arg);
void ChordNode::run()
{

    static pthread_t thread_counter = 0;

    int sd;

    // start_periodic_functions();

    open_to_connection(htonl(this->address.ip), htons(this->address.port), &sd);

    struct sockaddr_in from;
    socklen_t length = sizeof(from);

    while (1)
    {
        int client = accept(sd, (struct sockaddr *)&from, &length);

        if (client < 0)
        {
            perror("Error when accepting client in main thread");
            continue;
        }

        threadInfo *ti;
        ti = (struct threadInfo *)malloc(sizeof(threadInfo));
        ti->id = thread_counter++;
        ti->client = client;
        ti->chordnode = this;

        if (pthread_create(&ti->id, NULL, &treat_node, ti) != 0)
            HANDLE_CONTINUE("error at pthread_create(&thread_counter, NULL, &treat_node, td)");
        printf("createad thread(%lu) for client %s\n", ti->id, print_sockaddr_in(from).c_str());
    }
}

void ChordNode::serve_find_successor_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_find_successor_request";
    int client = ti->client;

    u32 key;
    // HOST_BYTE_ORDER
    if (read(ti->client, &key, 4) < 0)
        thread_handle_error_fn(1, "reading key < 0 %s\n", sd_to_address(ti->client).c_str());

    char rtype = responseType::OK;
    endpoint *successor = find_successor(key, &address);
    if (successor == nullptr)
    {
        rtype = responseType::FAIL;
    }

    if (write(client, &rtype, 1) < 0)
        thread_handle_error_fn(1, "write rt<0 to  %s\n", sd_to_address(ti->client).c_str());

    if (rtype == OK)
    { // HOST_BYTE_ORDER
        if (write(client, successor, sizeof(endpoint)) < 0)
            thread_handle_error_fn(1, "write endpoint<0 to  %s\n", sd_to_address(ti->client).c_str());

        thread_notify("sent (%s) to %s", ep2str(successor), sd2str(client));
    }
    else {
        thread_notify("no successor sent to %s" , sd2str(client));
    }

    close(client);
}

void ChordNode::serve_get_predecessor_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_get_predecessor_request";
    int client = ti->client;

    // HOST_BYTE_ORDER
    char rtype = responseType::OK;
    if (predecessor == nullptr)
    {
        rtype = responseType::FAIL;
    }
    if (write(client, &rtype, 1) < 0)
        thread_handle_error_fn(1, "write rt <0 to %s", sd2str(client));

    if (rtype == OK)
    {
        if (write(client, predecessor, sizeof(endpoint)) < 0)
            thread_handle_error_fn(1, "write endpoint<0  to %s\n", sd2str(client));
        thread_notify("sent %s to %s", ep2str(predecessor), sd2str(client));
    }
    else {
        thread_notify("no predecessor sent to %s", sd2str(client));
    }

    close(client);
}

void ChordNode::serve_get_successor_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_get_successor_request";
    int client = ti->client;

    char rtype = responseType::OK;

    if (fingers[0].address == nullptr)
    {
        rtype = responseType::FAIL;
    }

    if (write(client, &rtype, 1) < 0)
        thread_handle_error_fn(1, "write endpoint <0 to %s\n", sd2str(client));

    if (rtype == OK)
    { // HOST_BYTE_ORDER
        if (write(client, fingers[0].address, sizeof(endpoint)) < 0)
            thread_handle_error_fn(1, "write endpoint <0 to %s\n", sd2str(client));
        thread_notify("sent (%s) to %s", ep2str(fingers[0].address), sd2str(client));
    }
    else {
        thread_notify("not successor sent to %s", sd2str(client));
    }
    close(client);
}

void ChordNode::serve_notification_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_notification_request";
    int client = ti->client;
    endpoint notifier;

    // HOST_BYTE_ORDER
    if (read(client, &notifier, sizeof(endpoint)) < 0)
        thread_handle_error_fn(1, "read endpoint<0 from %s\n", sd2str(client));

    if (predecessor == nullptr)
    {
        predecessor = (endpoint *)malloc(sizeof(endpoint));
        *predecessor = notifier;
        thread_notify("set predecessor to %s", ep2str(predecessor));
    }
    else if (predecessor->id != notifier.id && notifier.id != id && between(notifier.id, predecessor->id, id))
    {
        *predecessor = notifier;
        thread_notify("set predecessor to %s", ep2str(predecessor));
    }
    
    close(client);
}

void *ChordNode::send_find_successor_request(endpoint *to, u32 key)
{
    char function_name_buffer_for_handle[] = "send_find_successor_request";
    char type = FIND_SUCCESSOR;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        return nullptr;

    // HOST_BYTE_ORDER
    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sending rt<0 to %s", sd2str(sd));
    if (write(sd, &key, 4) < 0)
        thread_handle_error_fn(1, "write key=%d to %s", key, sd2str(sd));

    if (read(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "read rt from %s", sd2str(sd));
    if (type == responseType::FAIL)
    {
        printf("(thread %lu) rest = fail from %s:%d during send_f_s_q",
               pthread_self(), u32_to_string(htonl(to->ip)).c_str(), to->port);
        close(sd);
        return nullptr;
    }
    endpoint *res = (endpoint *)malloc(sizeof(endpoint));

    // HOST_BYTE_ORDER
    if (read(sd, res, sizeof(endpoint)) < 0)
        thread_handle_error_fn(1, "read endpoint<0 from %s", sd2str(sd));

    close(sd);
    return res;
}

void *ChordNode::send_get_predecessor_request(endpoint *to)
{
    char function_name_buffer_for_handle[] = "send_get_predecessor_request";

    char type = requestType::GET_PREDECESSOR;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        return nullptr;

    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sending reqt to %s", sd2str(sd));
    if (read(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "read restype from %s", sd2str(sd));

    if (type == responseType::OK)
    {
        endpoint *res = (endpoint *)malloc(sizeof(endpoint));
        if (read(sd, res, sizeof(endpoint)) < 0)
            thread_handle_error_fn(1, "read endpoint from %s", sd2str(sd));
        close(sd);
        return res;
    }
    close(sd);
    return nullptr;
}

void *ChordNode::send_get_successor_request(endpoint *to)
{
    char function_name_buffer_for_handle[] = "send_get_successor_request";
    char type = requestType::GET_SUCCESSOR;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        return nullptr;

    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sendreqt to %s", sd2str(sd));
    if (read(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "read restype from %s", sd2str(sd));

    if (type == responseType::OK)
    {
        endpoint *res = (endpoint *)malloc(sizeof(endpoint));
        if (read(sd, res, sizeof(endpoint)) < 0)
            thread_handle_error_fn(1, "read endpoint from %s", sd2str(sd));
        close(sd);
        return res;
    }
    close(sd);
    return nullptr;
}

void *ChordNode::send_notification_request(endpoint *to)
{
    char function_name_buffer_for_handle[] = "send_notification_request";
    char type = requestType::GET_SUCCESSOR;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        return nullptr;

    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sendreqt to %s", sd2str(sd));
    if (write(sd, &address, sizeof(endpoint)) < 0)
        thread_handle_error_fn(1, "sendendpoint to %s", sd2str(sd));
    return nullptr;
}

void ChordNode::notify(endpoint *successor)
{
    requestParams params;
    params.type = requestType::NOTIFICATION;
    params.key = this->id;
    params.from = &this->address;

    request(successor, params);
}

u32 string_to_u32(string &ip, char byte_order = HOST_BYTE_ORDER)
{
    in_addr ipAddress;
    if (inet_aton(ip.c_str(), &ipAddress) != 0)
    {
        return byte_order == HOST_BYTE_ORDER ? ntohl(ipAddress.s_addr) : ipAddress.s_addr;
    }
    else
    {
        return 0;
    }
}

u32 sha1_hash(u32 port_, u32 &ip)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    std::string s = u32_to_string(ip, HOST_BYTE_ORDER) + " " + to_string(port_);

    SHA1(reinterpret_cast<const unsigned char *>(s.c_str()), s.length(), hash);

    u32 key = (*(u32 *)hash) & mod;
    return key;
}

string u32_to_string(u32 ipAddressInteger, char byte_order)
{
    in_addr ipAddress;
    ipAddress.s_addr = byte_order == HOST_BYTE_ORDER ? htonl(ipAddressInteger) : ipAddressInteger;

    char ipAddressString[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &ipAddress, ipAddressString, INET_ADDRSTRLEN) != nullptr)
    {
        return ipAddressString;
    }
    else
    {
        return "";
    }
}

void *treat_node(void *arg)
{
    char function_name_buffer_for_handle[] = "treat_node";
    struct threadInfo *ti = (threadInfo *)arg;
    fflush(stdout);
    pthread_detach(pthread_self());

    ChordNode *cn = (ChordNode *)ti->chordnode;

    char reqtype;
    if (read(ti->client, &reqtype, 1) < 0)
        thread_handle_error_fn(0, "read reqtype < 0");

    if (reqtype == requestType::CLIENT_REQUEST)
    {
        cn->try_serve_client(ti);
    }
    else if (reqtype == requestType::END_CONNECTION)
    {
    }
    else if (reqtype == GET_PREDECESSOR)
    {
        cn->serve_get_predecessor_request(ti);
    }
    else if (reqtype == GET_SUCCESSOR)
    {
        cn->serve_get_successor_request(ti);
    }
    else if (reqtype == FIND_SUCCESSOR)
    {
        cn->serve_find_successor_request(ti);
    }

    else if (reqtype == NOTIFICATION)
    {
        cn->serve_notification_request(ti);
    }
    return (NULL);
}

int ChordNode::open_to_connection(u32 ip, u32 port, int *sd)
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

    if (listen(*sd, MAX_LIST) == -1)
        HANDLE_EXIT("error at listen()\n");

    printf("Opened on %s:%d\n", u32_to_string(ip, NETWORK_BYTE_ORDER).c_str(), ntohs(port));
    return 0;
}

char ChordNode::try_serve_client(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "try_serve_client";

    char request[REQUEST_MAXLEN];
    int request_len;
    char keep_connection = KEEP;
    int client = ti->client;
    bool first_iteration = 1;
    char reqtype;

    while (keep_connection == KEEP)
    {
        if (!first_iteration)
        {
            if (read(client, &reqtype, 1) < 0)
                thread_handle_error_fn(1, "read reqtype < 0 from %s", sd2str(client));
        }
        first_iteration = 0;
        if (read(client, &request_len, 4) < 0)
            thread_handle_error_fn(1, "read request_len<< 0 from %s", sd2str(client));

        int bread = read(client, request, request_len);
        if (bread < 0)
            thread_handle_error_fn(1, "read(client, request, request_len)< 0 from %s", sd2str(client));
        request[bread] = 0;

        vector<string> args = parseCommand(request);

        char resp_type = responseType::OK;
        char response[RESPONSE_MAXLEN] = "";
        bzero(response, sizeof(response));
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

        fflush(stdout);

        if (resp_type == responseType::UNRECOGNIZED)
        {
            if (write(client, &resp_type, 1) < 0)
                thread_handle_error_fn(1, "write rt < 0 to %s", sd2str(client));
        }
        else
        {
            serve_locally(client, responseType::OK, response);
        }

        thread_notify("sent resptype %d", resp_type);
        continue;

        char redirect_decision = 0;
        u32 redirect_ip, redirect_port;

        if (redirect_decision == 1)
        {
            redirect_to(client, redirect_ip, redirect_port);
            printf("Can't serve locally, redirecting client to %s:%d\n", u32_to_string(redirect_ip).c_str(), ntohs(redirect_port));
            close(client);
            keep_connection = STOP_CONNECTION;
        }
    }
    return 0;
}

int ChordNode::redirect_to(int client, u32 redirect_ip, u32 redirect_port)
{
    char function_name_buffer_for_handle[] = "redirect_to";
    char type = responseType::REDIRECT;
    if (write(client, &type, 1) < 0)
        thread_handle_error_fn(1, "write restype to %s", sd2str(client));

    if (write(client, &redirect_ip, 4) < 0)
        thread_handle_error_fn(1, "write red_ip to %s", sd2str(client));

    if (write(client, &redirect_port, 4) < 0)
        thread_handle_error_fn(1, "write red_port to %s", sd2str(client));

    close(client);

    return 0;
}

int ChordNode::serve_locally(int client, responseType type, char *response)
{
    char function_name_buffer_for_handle[] = "serve_locally";
    int response_len = strlen(response);

    if (write(client, &type, 1) < 0)
        thread_handle_error_fn(1, "write type to %s\n", sd2str(client));

    if (write(client, &response_len, 4) < 0)
        thread_handle_error_fn(1, "write resp_len to %s", sd2str(client));

    if (write(client, response, response_len) < 0)
        thread_handle_error_fn(1, "write resp buffer to %s", sd2str(client));

    thread_notify("sent (%s) to %s", response, sd2str(client));
    fflush(stdout);
    return 0;
}

void ChordNode::start_periodic_functions()
{
    threadInfo *ti1 = (threadInfo *)malloc(sizeof(threadInfo));
    ti1->chordnode = this;

    if (pthread_create(&ti1->id, NULL, &stabilize, ti1) != 0)
        HANDLE_CONTINUE("error creating backgorund stabilizing function");

    threadInfo *ti2 = (threadInfo *)malloc(sizeof(threadInfo));
    ti2->chordnode = this;

    if (pthread_create(&ti2->id, NULL, &fix_fingers, ti2) != 0)
        HANDLE_CONTINUE("error creating backgorund stabilizing function");

    threadInfo *ti3 = (threadInfo *)malloc(sizeof(threadInfo));
    ti3->chordnode = this;

    if (pthread_create(&ti3->id, NULL, &check_predecessor, ti3) != 0)
        HANDLE_CONTINUE("error creating backgorund stabilizing function");
}

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