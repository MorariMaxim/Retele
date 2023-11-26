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
    bool operator==(const endpoint &other)
    {
        if (port != other.port)
            return 0;
        if (ip != other.ip)
            return 0;
        if (id != other.id)
            return 0;
        return 1;
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

class ChordNode
{

    using map = unordered_map<u32, string>;

    u32 id;
    endpoint address;
    map keys;
    fingerTable fingers;

    endpoint *predecessor;
    friend class fingerTable;

public:
    endpoint *find_successor(u32 key, endpoint *original);

    endpoint *closest_preceding_node(u32 key);

    ChordNode(u32 ip_, u32 port_);

    ChordNode(u32 ip_, u32 port_, endpoint *other);

    void* request(endpoint *to, requestParams type);

    bool between(u32 key, u32 start, u32 end);

    u32 clockw_dist(u32 key1, u32 key2);

    u32 cclockw_dist(u32 key1, u32 key2);

    void stabilize();

    void notify(endpoint *successor);

    void fig_fingers();

    void run();

    void printfInfo();
};

#endif

string u32_to_string(u32 ipAddressInteger);
u32 string_to_u32(string &ip);
u32 sha1_hash(u32 port_, u32 &ip);