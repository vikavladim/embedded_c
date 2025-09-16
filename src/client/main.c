#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/common.h"

int main() {
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