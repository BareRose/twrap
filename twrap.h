/*
twrap - Portable, protocol-agnostic TCP socket wrapper, primarily designed for client-server models in applications such as games.

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring
rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software.
If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#ifndef TWRAP_H
#define TWRAP_H

//constants
#define TWRAP_NOBLOCK 0
#define TWRAP_BLOCK 1
#define TWRAP_LISTEN 0
#define TWRAP_CONNECT 1

//structs
struct twrap_addr {
    char data[128]; //enough space to hold any kind of address
};

//public functions
int twrapInit();
    //initializes socket functionality, returns 0 on success
int twrapSocket(int, int, char*, char*);
    //protocol-agnostically creates a new socket configured according to the given parameters
    //sockets have to be created and bound/connected all at once to allow for protocol-agnosticity
    //int: Whether to make the socket blocking or not, either TWRAP_NOBLOCK or TWRAP_BLOCK
    //int: Mode of the socket
    //  TWRAP_LISTEN: Listen on given address (or all interfaces if NULL) and port, e.g. for a server
    //  TWRAP_CONNECT: Immediately connect to given address (localhost if NULL), e.g. for a client
    //char*: Host/address as a string, can be IPv4, IPv6, etc...
    //char*: Service/port as a string, e.g. "1728" or "http"
    //returns socket handle or -1 on failure
int twrapAccept(int, struct twrap_addr*);
    //uses the given socket (must be TWRAP_LISTEN) to accept a new incoming connection, optionally returning its address
    //returns a socket handle for the new connection, or -1 on failure
int twrapSend(int, char*, int);
    //uses the given socket (either TWRAP_CONNECT or returned by twrapAccept) to send given data (pointer + size)
    //returns how much data was actually sent (may be less than data size), or -1 on failure
int twrapReceive(int, char*, int);
    //receives data using given socket (either TWRAP_CONNECT or returned by twrapAccept) into given buffer (pointer + size)
    //returns the number of bytes received, or -1 on failure (e.g. if there is no data to receive)
int twrapSelect(int, double);
    //waits either until given socket has new data to receive or given time (in seconds) has passed,
    //returns 1 if new data is available, 0 if timeout was reached, and -1 on error
int twrapMultiSelect(int*, int, double);
    //waits either until a socket in given list has new data to receive or given time (in seconds) has passed,
    //returns 1 or more if new data is available, 0 if timeout was reached, and -1 on error
void twrapClose(int);
    //closes the given socket
void twrapTerminate();
    //terminates socket functionality

#endif