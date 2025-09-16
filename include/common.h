#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define HOST_IP "127.0.0.1"
#define PORT 9090
#define MAX_PAYLOAD_SIZE 1024
#define MAX_BUFFER_SIZE 65536

unsigned short checksum(void *b, int len);
int verify_udp_checksum(struct iphdr *ip_header, struct udphdr *udp_header);

int send_udp_raw(struct in_addr src_ip, in_port_t src_port,
                 struct in_addr dest_ip, in_port_t dest_port,
                 const char *payload, int payload_len);

#endif