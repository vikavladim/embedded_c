#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/server.h"

int main() {
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