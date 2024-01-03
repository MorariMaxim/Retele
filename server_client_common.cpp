#include "server_client_common.h"

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

std::string ip_port_to_string(u32 ip, u32 port, char byte_order)
{
    port = byte_order == NETWORK_BYTE_ORDER ? ntohs(port) : port;
    return u32_to_string(ip, byte_order) + ":" + to_string(port);
}

string print_sockaddr_in(sockaddr_in &addr)
{

    u32 port = addr.sin_port;
    u32 ip = addr.sin_addr.s_addr;

    return u32_to_string(ip, NETWORK_BYTE_ORDER) + ":" + to_string(ntohs(port));
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