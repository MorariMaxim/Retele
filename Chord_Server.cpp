#include "Chord_Server.h"

int ChordNode::find_successor(u32 key, endpoint *original)
{

    requestParams params;
    params.from = original;
    params.key = key;
    params.type = requestType::FIND_SUCCESSOR;

    endpoint *to;

    if (between(key, id, fingers[0].address->id))
    {
        to = fingers[0].address;
    }
    else
    {
        to = closest_preceding_node(key);
    }

    return send(to, params);
}

// incomplete
endpoint *ChordNode::closest_preceding_node(u32 key)
{

    endpoint *closest;
    for (u8 i = m_bits - 1; i >= 0; i--)
    {
    }

    return closest;
}

ChordNode::ChordNode(u32 port_, u32 ip_) : id(hash(port_, ip_)), fingers(fingerTable(id))
{
    this->address.port = port_;
    this->address.ip = ip_;
    this->address.id = this->id;
    predecessor = nullptr;
    fingers[0].address = &this->address;
}

ChordNode::ChordNode(u32 port_, u32 ip_, endpoint *other) : id(hash(port_, ip_)), fingers(fingerTable(id))
{
    this->address.port = port_;
    this->address.ip = ip_;
    this->address.id = this->id;
    predecessor = nullptr;

    requestParams params;
    params.from = &this->address;
    params.key = this->id;
    params.type = requestType::FIND_SUCCESSOR;
    fingers[0].address = (endpoint *)receive(other, params);
}

bool ChordNode::between(u32 key, u32 start, u32 end)
{
    if (start <= end)
    {
        return key > start && key <= end;
    }
    else
    {
        return key > start || key <= end;
    }
}

string u32_to_string(u32 ipAddressInteger);
u32 ChordNode::hash(u32 port_, u32 &ip)
{
    unsigned char hash[SHA_DIGEST_LENGTH];
    std::string s = u32_to_string(ip) + " " + to_string(port_);

    SHA1(reinterpret_cast<const unsigned char *>(s.c_str()), s.length(), hash);

    u32 key = (*(u32 *)hash) & mod;
    return key;
}

void ChordNode::stabilize()
{
    requestParams params;
    params.type = requestType::GET_PREDECESSOR;
    params.from = &this->address;

    auto s = fingers[0].address;
    if (s == nullptr)
        return;

    auto x = (endpoint *)receive(s, params);

    if (x != nullptr && between(x->id, id, s->id))
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

    requestParams params;
    params.type = requestType::FIND_SUCCESSOR;
    params.key = fingers[next].start;
    params.from = &this->address;

    endpoint *res = (endpoint *)receive(&this->address, params);

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

    send(successor, params);
}

int ChordNode::send(endpoint *to, requestParams type)
{
    return 0;
}

void *ChordNode::receive(endpoint *to, requestParams type)
{
    return 0;
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

string u32_to_string(u32 ipAddressInteger)
{
    in_addr ipAddress;
    ipAddress.s_addr = htonl(ipAddressInteger);

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