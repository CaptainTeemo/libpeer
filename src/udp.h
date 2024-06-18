#ifndef UDP_SOCKET_H_
#define UDP_SOCKET_H_

#include "address.h"

typedef struct UdpSocket UdpSocket;

struct UdpSocket {

  int fd;
  Address bind_addr;
  long long int timeout_sec;
  long int timeout_usec;
};

int udp_socket_create(UdpSocket *udp_socket, int family);

int udp_socket_open(UdpSocket *udp_socket, int family);

int udp_socket_bind(UdpSocket *udp_socket, Address *addr);

void udp_socket_close(UdpSocket *udp_socket);

int udp_socket_sendto(UdpSocket *udp_socket, Address *addr, const uint8_t *buf, size_t len);

int udp_socket_recvfrom(UdpSocket *udp_socket, Address *addr, uint8_t *buf, size_t len);

void udp_blocking_timeout(UdpSocket *udp_socket, long long int ms);

#endif // UDP_SOCKET_H_

