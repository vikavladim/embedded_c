#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "common.h"

void print_usage(const char *program_name) {
  printf("Usage: %s [<server_ip>] [<server_port>] [<client_port>]\n",
         program_name);
  printf("Defaults: server_ip=%s, server_port=%d, client_port=auto\n", HOST_IP,
         PORT);
  printf(
      "Client port: 0 for auto-select (%d-%d), or 1-65535 for specific port\n",
      MIN_EPHEMERAL_PORT, MAX_EPHEMERAL_PORT);
}

int main(int argc, char *argv[]) {
  if (argc > 1 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    print_usage(argv[0]);
    return 0;
  }

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
      fprintf(stderr,
              "Error: Invalid server port number: %s (must be 1-65535)\n",
              argv[2]);
      return 1;
    }
    PORT = (in_port_t)port;
  }

  if (argc >= 4) {
    char *endptr;
    long client_port = strtol(argv[3], &endptr, 10);
    if (*endptr != '\0' || client_port < MIN_EPHEMERAL_PORT ||
        client_port > MAX_EPHEMERAL_PORT) {
      fprintf(stderr,
              "Error: Invalid client port number: %s (must be 0 for auto or "
              "%d-%d)\n",
              argv[3], MIN_EPHEMERAL_PORT, MAX_EPHEMERAL_PORT);
      return 1;
    }
    CLIENT_PORT = (in_port_t)client_port;
  }

  if (argc > 4) {
    fprintf(stderr, "Error: Too many arguments\n");
    print_usage(argv[0]);
    return 1;
  }

  printf("Starting client:\n");
  printf("  Server: %s:%d\n", HOST_IP, PORT);
  if (CLIENT_PORT == 0) {
    printf("  Client port: auto-select (%d-%d)\n", MIN_EPHEMERAL_PORT,
           MAX_EPHEMERAL_PORT);
  } else {
    printf("  Client port: %d\n", CLIENT_PORT);
  }

  in_port_t client_port = 0;
  pthread_t thread;

  if (initialize_client(&client_port) != 0) {
    return 1;
  }

  if (setup_signal_handlers() != 0) {
    return 1;
  }

  if (create_receive_thread(&thread, client_port) != 0) {
    return 1;
  }

  int result = run_client_loop(client_port);

  send_close_message(client_port);

  running = 0;

  pthread_cancel(thread);
  pthread_join(thread, NULL);

  printf("Client stopped.\n");
  return result;
}