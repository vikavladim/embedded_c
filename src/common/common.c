#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char HOST_IP[16] = "127.0.0.1";
in_port_t PORT = 9090;

unsigned short checksum(void *b, int len) {
  unsigned short *buf = b;
  unsigned int sum = 0;
  unsigned short result;

  for (sum = 0; len > 1; len -= 2) {
    sum += *buf++;
  }
  if (len == 1) {
    sum += *(unsigned char *)buf;
  }
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  result = ~sum;
  return result;
}

int send_udp_raw(struct in_addr src_ip, in_port_t src_port,
                 struct in_addr dest_ip, in_port_t dest_port,
                 const char *message, int message_len) {
  if (!message || message_len < 0) {
    fprintf(stderr, "Invalid payload parameters\n");
    return -1;
  }

  const size_t packet_size =
      sizeof(struct iphdr) + sizeof(struct udphdr) + message_len;

  if (packet_size > MAX_BUFFER_SIZE) {
    fprintf(stderr, "Packet too large: %lu bytes (max: %lu)\n",
            (unsigned long)packet_size, (unsigned long)MAX_BUFFER_SIZE);
    return -1;
  }

  int send_sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (send_sock < 0) {
    perror("send socket");
    return -1;
  }

  int on = 1;
  if (setsockopt(send_sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
    perror("setsockopt");
    close(send_sock);
    return -1;
  }

  char *packet = malloc(packet_size);
  if (!packet) {
    perror("malloc");
    close(send_sock);
    return -1;
  }

  memset(packet, 0, packet_size);

  struct iphdr *ip_header = (struct iphdr *)packet;
  struct udphdr *udp_header = (struct udphdr *)(packet + sizeof(struct iphdr));
  char *udp_payload = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

  memcpy(udp_payload, message, message_len);

  udp_header->source = htons(src_port);
  udp_header->dest = htons(dest_port);
  udp_header->len = htons(sizeof(struct udphdr) + message_len);
  udp_header->check = 0;

  ip_header->ihl = 5;
  ip_header->version = 4;
  ip_header->tos = 0;
  ip_header->tot_len = htons(packet_size);
  ip_header->id = htons(getpid());
  ip_header->frag_off = 0;
  ip_header->ttl = 255;
  ip_header->protocol = IPPROTO_UDP;
  ip_header->check = 0;
  ip_header->saddr = src_ip.s_addr;
  ip_header->daddr = dest_ip.s_addr;

  ip_header->check = checksum(ip_header, sizeof(struct iphdr));

  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr = dest_ip;

  int send_result = sendto(send_sock, packet, packet_size, 0,
                           (struct sockaddr *)&dest_addr, sizeof(dest_addr));

  free(packet);
  close(send_sock);

  if (send_result < 0) {
    perror("sendto");
    return -1;
  }

  return 0;
}

int verify_udp_checksum(struct iphdr *ip_header, struct udphdr *udp_header) {
  if (udp_header->check == 0) {
    return 1;
  }

  struct {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_len;
  } phdr;

  phdr.src_ip = ip_header->saddr;
  phdr.dst_ip = ip_header->daddr;
  phdr.zero = 0;
  phdr.protocol = IPPROTO_UDP;
  phdr.udp_len = udp_header->len;

  size_t udp_len = ntohs(udp_header->len);
  size_t total_len = sizeof(phdr) + udp_len;
  if (total_len % 2 != 0) total_len++;

  char *checksum_data = malloc(total_len);
  if (!checksum_data) return 0;

  memcpy(checksum_data, &phdr, sizeof(phdr));
  memcpy(checksum_data + sizeof(phdr), udp_header, udp_len);

  if (total_len > sizeof(phdr) + udp_len) {
    checksum_data[total_len - 1] = 0;
  }

  unsigned short saved_checksum = udp_header->check;
  udp_header->check = 0;

  unsigned short calculated = checksum(checksum_data, total_len);
  udp_header->check = saved_checksum;

  free(checksum_data);

  return (saved_checksum == calculated);
}