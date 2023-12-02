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
    bool operator==(const endpoint &other) const
    {
        if (port != other.port)
            return 0;
        if (ip != other.ip)
            return 0;
        if (id != other.id)
            return 0;
        return 1;
    }
    bool operator!=(const endpoint &other) const
    {
        return !(*this == other);
    }
    endpoint &operator=(const endpoint &other)
    {
        port = other.port;
        ip = other.ip;
        id = other.id;
        return *this;
    }
    endpoint(u32 ip_, u32 port_, u32 id_)
    {
        ip = ip_;
        port = port_;
        id = id_;
    }
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

    fingerTable(u32 id)
    {
        u32 x = 1;
        for (size_t i = 0; i < m_bits; i++)
        {
            fingers[i].start = (id + x) & mod;
            x <<= 1;
            fingers[i].end = (id + x - 1) & mod;
        }
    }
    friend class ChordNode;

public:
    finger &operator[](u_char index)
    {
        if (index >= m_bits)
            return fingers[0];
        return fingers[index];
    }
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

    data_store_(string address)
    {
        data_store_path = "./data_store/" + address;
        if (!directory_exists(data_store_path.c_str()))
        {
            if (create_directory(data_store_path.c_str()))
            {
            }
            else
            {
                fprintf(stderr, "error creating %s", data_store_path.c_str());
                exit(0);
            }
        }
    }
    std::string &operator[](const std::string &key)
    {
        return keys[key];
    }

    unique_lock<std::mutex> lock_data_store()
    {
        unique_lock<std::mutex> lock(data_store_mutex);
        return lock;
    }
    map::iterator begin()
    {
        return keys.begin();
    }

    map::iterator end()
    {
        return keys.end();
    }

    map::iterator find(const string &s)
    {
        return keys.find(s);
    }

    map::iterator erase(map::iterator &it)
    {
        auto dslock = lock_data_store();
        string output_file = path_from_key(it->first);
        remove(output_file.c_str());
        dslock.unlock();
        return keys.erase(it);
    }
    void insert(const string &key, const string &value)
    {
        auto dslock = lock_data_store();
        keys[key] = value;
        string output_file = path_from_key(key);

        remove(output_file.c_str());

        json data;
        data[key] = value;

        std::ofstream outputFile(output_file);
        outputFile << data.dump(2);
        outputFile.close();
        dslock.unlock();
    }
    string path_from_key(const string &key)
    {
        return data_store_path + "/" + to_string(sha1_hash(key)) + ".json";
    }
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

    endpoint *find_successor(u32 key, endpoint *original);

    endpoint *closest_preceding_node(u32 key);

    ChordNode(u32 ip_, u32 port_);

    ChordNode(u32 ip_, u32 port_, endpoint *other);

    void *request(endpoint *to, requestParams type);

    bool between(u32 key, u32 start, u32 end);

    u32 distance(u32 key1, u32 key2);

    void notify(endpoint *successor);

    string printInfo();

    int open_to_connection(u32 id, u32 port, int *sd);
    char try_serve_client(threadInfo *ti);
    int redirect_to(int client, u32 red_ip, u32 red_port);
    int serve_locally(int client, responseType type, char *response);
    void start_periodic_functions();
    void request_keys_periodic();
    void run();

    void serve_find_successor_request(threadInfo *ti);
    void serve_get_predecessor_request(threadInfo *ti);
    void serve_get_successor_request(threadInfo *ti);
    void serve_notification_request(threadInfo *ti);
    void serve_keys_request(threadInfo *ti);

    void *send_find_successor_request(endpoint *to, u32 key);
    void *send_get_predecessor_request(endpoint *to);
    void *send_get_successor_request(endpoint *to);
    void *send_notification_request(endpoint *to);
    void *send_keys_request();
};

string u32_to_string(u32 ipAddressInteger, char byte_order = NETWORK_BYTE_ORDER);
u32 string_to_u32(string &ip, char byte_order);
u32 sha1_hash(u32 port_, u32 &ip);
u32 sha1_hash(const string &s);
void *stabilize(void *arg);
void *fix_fingers(void *arg);
void *check_predecessor(void *arg);
int set_connection(u32 ip, u32 port);
string print_sockaddr_in(sockaddr_in &addr);
string ip_port_to_string(u32 ip, u32 port, char byte_order = HOST_BYTE_ORDER);
string sd_to_address(int sd);
vector<string> parseCommand(const string &command);
int read_string_from(int from, char *into, int size);
int send_string_to(int to, const char *from);
#endif