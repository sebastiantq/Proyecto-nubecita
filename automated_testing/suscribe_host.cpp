#include <iostream>

// Socket libraries
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// Pthread library
#include <thread>

#include "../libraries/hostshare.h"

#define PORT 476

using namespace std;

struct arg_struct {
    int nro_host;
};

void create_host(int port){
  int sock = 0, valread, i;
  struct sockaddr_in serv_addr;

  host message, server_reply;

  strcpy(message.ip, "127.0.0.1");
  message.port = port;
  message.cont = 0;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if(sock == -1){
    printf("Error al crear el socket\n");
    return;
  }

  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(477);

  //Connect to remote server
  if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    printf("Conexion fallida\n");
    return;
  }

  printf("Creando host con puerto %d...\n", port);
  if(send(sock, (struct host_struct*)&message, sizeof(message), 0) < 0){
    printf("Error al enviar, host con puerto %d\n", port);
    return;
  }

  memset((struct host_struct*)&server_reply, 0, sizeof(message));

  if(recv(sock, (struct host_struct*)&server_reply, sizeof(message), 0) < 0){
    printf("Error al recibir, host con puerto %d\n", port);
  }

  printf("Host con puerto %d creado...\n", port);

  pthread_exit(0);

  return;
}

int main(int argc, char const *argv[]){
  int iret, n, i;

  cout << "Numero n de hosts a levantar: ";
  cin >> n;
  cout << endl;

  thread threads[n];

  for(i = 478; i < 478 + n; i++){
    threads[i - 478] = thread(create_host, i);
  }

  // Esperamos a que termine de crear el ultimo contenedor
  for(i = 0; i < n; i++){
    threads[i].join();
  }

  printf("\nTermino de levantar hosts\n\n");

  return 0;
}
