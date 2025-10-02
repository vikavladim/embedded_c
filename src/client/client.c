#include "client.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

int running = 1;
in_port_t CLIENT_PORT = 0;

void handle_signal(int sig) {
  (void)sig;
  running = 0;
}

int initialize_client(in_port_t *client_port) {
  printf("Starting Raw Socket Echo Client connecting to %s:%d...\n", HOST_IP,
         PORT);

  if (CLIENT_PORT == 0) {
    srand(time(NULL));
    *client_port = MIN_EPHEMERAL_PORT +
                   rand() % (MAX_EPHEMERAL_PORT - MIN_EPHEMERAL_PORT + 1);
    printf("Client using auto-selected source port: %d\n", *client_port);
  } else {
    *client_port = CLIENT_PORT;
    printf("Client using specified source port: %d\n", *client_port);
  }

  return 0;
}

int setup_signal_handlers(void) {
  if (signal(SIGINT, handle_signal) == SIG_ERR ||
      signal(SIGTERM, handle_signal) == SIG_ERR) {
    perror("signal");
    return -1;
  }
  return 0;
}

int create_raw_socket(void) {
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
  if (sock < 0) {
    perror("receive socket");
    return -1;
  }

  return sock;
}

int process_udp_packet(const char *buffer, in_port_t client_port) {
  struct iphdr *ip_header = (struct iphdr *)buffer;
  if (ip_header->protocol == IPPROTO_UDP) {
    int ip_header_len = ip_header->ihl * 4;
    struct udphdr *udp_header = (struct udphdr *)(buffer + ip_header_len);

    if (ntohs(udp_header->dest) == client_port) {
      char *payload = (char *)udp_header + sizeof(struct udphdr);
      int payload_len = ntohs(udp_header->len) - sizeof(struct udphdr);

      if (!verify_udp_checksum(ip_header, udp_header)) {
        printf("Invalid UDP checksum, packet dropped\n");
        return 0;
      }

      if (payload_len > 0 && payload_len < MAX_PAYLOAD_SIZE) {
        char *message = malloc(payload_len + 1);
        if (message) {
          strncpy(message, payload, payload_len);
          message[payload_len] = '\0';
          printf("Ответ сервера: %s\n", message);

          if (strcmp(message, "CLOSE_CONNECTION") == 0) {
            free(message);
            return 1;
          }
          free(message);
        }
      }
    }
  }
  return 0;
}

void *receive_thread(void *arg) {
  in_port_t client_port = *(in_port_t *)arg;
  int sock = create_raw_socket();
  if (sock < 0) {
    return NULL;
  }

  char buffer[MAX_BUFFER_SIZE];

  while (running) {
    int len = recv(sock, buffer, MAX_BUFFER_SIZE, 0);
    if (len > 0) {
      if (process_udp_packet(buffer, client_port)) {
        running = 0;
        printf("Server requested connection close. Shutting down...\n");
        break;
      }
    }
  }

  close(sock);
  return NULL;
}

int create_receive_thread(pthread_t *thread, in_port_t client_port) {
  if (pthread_create(thread, NULL, receive_thread, &client_port) != 0) {
    perror("pthread_create");
    return -1;
  }
  return 0;
}

int get_server_addresses(struct in_addr *server_ip, struct in_addr *host_ip) {
  if (inet_pton(AF_INET, HOST_IP, server_ip) != 1) {
    perror("inet_pton server_ip");
    return -1;
  }

  if (inet_pton(AF_INET, HOST_IP, host_ip) != 1) {
    perror("inet_pton host_ip");
    return -1;
  }

  return 0;
}

void send_message_to_server(const char *message, in_port_t client_port,
                            struct in_addr host_ip, struct in_addr server_ip) {
  char *msg_copy = malloc(strlen(message) + 1);
  if (msg_copy) {
    strcpy(msg_copy, message);
    send_udp_raw(host_ip, client_port, server_ip, PORT, msg_copy,
                 strlen(msg_copy));
    free(msg_copy);
  }
}

int run_client_loop(in_port_t client_port) {
  struct in_addr server_ip, host_ip;
  if (get_server_addresses(&server_ip, &host_ip) != 0) {
    return -1;
  }

  char *input_buffer = malloc(MAX_PAYLOAD_SIZE);
  if (!input_buffer) {
    perror("malloc");
    return -1;
  }

  printf("Client started. Enter messages ('exit' to quit):\n");

  while (running) {
    printf("> ");
    fflush(stdout);

    if (fgets(input_buffer, MAX_PAYLOAD_SIZE, stdin) == NULL) {
      break;
    }

    if (!running) {
      break;
    }

    size_t len = strlen(input_buffer);
    if (len > 0 && input_buffer[len - 1] == '\n') {
      input_buffer[len - 1] = '\0';
    }

    if (strcmp(input_buffer, "exit") == 0) {
      break;
    }

    send_message_to_server(input_buffer, client_port, host_ip, server_ip);
  }

  free(input_buffer);
  return 0;
}

void send_close_message(in_port_t client_port) {
  struct in_addr server_ip, host_ip;
  if (get_server_addresses(&server_ip, &host_ip) == 0) {
    printf("Sending CLOSE_CONNECTION to server...\n");
    send_udp_raw(host_ip, client_port, server_ip, PORT, "CLOSE_CONNECTION",
                 strlen("CLOSE_CONNECTION"));
  }
}