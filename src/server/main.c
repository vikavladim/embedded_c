#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "server.h"

int main(int argc, char *argv[]) {
  if (argc >= 2) {
    struct in_addr test_addr;
    if (inet_pton(AF_INET, argv[1], &test_addr) != 1) {
      fprintf(stderr, "Error: Invalid IP address format: %s\n", argv[1]);
      return 1;
    }
    strncpy(HOST_IP, argv[1], sizeof(HOST_IP) - 1);
    HOST_IP[sizeof(HOST_IP) - 1] = '\0';
  }

  if (argc >= 3) {
    char *endptr;
    long port = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0' || port < 1 || port > 65535) {
      fprintf(stderr, "Error: Invalid port number: %s\n", argv[2]);
      return 1;
    }
    PORT = (in_port_t)port;
  }

  if (argc > 3) {
    printf("Usage: %s [<host_ip>] [<port>]\n", argv[0]);
    printf("Using: host_ip=%s, port=%d\n", HOST_IP, PORT);
  }

  client_manager_t manager;

  if (initialize_server(&manager) != 0) {
    return 1;
  }

  int sock = create_raw_socket();
  if (sock < 0) {
    cleanup_clients(&manager);
    return 1;
  }

  printf("Server listening on port %d...\n", PORT);
  printf("Press Ctrl+C to stop the server\n");

  int result = run_server_loop(sock, &manager);

  close(sock);
  cleanup_clients(&manager);
  printf("Server stopped.\n");

  return result;
}