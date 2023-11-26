#include "Chord_Server.h"

endpoint *ChordNode::find_successor(u32 key, endpoint *original)
{

    if (this->id == key)
        return &this->address;

    requestParams params;
    params.from = original;
    params.key = key;
    params.type = requestType::FIND_SUCCESSOR;

    endpoint *to = nullptr ;

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

void *ChordNode::request(endpoint *to, requestParams type)
{
    return nullptr;
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

string u32_to_string(u32 ipAddressInteger);
void ChordNode::stabilize()
{
    requestParams params;
    params.type = requestType::GET_PREDECESSOR;
    params.from = &this->address;

    auto s = fingers[0].address;
    if (s == nullptr)
        return;

    auto x = (endpoint *)request(s, params);

    if (x != nullptr && between(x->id, id, s->id) && x->id!=this->id && x->id != s->id)
    {
        s->id = x->id;
        s->port = x->port;
        s->ip = x->ip;
    }

    notify(s);
}

void ChordNode::fig_fingers()
{
    static u32 next = 0;

    next = (next + 1) & mod;

    endpoint *res = find_successor(fingers[next].start,&this->address);

    if(fingers[next].address == nullptr) {
        fingers[next].address = (endpoint *)malloc(sizeof(endpoint));
    }    
    fingers[next].address->id = res->id;
    fingers[next].address->ip = res->ip;
    fingers[next].address->port = res->port;
}

void ChordNode::printfInfo()
{

    printf("%d %d %d\n", this->id, this->address.ip, this->address.port);

    printf("Its finger table:\n");
    for (size_t i = 0; i < m_bits; i++)
    {
        printf("%d, %d\n", fingers[i].start, fingers[i].end);
    }
}

void ChordNode::run()
{
}

void ChordNode::notify(endpoint *successor)
{
    requestParams params;
    params.type = requestType::NOTIFICATION;
    params.key = this->id;
    params.from = &this->address;

    request(successor, params);
} 
 
u32 string_to_u32(string &ip)
{
    in_addr ipAddress;
    if (inet_aton(ip.c_str(), &ipAddress) != 0)
    {
        return ntohl(ipAddress.s_addr);
    }
    else
    {
        return 0;
    }
}

u32 sha1_hash(u32 port_, u32 &ip)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    std::string s = u32_to_string(ip) + " " + to_string(port_);

    SHA1(reinterpret_cast<const unsigned char *>(s.c_str()), s.length(), hash);

    u32 key = (*(u32 *)hash) & mod;
    return key;
}

string u32_to_string(u32 ipAddressInteger)
{
    in_addr ipAddress;
    ipAddress.s_addr = ipAddressInteger;

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