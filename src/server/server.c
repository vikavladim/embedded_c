#include "../include/server.h"

#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/client_list.h"
#include "../include/common.h"

static int server_running = 1;

void signal_handler(int sig) {
  (void)sig;
  server_running = 0;
  printf("\nServer shutting down...\n");
}

int setup_signals() {
  if (signal(SIGINT, signal_handler) == SIG_ERR ||
      signal(SIGTERM, signal_handler) == SIG_ERR) {
    perror("signal");
    return -1;
  }
  return 0;
}

int initialize_server(client_manager_t *manager) {
  printf("Starting Raw Socket Echo Server...\n");
  client_list_init(manager);
  server_running = 1;
  if (setup_signals() != 0) {
    return -1;
  }
  return 0;
}

void cleanup_clients(client_manager_t *manager) { client_list_clear(manager); }

int create_raw_socket() {
  int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
  if (sock < 0) {
    perror("socket");
    return -1;
  }
  struct sockaddr_ll sll;
  memset(&sll, 0, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_ifindex = if_nametoindex("lo");
  sll.sll_protocol = htons(ETH_P_IP);
  if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
    perror("bind");
    close(sock);
    return -1;
  }
  return sock;
}

void handle_close_connection(client_manager_t *manager, struct in_addr src_ip,
                             in_port_t src_port) {
  if (client_list_remove(manager, src_ip, src_port)) {
    printf("Client %s:%d disconnected\n", inet_ntoa(src_ip), src_port);
  }
}

void handle_client_message(client_manager_t *manager, struct in_addr src_ip,
                           in_port_t src_port, const char *message) {
  client_info_t *client = client_list_find(manager, src_ip, src_port);
  if (client == NULL) {
    client = client_list_add(manager, src_ip, src_port);
    if (client != NULL) {
      printf("New client: %s:%d\n", inet_ntoa(src_ip), src_port);
    } else {
      printf("Failed to add new client\n");
      return;
    }
  }
  client->message_count++;
  int response_size = strlen(message) + 20;
  char *response = malloc(response_size);
  if (response) {
    int written = snprintf(response, response_size, "%s %d", message,
                           client->message_count);
    if (written >= response_size) {
      response[response_size - 1] = '\0';
    }
    struct in_addr host_ip;
    inet_pton(AF_INET, HOST_IP, &host_ip);
    send_udp_raw(host_ip, PORT, src_ip, src_port, response, strlen(response));
    printf("Sent to %s:%d: %s\n", inet_ntoa(src_ip), src_port, response);
    free(response);
  }
}

void process_udp_payload(client_manager_t *manager, struct in_addr src_ip,
                         in_port_t src_port, const char *payload,
                         int payload_len) {
  char *message = malloc(payload_len + 1);
  if (!message) {
    return;
  }
  strncpy(message, payload, payload_len);
  message[payload_len] = '\0';
  if (strcmp(message, "CLOSE_CONNECTION") == 0) {
    handle_close_connection(manager, src_ip, src_port);
  } else {
    handle_client_message(manager, src_ip, src_port, message);
  }
  free(message);
}

void process_udp_packet(client_manager_t *manager, char *buffer, int len) {
  (void)len;
  struct iphdr *ip_header = (struct iphdr *)(buffer + 14);
  int ip_header_len = ip_header->ihl * 4;
  if (ip_header->protocol != IPPROTO_UDP) {
    return;
  }
  struct udphdr *udp_header = (struct udphdr *)(buffer + 14 + ip_header_len);
  if (ntohs(udp_header->dest) != PORT) {
    return;
  }
  char *payload = (char *)udp_header + sizeof(struct udphdr);
  int payload_len = ntohs(udp_header->len) - sizeof(struct udphdr);
  if (payload_len <= 0 || payload_len >= MAX_PAYLOAD_SIZE) {
    return;
  }
  if (!verify_udp_checksum(ip_header, udp_header)) {
    printf("Invalid UDP checksum from %s:%d, packet dropped\n",
           inet_ntoa((struct in_addr){ip_header->saddr}),
           ntohs(udp_header->source));
    return;
  }
  struct in_addr src_ip;
  src_ip.s_addr = ip_header->saddr;
  in_port_t src_port = ntohs(udp_header->source);
  process_udp_payload(manager, src_ip, src_port, payload, payload_len);
}

int run_server_loop(int sock, client_manager_t *manager) {
  char buffer[MAX_BUFFER_SIZE];
  printf("Server listening on port %d...\n", PORT);
  printf("Press Ctrl+C to stop the server\n");
  while (server_running) {
    int len = recv(sock, buffer, MAX_BUFFER_SIZE, 0);
    if (len > 0 && len >= 14) {
      process_udp_packet(manager, buffer, len);
    }
  }
  printf("Disconnecting all clients...\n");
  client_info_t *current = manager->head;
  while (current != NULL) {
    printf("Notifying client %s:%d...\n", inet_ntoa(current->ip),
           current->port);
    struct in_addr host_ip;
    inet_pton(AF_INET, HOST_IP, &host_ip);
    send_udp_raw(host_ip, PORT, current->ip, current->port, "CLOSE_CONNECTION",
                 strlen("CLOSE_CONNECTION"));
    current = current->next;
  }
  client_list_clear(manager);
  return 0;
}