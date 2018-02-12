/*
twrap - Portable, protocol-agnostic TCP socket wrapper, primarily designed for client-server models in applications such as games.

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring
rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

//includes
#include "twrap.h" //file header
#ifdef _WIN32 //windows
    #define TWRAP_WINDOWS
    #include <ws2tcpip.h>
#else //unix
    #include <sys/socket.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif
#include <stddef.h> //NULL

//public functions
int twrapInit () {
    //initializes socket functionality, returns 0 on success
    #ifdef TWRAP_WINDOWS
        WSADATA WsaData;
        return (WSAStartup(MAKEWORD(2,2), &WsaData) != NO_ERROR);
    #else
        return 0;
    #endif
}
int twrapSocket (int block, int nagle, int mode, char* host, char* serv) {
    //protocol-agnostically creates a new socket configured according to the given parameters
    //sockets have to be created and bound/connected all at once to allow for protocol-agnosticity
    //int: Whether to make the socket blocking or not, either TWRAP_NOBLOCK or TWRAP_BLOCK
    //int: Whether to disable Nagle's algorithm or not, either TWRAP_NAGLE or TWRAP_NODELAY
    //int: Mode of the socket
    //  TWRAP_LISTEN: Listen on given address (or all interfaces if NULL) and port, e.g. for a server
    //  TWRAP_CONNECT: Immediately connect to given address (localhost if NULL), e.g. for a client
    //char*: Host/address as a string, can be IPv4, IPv6, etc...
    //char*: Service/port as a string, e.g. "1728" or "http"
    //returns socket handle or -1 on failure
    int sock, flags = (mode == TWRAP_LISTEN) ? AI_PASSIVE : 0;
    struct addrinfo* result, hint = {flags, AF_UNSPEC, SOCK_STREAM, 0, 0, NULL, NULL, NULL};
    //get address info
    if (getaddrinfo(host, serv, &hint, &result) != 0) return -1; //return -1 on error
    //create socket
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    #ifdef TWRAP_WINDOWS
        if (sock == INVALID_SOCKET) return -1; //return -1 on error
    #else
        if (sock == -1) return -1; //return -1 on error
    #endif
    //make sure IPV6_ONLY is disabled
    if (result->ai_family == AF_INET6) {
        int no = 0;
        setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&no, sizeof(no));
    }
    //set TCP_NODELAY based on argument
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&nagle, sizeof(nagle));
    //bind + listen if applicable
    if ((mode == TWRAP_LISTEN)&&((bind(sock, result->ai_addr, result->ai_addrlen))||(listen(sock, 64)))) {
        //close socket and return -1 on error
        twrapClose(sock);
        return -1;
    }
    //set non-blocking if needed
    if (block == TWRAP_NOBLOCK) {
        #ifdef TWRAP_WINDOWS
        DWORD no_block = 1;
        if (ioctlsocket(sock, FIONBIO, &no_block) != 0) {
        #else
        if (fcntl(sock, F_SETFL, O_NONBLOCK, 1) == -1) {
        #endif
            //close socket and return -1 on error
            twrapClose(sock);
            return -1;
        }
    }
    //connect if applicable
    if ((mode == TWRAP_CONNECT)&&(connect(sock, result->ai_addr, result->ai_addrlen))&&(block == TWRAP_BLOCK)) {
        //close socket and return -1 on error (only relevant if blocking)
        twrapClose(sock);
        return -1;
    }
    //free address info
    freeaddrinfo(result);
    //return socket handle
    return sock;
}
int twrapAccept (int sock, struct twrap_addr* addr) {
    //uses the given socket (must be TWRAP_LISTEN) to accept a new incoming connection, optionally returning its address
    //returns a socket handle for the new connection, or -1 on failure
    #ifdef TWRAP_WINDOWS
        int addr_size = sizeof(struct twrap_addr);
    #else
        socklen_t addr_size = sizeof(struct twrap_addr);
    #endif
    return accept(sock, (struct sockaddr*)addr, (addr) ? &addr_size : NULL);
}
int twrapAddress (int sock, struct twrap_addr* addr) {
    //writes the address the given socket is bound to into given address pointer, useful when automatically assigning ports
    //returns 0 on success, non-zero on failure
    #ifdef TWRAP_WINDOWS
        int addr_size = sizeof(struct twrap_addr);
    #else
        socklen_t addr_size = sizeof(struct twrap_addr);
    #endif
    return getsockname(sock, (struct sockaddr*)addr, &addr_size);
}
int twrapAddressInfo (struct twrap_addr* addr, char* host, int host_size, char* serv, int serv_size) {
    //writes the host/address and service/port of given address into given buffers (pointer + size), either buffer may be NULL
    //returns 0 on success, non-zero on failure
    return getnameinfo((struct sockaddr*)addr, sizeof(struct twrap_addr), host, host_size, serv, serv_size, 0);
}
int twrapSend (int sock, char* data, int data_size) {
    //uses the given socket (either TWRAP_CONNECT or returned by twrapAccept) to send given data (pointer + size)
    //returns how much data was actually sent (may be less than data size), or -1 on failure
    return send(sock, data, data_size, 0);
}
int twrapReceive (int sock, char* data, int data_size) {
    //receives data using given socket (either TWRAP_CONNECT or returned by twrapAccept) into given buffer (pointer + size)
    //returns the number of bytes received, or -1 on failure (e.g. if there is no data to receive)
    return recv(sock, data, data_size, 0);
}
int twrapSelect (int sock, double timeout) {
    //waits either until given socket has new data to receive or given time (in seconds) has passed,
    //returns 1 if new data is available, 0 if timeout was reached, and -1 on error
    fd_set set; struct timeval time;
    //fd set
    FD_ZERO(&set);
    FD_SET(sock, &set);
    //timeout
    time.tv_sec = timeout;
    time.tv_usec = (timeout - time.tv_sec)*1000000;
    //return
    return select(sock+1, &set, NULL, NULL, &time);
}
int twrapMultiSelect (int* socks, int socks_size, double timeout) {
    //waits either until a socket in given list has new data to receive or given time (in seconds) has passed,
    //returns 1 or more if new data is available, 0 if timeout was reached, and -1 on error
    fd_set set; struct timeval time; int sock_max = -1;
    //fd set
    FD_ZERO(&set);
    for (int i = 0; i < socks_size; i++) {
        if (socks[i] > sock_max) sock_max = socks[i];
        FD_SET(socks[i], &set);
    }
    //timeout
    time.tv_sec = timeout;
    time.tv_usec = (timeout - time.tv_sec)*1000000;
    //return
    return select(sock_max+1, &set, NULL, NULL, &time);
}
void twrapClose (int sock) {
    //closes the given socket
    #ifdef TWRAP_WINDOWS
        closesocket(sock);
    #else
        close(sock);
    #endif
}
void twrapTerminate () {
    //terminates socket functionality
    #ifdef TWRAP_WINDOWS
        WSACleanup();
    #endif
}