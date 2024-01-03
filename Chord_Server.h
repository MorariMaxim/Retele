#ifndef CHORD_SERVER_H
#define CHORD_SERVER_H

#include "server_client_common.h"

using namespace std;

using u32 = uint32_t;
using u8 = u_char;
const char m_bits = 10;
const u32 mod = (1 << m_bits) - 1;

class endpoint
{
public:
    u32 port;

    u32 ip;
    u32 id;
    bool operator==(const endpoint &other) const;
    bool operator!=(const endpoint &other) const;
    endpoint &operator=(const endpoint &other);
    endpoint(u32 ip_, u32 port_, u32 id_);
};

class ChordNode;
class fingerTable
{
    struct finger
    {
        u32 start, end;
        endpoint *address = nullptr;
    };

    finger fingers[m_bits];

    fingerTable(u32 id);
    friend class ChordNode;

public:
    finger &operator[](u_char index);
};
class requestParams
{
public:
    requestType type;
    u32 key;
    endpoint *from;
};
struct key_strct
{
    u32 hash;
    string key;
    u32 id;
};
struct value_strct
{
};
u32 sha1_hash(const std::string &s);

using json = nlohmann::json;
int directory_exists(const char *path);
int create_directory(const char *path);
class data_store_
{
    using map = unordered_map<string, string>;

public:
    std::mutex data_store_mutex;
    std::unordered_map<std::string, std::string> keys;
    string data_store_path;

    data_store_(string address);

    void retrieve_saved_keys();

    std::string &operator[](const std::string &key);

    unique_lock<std::mutex> lock_data_store();
    map::iterator begin();

    map::iterator end();

    map::iterator find(const string &s);

    map::iterator erase(map::iterator &it);
    void insert(const string &key, const string &value);
    string path_from_key(const string &key);
};
class ChordNode
{
public:
    u32 id;
    endpoint address;

    data_store_ data_store;
    fingerTable fingers;

    endpoint *predecessor;
    friend class fingerTable;

    endpoint *find_successor(u32 key );

    endpoint *closest_preceding_node(u32 key);

    ChordNode(u32 ip_, u32 port_);

    ChordNode(u32 ip_, u32 port_, endpoint *other);

    void *request(endpoint *to, requestParams type);

    bool between(u32 key, u32 start, u32 end);

    u32 distance(u32 key1, u32 key2);

    void notify(endpoint *successor);

    string printInfo();

    int open_to_connection(u32 id, u32 port, int *sd);
    char try_serve_client(int client);
    int redirect_to(int client, u32 red_ip, u32 red_port);
    int serve_locally(int client, responseType type, char *response);
    void start_periodic_functions();
    void request_keys_periodic();
    void stabilize_periodic();
    void fix_fingers_periodic();
    void check_predecessor_periodic();
    void run();

    void *treat_node(int client);    
    void serve_find_successor_request(int client);
    void serve_get_predecessor_request(int client);
    void serve_get_successor_request(int client);
    void serve_notification_request(int client);
    void serve_keys_request(int client);

    void *send_find_successor_request(endpoint *to, u32 key);
    void *send_get_predecessor_request(endpoint *to);
    void *send_get_successor_request(endpoint *to);
    void *send_notification_request(endpoint *to);
    void *send_keys_request();
    

    void stabilize();
    void fix_fingers();
    void check_predecessor();
};

u32 sha1_hash(u32 port_, u32 &ip);
u32 sha1_hash(const string &s);
vector<string> parseCommand(const string &command);
#endif