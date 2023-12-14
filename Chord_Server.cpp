#include "Chord_Server.h"
endpoint *ChordNode::find_successor(u32 key, endpoint *original)
{
    char function_name_buffer_for_handle[] = "find_successor";

    if (this->id == key)
    {
        endpoint *res = (endpoint *)malloc(sizeof(endpoint));
        *res = this->address;
        return res;
    }

    requestParams params;
    params.from = original;
    params.key = key;
    params.type = requestType::FIND_SUCCESSOR;

    endpoint *to = nullptr;

    thread_notify("fingers[0].address = %s; id = %d", ep2str(fingers[0].address), fingers[0].address->id);
    if (fingers[0].address && between(key, id, fingers[0].address->id))
    {
        endpoint *res = (endpoint *)malloc(sizeof(endpoint));
        *res = *fingers[0].address;
        thread_notify("to = successor");
        return res;
    }
    else
    {
        to = closest_preceding_node(key);
        thread_notify("to = closest_preceding");
    }
    if (!to)
        return nullptr;
    if (this->address == *to)
    {
        endpoint *res = (endpoint *)malloc(sizeof(endpoint));
        *res = *to;
        return res;
    }

    thread_notify("request find_s for %d from %s", key, ep2str(to));
    return (endpoint *)request(to, params);
}
endpoint *ChordNode::closest_preceding_node(u32 key)
{
    char function_name_buffer_for_handle[] = "closest_preceding_node";
    u32 min_distance = mod + 2, temp;
    u8 min_index;
    bool found = 0;
    for (int i = m_bits - 1; i >= 0; i--)
    {
        if (fingers[i].address == nullptr)
            continue;

        temp = fingers[i].address->id;
        if (between(temp, this->id, key) && distance(temp, key) < min_distance)
        {
            min_distance = distance(temp, key);
            min_index = i;
            found = 1;
        }
    }
    if (!found)
        return nullptr;

    thread_notify("found at fingers[%d].address->id = %d, this->id = %d, key = %d, distance = %d",
                  min_index, fingers[min_index].address->id, this->id, key, min_distance);
    return fingers[min_index].address;
}

ChordNode::ChordNode(u32 ip_, u32 port_) : id(sha1_hash(port_, ip_)), address(ip_, port_, this->id), data_store(ip_port_to_string(ip_, port_)), fingers(fingerTable(id))
{ 
    predecessor = nullptr;
    fingers[0].address = (endpoint *)malloc(sizeof(endpoint));
    *fingers[0].address = address;
}

