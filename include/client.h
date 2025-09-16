#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <pthread.h>

#include "common.h"

extern int running;

void handle_signal(int sig);
void *receive_thread(void *arg);
int initialize_client(in_port_t *client_port);
int setup_signal_handlers(void);
int create_receive_thread(pthread_t *thread, in_port_t client_port);
int run_client_loop(in_port_t client_port);
void send_close_message(in_port_t client_port);

#endif