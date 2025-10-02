#include "client_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void client_list_init(client_manager_t *manager) {
  manager->head = NULL;
  manager->count = 0;
}

client_info_t *client_list_find(client_manager_t *manager, struct in_addr ip,
                                in_port_t port) {
  client_info_t *current = manager->head;
  while (current != NULL) {
    if (current->ip.s_addr == ip.s_addr && current->port == port) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

client_info_t *client_list_add(client_manager_t *manager, struct in_addr ip,
                               in_port_t port) {
  client_info_t *existing = client_list_find(manager, ip, port);
  if (existing != NULL) {
    return existing;
  }
  client_info_t *new_client = malloc(sizeof(client_info_t));
  if (!new_client) {
    return NULL;
  }
  new_client->ip = ip;
  new_client->port = port;
  new_client->message_count = 0;
  new_client->next = manager->head;
  manager->head = new_client;
  manager->count++;
  return new_client;
}

int client_list_remove(client_manager_t *manager, struct in_addr ip,
                       in_port_t port) {
  client_info_t *current = manager->head;
  client_info_t *prev = NULL;
  while (current != NULL) {
    if (current->ip.s_addr == ip.s_addr && current->port == port) {
      if (prev == NULL) {
        manager->head = current->next;
      } else {
        prev->next = current->next;
      }
      free(current);
      manager->count--;
      return 1;
    }
    prev = current;
    current = current->next;
  }
  return 0;
}

void client_list_clear(client_manager_t *manager) {
  client_info_t *current = manager->head;
  while (current != NULL) {
    client_info_t *next = current->next;
    free(current);
    current = next;
  }
  manager->head = NULL;
  manager->count = 0;
}

void client_list_print(client_manager_t *manager) {
  printf("Connected clients (%d):\n", manager->count);
  client_info_t *current = manager->head;
  int index = 1;
  while (current != NULL) {
    printf("  %d. %s:%d (messages: %d)\n", index++, inet_ntoa(current->ip),
           current->port, current->message_count);
    current = current->next;
  }
  if (manager->count == 0) {
    printf("  No clients connected\n");
  }
}