ChordNode::ChordNode(u32 ip_, u32 port_, endpoint *other) : ChordNode(ip_,port_)
{ 
    requestParams params;
    params.from = &this->address;
    params.key = this->id;
    params.type = requestType::FIND_SUCCESSOR;
    free(fingers[0].address);
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

u32 ChordNode::distance(u32 key1, u32 key2)
{
    if (key2 >= key1)
        return key2 - key1;
    else
        return mod - (key1 - key2) + 1;
}

void *stabilize(void *arg)
{
    char function_name_buffer_for_handle[] = "stabilize";
    thread_notify("started");
    while (1)
    {
        sleep(STABILIZE_PERIOD);

        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        requestParams params;
        params.type = requestType::GET_PREDECESSOR;
        params.from = &(cn->address);

        auto s = *cn->fingers[0].address;
        if (s == cn->address)
            continue;

        // thread_notify("sending request for predecessor from %s", ep2str(s));

        auto x = (endpoint *)cn->request(&s, params);
        if (x != nullptr)
        {
            thread_notify("x = %s,id= %d", ep2str(x), x->id);
        }
        if (x != nullptr &&
            (cn->distance(cn->id, x->id) <
                 cn->distance(cn->id, s.id) ||
             *cn->fingers[0].address == cn->address) &&
            cn->address != *x)
        {
            *cn->fingers[0].address = *x;
            thread_notify("set successor to %s", ep2str(x));
        }
        if (x != nullptr)
        {
            thread_notify("trying to free %s", ep2str(x));
            free(x);
            x = nullptr;
        }

        ;
        if (*cn->fingers[0].address != cn->address)
            cn->notify(cn->fingers[0].address);
    }
}

void *fix_fingers(void *arg)
{
    char function_name_buffer_for_handle[] = "fix_fingers";
    thread_notify("started");
    while (1)
    {
        sleep(FIX_FINGERS_PERIOD);

        static u32 next = 0;

        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        next = (next + 1) % m_bits;

        thread_notify("trying to find finger for %d, next = %d", cn->fingers[next].start, next);
        endpoint *res = cn->find_successor(cn->fingers[next].start, &cn->address);

        if (res == nullptr)
        {
            continue;
        }
        if (cn->fingers[next].address == nullptr)
        {
            cn->fingers[next].address = (endpoint *)malloc(sizeof(endpoint));
            *cn->fingers[next].address = *res;
            thread_notify("found successor of %d, %s", cn->fingers[next].start, ep2str(res));
        }
        else if (cn->distance(cn->fingers[next].start, res->id) <
                 cn->distance(cn->fingers[next].start, cn->fingers[next].address->id))
        {
            *cn->fingers[next].address = *res;
            thread_notify("found successor of %d, %s", cn->fingers[next].start, ep2str(res));
        }
        if (res != nullptr)
        {
            thread_notify("trying to free %s", ep2str(res));
            free(res);
            res = nullptr;
        }
    }
}

void *check_predecessor(void *arg)
{
    char function_name_buffer_for_handle[] = "check_predecessor";
    thread_notify("started");
    while (1)
    {
        sleep(CHECK_PREDECESSOR_PERIOD);
        threadInfo *ti = (threadInfo *)arg;
        ChordNode *cn = (ChordNode *)ti->chordnode;

        if (cn->predecessor == nullptr)
            continue;
        // thread_notify("checking for predecessor(not null)");
        char rtype = PING;
        int sd = set_connection(htonl(cn->predecessor->ip), htons(cn->predecessor->port));

        if ((sd == -2) && cn->predecessor != nullptr)
        {
            thread_notify("predecessor failed");
            thread_notify("trying to free %s", ep2str(cn->predecessor));
            free(cn->predecessor);
            cn->predecessor = nullptr;
        }
        if (sd == -1 || sd == -3)
            continue;

        if (write(sd, &rtype, 1) < 0)
            thread_notify("pinging to %s\n", sd2str(sd));
        // thread_notify("pinged to predecessor");
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
    port = byte_order == NETWORK_BYTE_ORDER ? ntohs(port) : port;
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

string ChordNode::printInfo()
{
    string info = "";
    info.reserve(300);

    char buffer[100];
    snprintf(buffer, 100, "ChorNode info:\nid=%d, address=%s:%d\n", this->id,
             u32_to_string(this->address.ip, HOST_BYTE_ORDER).c_str(), this->address.port);
    info += buffer;
    snprintf(buffer, 100, "Its finger table:\n");
    info += buffer;
    for (size_t i = 0; i < m_bits; i++)
    {
        snprintf(buffer, 100, "[%d,%d] ", fingers[i].start, fingers[i].end);
        info += buffer;
        if (fingers[i].address)
        {
            snprintf(buffer, 100, "%s", ep2str(fingers[i].address));
        }
        else
        {
            buffer[0] = 0;
            strcat(buffer, "void");
        }
        strcat(buffer, "\n");
        info += buffer;
    }
    if (predecessor)
    {
        snprintf(buffer, 100, "predecessor = %s\n", ep2str(predecessor));
    }
    else
    {
        snprintf(buffer, 100, "predecessor = void\n");
    }
    info += buffer;
    return info;
}
void *treat_node(void *arg);
void ChordNode::run()
{

    static pthread_t thread_counter = 0;

    int sd;

    start_periodic_functions();

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
    thread_notify("key = %d", key);
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
    else
    {
        thread_notify("no successor sent to %s", sd2str(client));
    }
    if (successor != nullptr)
    {
        thread_notify("trying to free %s", ep2str(successor));
        free(successor);
        successor = nullptr;
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
    else
    {
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
    else
    {
        thread_notify("not successor sent to %s", sd2str(client));
    }
    close(client);
}

void ChordNode::serve_notification_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_notification_request";
    int client = ti->client;
    endpoint *notifier = (endpoint *)malloc(sizeof(endpoint));

    // HOST_BYTE_ORDER
    if (read(client, notifier, sizeof(endpoint)) < 0)
        thread_handle_error_fn(1, "read endpoint<0 from %s\n", sd2str(client));

    thread_notify("notifier %s, id = %d", ep2str(notifier), notifier->id);

    if (predecessor == nullptr)
    {
        predecessor = (endpoint *)malloc(sizeof(endpoint));
        *predecessor = *notifier;
        thread_notify("set predecessor to %s, id = %d", ep2str(predecessor), predecessor->id);
    }
    else if (predecessor->id != notifier->id && notifier->id != id &&
             between(notifier->id, (predecessor->id + 1) & mod, id))
    {
        thread_notify("old predecessor %s, id = %d", ep2str(predecessor), predecessor->id);
        *predecessor = *notifier;
        thread_notify("1.set predecessor to %s, id = %d", ep2str(predecessor), predecessor->id);
    }
    if (predecessor != nullptr && *fingers[0].address == address)
    {
        *fingers[0].address = *predecessor;
        thread_notify("2.set successor to %s", ep2str(fingers[0].address));
    }
    thread_notify("trying to free %s", ep2str(notifier));
    free(notifier);
    close(client);
}

void ChordNode::serve_keys_request(threadInfo *ti)
{
    char function_name_buffer_for_handle[] = "serve_keys_request";
    int sd = ti->client;

    u32 key;
    if (read(sd, &key, 4) < 0)
        thread_handle_error_fn(1, "read key < 0 from %s", sd2str(sd));

    u32 temp;
    char type = responseType::OK;

    for (auto it = data_store.begin(); it != data_store.end();)
    {
        temp = sha1_hash(it->first);

        if (distance(temp, this->id) > distance(temp, key))
        {
            if (write(sd, &type, 1) < 0)
                thread_handle_error_fn(1, "send restype <0 to %s", sd2str(sd));

            send_string_to(sd, it->first.c_str());
            send_string_to(sd, it->second.c_str());

            it = data_store.erase(it);
        }
        else
        {
            ++it;
        }
    }
    type = responseType::FAIL;
    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "send restype <0 to %s", sd2str(sd));
}

void *ChordNode::send_find_successor_request(endpoint *to, u32 key)
{
    char function_name_buffer_for_handle[] = "send_find_successor_request";
    char type = FIND_SUCCESSOR;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        thread_notify("cant connect to %s", ep2str(to));

    // HOST_BYTE_ORDER
    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sending rt<0 to %s", sd2str(sd));
    if (write(sd, &key, 4) < 0)
        thread_handle_error_fn(1, "write key=%d to %s", key, sd2str(sd));

    if (read(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "read rt from %s", sd2str(sd));
    if (type == responseType::FAIL)
    {
        thread_notify("rest = fail from %s", sd2str(sd));
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
    char type = requestType::NOTIFICATION;

    int sd = set_connection(htonl(to->ip), htons(to->port));
    if (sd < 0)
        return nullptr;

    thread_notify("sending");
    if (write(sd, &type, 1) < 0)
        thread_handle_error_fn(1, "sendreqt to %s", sd2str(sd));
    if (write(sd, &address, sizeof(endpoint)) < 0)
        thread_handle_error_fn(1, "sendendpoint to %s", sd2str(sd));
    return nullptr;
}

void *ChordNode::send_keys_request()
{

    int debug_option = 1;
    char function_name_buffer_for_handle[] = "send_keys_request";
    if (*fingers[0].address == this->address)
    {
        return nullptr;
        thread_notify("no successor");
    }

    thread_notify("stariting send_keys_req from %s", ep2str(fingers[0].address));

    endpoint to = *fingers[0].address;
    auto to_ = &to;

    char reqt = requestType::REQUEST_KEYS;
    int sd = set_connection(htonl(to.ip), htons(to.port));
    if (sd < 0)
    {
        thread_notify("couldn't set connection to %s", ep2str(to_));
        return nullptr;
    }

    if (write(sd, &reqt, 1) < 0)
        thread_handle_error_fn(1, "send reqt to %s <0", ep2str(to_));
    if (write(sd, &this->id, 4) < 0)
        thread_handle_error_fn(1, "send id to %s <0", ep2str(to_));

    char rest;
    bool received_anything = 0;
    while (1)
    {
        if (read(sd, &rest, 1) < 0)
            thread_handle_error_fn(1, "read restype from %s", sd2str(sd));

        if (rest != OK)
        {
            if (!received_anything)
                thread_notify("didn't receive anything");
            return nullptr;
        }
        char key_comp[KEY_MAX_LEN];
        read_string_from(sd, key_comp, KEY_MAX_LEN);

        char value_comp[VALUE_MAX_LEN];
        read_string_from(sd, value_comp, VALUE_MAX_LEN);

        thread_notify("received (%s,%s) from %s", key_comp, value_comp, ep2str(to_));

        data_store.insert(key_comp,value_comp);
        received_anything = 1;
    }

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
    string s = u32_to_string(ip, HOST_BYTE_ORDER) + " " + to_string(port_);
    return sha1_hash(s);
}

u32 sha1_hash(const string &s)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char *>(s.c_str()), s.length(), hash);
    u32 key = (*(u32 *)hash) & mod;
    return key;
}

int directory_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int create_directory(const char *path) {    
    return mkdir(path, 0777) == 0;     
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

    char notification_buffer[20] = "";
    switch (reqtype)
    {
    case GET_SUCCESSOR:
        strcat(notification_buffer, "GET_SUCCESSOR");
        break;
    case GET_PREDECESSOR:
        strcat(notification_buffer, "GET_PREDECESSOR");
        break;
    case FIND_SUCCESSOR:
        strcat(notification_buffer, "FIND_SUCCESSOR");
        break;
    case NOTIFICATION:
        strcat(notification_buffer, "NOTIFICATION");
        break;
    case CLIENT_REQUEST:
        strcat(notification_buffer, "CLIENT_REQUEST");
        break;
    case END_CONNECTION:
        strcat(notification_buffer, "END_CONNECTION");
        break;
    case PING:
        strcat(notification_buffer, "PING");
        break;
    default:
        break;
    }
    //  thread_notify("reqtype = %s", notification_buffer);

    if (reqtype == requestType::CLIENT_REQUEST)
    {
        printf("Created thread for client %s", sd2str(ti->client));
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
    else if (reqtype == PING)
    {
    }
    else if (reqtype == REQUEST_KEYS)
    {
        cn->serve_keys_request(ti);
    }
    else
    {
        thread_handle_error_fn(1, "unknown request");
    }
    close(ti->client);
    pthread_exit(NULL);
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
    int debug_option = 1;
    char function_name_buffer_for_handle[] = "try_serve_client";

    char request[REQUEST_MAXLEN];
    int request_len;
    char keep_connection = KEEP;
    int client = ti->client;
    bool first_iteration = 1;
    char reqtype;
    u32 redirect_ip, redirect_port;

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

        char resp_type = responseType::UNRECOGNIZED;
        
        char response[RESPONSE_MAXLEN] = "";
        bzero(response, sizeof(response));
        int len = args.size();
        string command;
        u32 hash;
        endpoint *succ;

        if (len < 1)
        {
            resp_type = responseType::UNRECOGNIZED;
            goto send_response;
        }
        command = args[0];

        if (len == 1)
        {
            resp_type = responseType::OK;
            if (command.compare("printinfo") == 0)
            {
                string info = printInfo();
                sprintf(response, "%s", info.c_str());
            }
            goto send_response;
        }
        hash = sha1_hash(args[1]);
        succ = find_successor(hash, nullptr);
        thread_notify("hash of %s = %d, successor = %s", args[1].c_str(), hash, ep2str(succ)); 

        if (succ != nullptr && succ->id != this->id)
        {
            resp_type = responseType::REDIRECT;
            redirect_ip = succ->ip;
            redirect_port = succ->port;
            sprintf(response, "Recognized a valid request, but have to redirect %s", ep2str(succ));
            goto send_response;
        } 
        if (command.compare("get") == 0)
        {
            resp_type = responseType::OK;
            auto it = data_store.find(args[1]);
            if (it != data_store.end())
            {
                sprintf(response, "Found (%s,%s)", args[1].c_str(), it->second.c_str());
            }
            else
            {
                sprintf(response, "Recognized request for fetching the value of key \"%s\", but found no such pair", args[1].c_str());
            }
        }
        else if (command.compare("insert") == 0)
        {
            resp_type = responseType::OK;
            if (len < 3)
            {
                sprintf(response, "insert <key> <value>");
                goto send_response;
            }
            else
            {
                data_store[args[1]] = args[2];
                data_store.insert(args[1],args[2]);
                sprintf(response, "Successfully inserted (%s,%s) pair", args[1].c_str(), args[2].c_str());
            }
        }
        else if (command.compare("delete") == 0)
        { 
            resp_type = responseType::OK;
            if (len < 2)
            {
                sprintf(response, "delete <key>");
                goto send_response;
            }
            thread_notify("step 4");
            auto key = data_store.find(args[1]);

            if (key != data_store.end())
            { 
                string value = key->second;
                data_store.erase(key);                 
                sprintf(response, "Successfully deleted (%s,%s) pair",args[1].c_str(),value.c_str());
            }
            else
            { 
                sprintf(response, "Found no (%s,<value>) pair", args[1].c_str());
            }
        }
        else
        {
            resp_type = responseType::UNRECOGNIZED;
        } 
    send_response: 
        if (resp_type == responseType::UNRECOGNIZED)
        {
            if (write(client, &resp_type, 1) < 0)
                thread_handle_error_fn(1, "write rt < 0 to %s", sd2str(client));
        }
        else if (resp_type == responseType::OK)
        {
            thread_notify("sending response=(%s)",response);
            serve_locally(client, responseType::OK, response);
        }
        else if (resp_type == responseType::REDIRECT)
        {
            redirect_to(client, htonl(redirect_ip), htons(redirect_port));
            printf("Can't serve locally, redirecting client to %s:%d\n", u32_to_string(redirect_ip, HOST_BYTE_ORDER).c_str(), redirect_port);
            keep_connection = STOP_CONNECTION;
        }
        thread_notify("sent resptype %d", resp_type);
        continue;
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

    thread req_keys_prd(&ChordNode::request_keys_periodic, this);
    req_keys_prd.detach();
}

void ChordNode::request_keys_periodic()
{
    char function_name_buffer_for_handle[] = "request_keys_periodic";
    thread_notify("started");

    while (1)
    {
        sleep(REQ_KEYS_PERIOD);

        thread soldier(&ChordNode::send_keys_request, this);

        soldier.join();
    }
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

int read_string_from(int from, char *into, int size)
{
    char function_name_buffer_for_handle[] = "read_string_from";

    int len;

    if (read(from, &len, 4) < 0)
        thread_handle_error_fn(1, "reading len < 0");

    len = len > size ? size : len;

    if (read(from, into, len) < 0)
        thread_handle_error_fn(1, "reading string < 0");
    into[len] = 0;

    return 0;
}

int send_string_to(int to, const char *from)
{
    char function_name_buffer_for_handle[] = "send_string_to";
    int len = strlen(from);

    if (write(to, &len, 4) < 0)
        thread_handle_error_fn(1, "sending len <0");

    if (write(to, from, len) < 0)
        thread_handle_error_fn(1, "sending string < 0");

    return 0;
}
