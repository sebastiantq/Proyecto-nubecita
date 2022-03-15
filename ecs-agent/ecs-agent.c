// En cuanto se levanta se conecta al suscribe_host
// Luego de enviar la ip y el puerto comienza a recibir peticiones del admin_container
// Crea, detiene y elimina contenedores

#include <stdio.h>
#include <string.h>			//strlen
#include <signal.h> 		// signals
#include <unistd.h>			//write
#include <sys/prctl.h> 	// prctl(), PR_SET_PDEATHSIG
#include <arpa/inet.h>	//inet_addr
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "../libraries/hostshare.h"

#ifndef SIZE
#define SIZE 512
#endif

char* ip = "127.0.0.1";
int port = 478;

void awake(){
  char* suscribe_host_ip = "127.0.0.1";
  int suscribe_host_port = 477;

  int sock;
	struct sockaddr_in server;

	host message, server_reply;

  strcpy(message.ip, ip);
  message.port = port;
  message.cont = 0;

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		printf("Could not create socket");
	}

	puts("Socket created");

	server.sin_addr.s_addr = inet_addr(suscribe_host_ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(suscribe_host_port);

	//Connect to remote server
	if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("connect failed. Error");
		return;
	}

	puts("Connected\n");

  //Send some data
  if(send(sock, (struct host_struct*)&message, sizeof(message), 0) < 0){
    puts("Send failed");
    return;
  }else{
    puts("send ok");
  }

  //Receive a reply from the server
  memset((struct host_struct*)&server_reply, 0, sizeof(struct host_struct));

  if(recv(sock, (struct host_struct*)&server_reply, sizeof(message), 0) < 0){
    puts("recv failed");
  }else{
    puts("recv ok");
  }

  puts("Reply:\n");
  printf("Ip: %s\n", server_reply.ip);
  printf("Port: %d\n", server_reply.port);
  printf("Cont: %d\n\n", server_reply.cont);

	close(sock);

  return;
}

void print_containers(){
  pid_t pid;

	pid = fork();

	if(pid < 0){
		return;
	}else if(pid == 0){
    execlp("docker", "docker", "ps", NULL);
	}else{
		printf("\n");
		wait(NULL);
		printf("\n");
	}
}

int create_container(char* name){
  pid_t pid;

	pid = fork();

	if(pid < 0){
		return 0;
	}else if(pid == 0){
    execlp("docker", "docker", "run", "-id", "--name", name, "ubuntu:latest", "/bin/bash", NULL);
	}else{
		printf("Creando contenedor...\n");
		wait(NULL);
		printf("Contenedor creado\n");
	}

  print_containers();

  return 1;
}

int stop_container(char* name){
  pid_t pid;

	pid = fork();

	if(pid < 0){
		return 0;
	}else if(pid == 0){
    execlp("docker", "docker", "stop", name, NULL);
	}else{
		printf("Deteniendo contenedor...\n");
		wait(NULL);
		printf("Contenedor detenido\n");
	}

  print_containers();

  return 1;
}

int delete_container(char* name){
  pid_t pid;

	pid = fork();

	if(pid < 0){
		return 0;
	}else if(pid == 0){
    if(stop_container(name)){
      execlp("docker", "docker", "rm", name, NULL);
    }
	}else{
		printf("Eliminando contenedor...\n");
		wait(NULL);
		printf("Contenedor eliminado\n");
	}

  print_containers();

  return 1;
}

void listen_admin_container(){
  int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;
	char client_petition[512], server_response;

	int active_connection = 0;

  char container_name[512];

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc == -1){
		printf("Error al inicializar el servicio host1");
	}

	puts("host1 iniciado");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if(bind(socket_desc, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("Error al crear el socket");
		return;
	}

	puts("Socket iniciado");

	listen(socket_desc, 10);

	puts("Esperando por conexiones entrantes...");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*) & c);

	if(client_sock < 0){
		perror("Error al establecer la conexion");
		return;
	}

	active_connection = 1;
	puts("\nConexion establecida a host1\n");

	while(active_connection){
		// memset(client_petition, 0, 512);

		if(recv(client_sock, client_petition, 512, 0) > 0){
      puts(client_petition);
      strcpy(container_name, client_petition + 2);

			// Realizamos la accion dependiendo de la peticion del cliente
			if(client_petition[1] == 'c'){
  			printf("\n");
				server_response = create_container(container_name);
			}else if(client_petition[1] == 's'){
  			printf("\n");
				server_response = stop_container(container_name);
			}else if(client_petition[1] == 'd'){
  			printf("\n");
				server_response = delete_container(container_name);
			}else{
        server_response = 0;
      }

      if(server_response){
        printf("\nCorrecto\n");
      }else{
        printf("\nError\n\n");
      }

			send(client_sock, &client_petition, strlen(client_petition), 0);
		}else{
    	c = sizeof(struct sockaddr_in);

    	client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*) & c);

			// active_connection = 0;
		}
	}
}

int main(int argc, char *argv[]) {
  printf("Host levantado\n");

  awake();

  printf("\nRecibiendo peticiones...\n");

  listen_admin_container();

  return 1;
}
