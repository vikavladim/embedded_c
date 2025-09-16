#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct client_info {
  struct in_addr ip;
  in_port_t port;
  int message_count;
  struct client_info *next;
} client_info_t;

typedef struct {
  client_info_t *head;
  int count;
} client_manager_t;

void client_list_init(client_manager_t *manager);
client_info_t *client_list_find(client_manager_t *manager, struct in_addr ip,
                                in_port_t port);
client_info_t *client_list_add(client_manager_t *manager, struct in_addr ip,
                               in_port_t port);
int client_list_remove(client_manager_t *manager, struct in_addr ip,
                       in_port_t port);
void client_list_clear(client_manager_t *manager);
void client_list_print(client_manager_t *manager);

#endif