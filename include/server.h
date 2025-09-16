#ifndef SERVER_H
#define SERVER_H

#include "client_list.h"

int setup_signals();
int initialize_server(client_manager_t *manager);
int create_raw_socket();
void cleanup_clients(client_manager_t *manager);
int run_server_loop(int sock, client_manager_t *manager);
void handle_close_connection(client_manager_t *manager, struct in_addr src_ip,
                             in_port_t src_port);
void handle_client_message(client_manager_t *manager, struct in_addr src_ip,
                           in_port_t src_port, const char *message);
void process_udp_payload(client_manager_t *manager, struct in_addr src_ip,
                         in_port_t src_port, const char *payload,
                         int payload_len);
void process_udp_packet(client_manager_t *manager, char *buffer, int len);

#endif