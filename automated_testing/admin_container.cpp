#include <iostream>

// Socket libraries
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// Pthread library
#include <thread>

#include <vector>

#define PORT 476

using namespace std;

// Mutex para el server_response
pthread_mutex_t mutex_pthread = PTHREAD_MUTEX_INITIALIZER;

void create_containers(int nro_container, string container_name){
  int sock = 0, valread, i;
  struct sockaddr_in serv_addr;
  string command = "-c" + container_name;
  char server_reply[512];

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if(sock == -1){
    printf("Error al crear el socket\n");
    return;
  }

  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  //Connect to remote server
  if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    printf("Conexion fallida\n");
    return;
  }

  if(send(sock, command.c_str(), command.length(), 0) < 0){
    printf("Error al enviar, contenedor nro %d\n", nro_container);
    return;
  }

  memset(server_reply, 0, 512);
  if(recv(sock, server_reply, command.length(), 0) < 0){
    printf("Error al recibir, contenedor nro %d\n", nro_container);
    return;
  }

  printf("Contenedor %d creado...\n", nro_container);

  return;
}

void stop_containers(int nro_container, string container_name){
  int sock = 0, valread, i;
  struct sockaddr_in serv_addr;
  string command = "-s" + container_name;
  char server_reply[512];

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if(sock == -1){
    printf("Error al crear el socket\n");
    return;
  }

  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);


  //Connect to remote server
  if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    printf("Conexion fallida\n");
    return;
  }

  if(send(sock, command.c_str(), command.length(), 0) < 0){
    printf("Error al enviar, contenedor nro %d\n", nro_container);
    return;
  }

  memset(server_reply, 0, 512);
  if(recv(sock, server_reply, command.length(), 0) < 0){
    printf("Error al recibir, contenedor nro %d\n", nro_container);
    return;
  }

  printf("Contenedor %d detenido...\n", nro_container);

  return;
}

void delete_containers(int nro_container, string container_name){
  int sock = 0, valread, i;
  struct sockaddr_in serv_addr;
  string command = "-d" + container_name;
  char server_reply[512];

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);

  if(sock == -1){
    printf("Error al crear el socket\n");
    return;
  }

  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);


  //Connect to remote server
  if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
    printf("Conexion fallida\n");
    return;
  }

  if(send(sock, command.c_str(), command.length(), 0) < 0){
    printf("Error al enviar, contenedor nro %d\n", nro_container);
    return;
  }

  memset(server_reply, 0, 512);
  if(recv(sock, server_reply, command.length(), 0) < 0){
    printf("Error al recibir, contenedor nro %d\n", nro_container);
  }

  printf("Contenedor %d eliminado...\n", nro_container);

  return;
}

int main(int argc, char const *argv[]){
  int i, n;
  string container_name;

  cout << "Ingrese el numero n de contenedores a crear, detener y eliminar: ";
  cin >> n;
  cout << endl;

  thread threads[n];

  for(i = 0; i < n; i++){
    container_name = "container" + to_string(i + 1);

    pthread_mutex_lock(&mutex_pthread);
    threads[i] = thread(create_containers, i + 1, container_name);
    pthread_mutex_unlock(&mutex_pthread);
  }

  // Esperamos a que termine de crear el ultimo contenedor
  for(i = 0; i < n; i++){
    threads[i].join();
  }

  printf("\nTermino de crear\n\n");

  for(i = 0; i < n; i++){
    container_name = "container" + to_string(i + 1);

    pthread_mutex_lock(&mutex_pthread);
    threads[i] = thread(stop_containers, i + 1, container_name);
    pthread_mutex_unlock(&mutex_pthread);
  }

  // Esperamos a que termine de crear el ultimo contenedor
  for(i = 0; i < n; i++){
    threads[i].join();
  }

  printf("\nTermino de detener\n\n");

  for(i = 0; i < n; i++){
    container_name = "container" + to_string(i + 1);

    pthread_mutex_lock(&mutex_pthread);
    threads[i] = thread(delete_containers, i + 1, container_name);
    pthread_mutex_unlock(&mutex_pthread);
  }

  // Esperamos a que termine de crear el ultimo contenedor
  for(i = 0; i < n; i++){
    threads[i].join();
  }

  printf("\nTermino de borrar\n\n");

  return 0;
}
