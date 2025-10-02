#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define MAX_PAYLOAD_SIZE 1024
#define MAX_BUFFER_SIZE 2048

extern char HOST_IP[16];
extern in_port_t PORT;

unsigned short checksum(void *b, int len);
int verify_udp_checksum(struct iphdr *ip_header, struct udphdr *udp_header);

int send_udp_raw(struct in_addr src_ip, in_port_t src_port,
                 struct in_addr dest_ip, in_port_t dest_port,
                 const char *payload, int payload_len);

#endif