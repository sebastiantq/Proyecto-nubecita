// En cuanto se levanta se conecta al suscribe_host
// Luego de enviar la ip y el puerto comienza a recibir peticiones del admin_container
// Crea, detiene y elimina contenedores

#include <stdio.h>
#include <stdlib.h>
#include <string.h>			//strlen
#include <signal.h> 		// signals
#include <unistd.h>			//write
#include <sys/prctl.h> 	// prctl(), PR_SET_PDEATHSIG
#include <arpa/inet.h>	//inet_addr
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "../libraries/hostshare.h"

// PThread
#include <pthread.h>

#ifndef SIZE
#define SIZE 512
#endif

#ifndef MAX_BUFFER_SCHEDULER
#define MAX_BUFFER_SCHEDULER 10
#endif

char* ip = "127.0.0.1";
int port = 478;

// Scheduler 1: 478
// Host 1: 479
// Scheduler 2: 480
// Host 2: 481

// Variables compartidas admin_container y new_connection para pthreads
int socket_desc, c, read_size;
struct sockaddr_in server, client;
char client_petition[512], container_name[512], server_response;

char petitions_list[MAX_BUFFER_SCHEDULER][512] = {""};

// Mutex para el petitions_list
pthread_mutex_t petitions_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void *new_petition(void *ptr);
void *scheduler();

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

void *new_petition(void *ptr){
  int active_connection = 1;
  long conn_client_sock = (long) ptr;
	puts("\nConexion establecida a scheduler1\n");

	while(active_connection){
		// memset(client_petition, 0, 512);

		if(recv(conn_client_sock, client_petition, 512, 0) > 0){
      puts(client_petition);

      // Seccion crítica
      pthread_mutex_lock(&petitions_list_mutex);

      // Agregamos la petición a la lista de peticiones
      for (size_t i = 0; i < MAX_BUFFER_SCHEDULER; i++) {
        if(strlen(petitions_list[i]) == 0){
          strcpy(petitions_list[i], client_petition);
          server_response = 1;
          break;
        }else{
          printf("Peticion: %s\n", petitions_list[i]);
          server_response = 0;
        }
      }

      pthread_mutex_unlock(&petitions_list_mutex);

      if(server_response){
        printf("\nCorrecto\n");
      }else{
        printf("\nScheduler lleno\n\n");
      }

			send(conn_client_sock, &client_petition, strlen(client_petition), 0);
		}else{
    	c = sizeof(struct sockaddr_in);

      pthread_exit(0);    /* used to terminate a thread */

    	// client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*) & c);

			// active_connection = 0;
		}
	}

  pthread_exit(0);    /* used to terminate a thread */
}

// C function showing how to do time delay
#include <stdio.h>
// To use time library of C
#include <time.h>

int send_command(host* info_host, char* command){
  int sock;
	struct sockaddr_in server;

	char server_reply[512];

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		printf("Could not create socket");
	}

	puts("Socket created");

	server.sin_addr.s_addr = inet_addr(info_host->ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(info_host->port);

	//Connect to remote server
	if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("connect failed. Error");
		return 0;
	}

	puts("Connected\n");

  //Send some data
  if(send(sock, command, 512, 0) < 0){
    puts("Send failed");
    return 0;
  }else{
    puts("send ok");
  }

  //Receive a reply from the server
  // memset(server_reply, 0, 512);

  if(recv(sock, server_reply, 512, 0) < 0){
    puts("recv failed");
  }else{
    puts("recv ok");
  }

  printf("Host reply: %s\n", server_reply);

	close(sock);

  return 1;
}

// Algoritmo de scheduling para los procesos en cola
void *scheduler(){
  int min = 9, pos = -1, scheduling = 1, ansSend = 0;

  host agent;

  strcpy(agent.ip, "127.0.0.1");
  agent.port = 479;
  agent.cont = 0;

  while (scheduling) {
    min = 9;
    pos = -1;

    // Seccion crítica
    pthread_mutex_lock(&petitions_list_mutex);

    // Agregamos la petición a la lista de peticiones
    for (size_t i = 0; i < MAX_BUFFER_SCHEDULER; i++) {
      if(strlen(petitions_list[i]) > 0){
        if(atoi(&petitions_list[i][2]) < min){ // seleccionamos el menor para sacar el de maxima prioridad
          min = atoi(&petitions_list[i][2]);
          pos = i;
        }
      }
    }

    if(pos != -1){
      ansSend = send_command(&agent, petitions_list[pos]);

      printf("Scheduling...\n");
      printf("Enviando comando: %s", petitions_list[pos]);

      strcpy(petitions_list[pos], "");
    }

    pthread_mutex_unlock(&petitions_list_mutex);

    // Delay de 10 segundos
    printf("Esperando delay...\n");
    sleep(10);
    printf("Delay finalizado\n");
  }

  pthread_exit(0);    /* used to terminate a thread */
}

void listen_admin_container(){
  pthread_t tid;  /* the thread identifier */

	int listen_connections = 1;
  long client_sock;

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc == -1){
		printf("Error al inicializar el servicio scheduler1");
	}

	puts("scheduler1 iniciado");

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

  client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

  // While para aceptar multiples conexiones
  while(listen_connections){
    if(client_sock < 0){
      printf("Error al establecer la conexion\n");
    }else{
      pthread_create(&tid, NULL, new_petition, (void *)client_sock);   /* create the thread */
    }

    client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
  }
}

int main(int argc, char *argv[]) {
  pthread_t schedulerid;  /* the thread identifier */

  printf("Scheduler levantado\n");

  awake();

  printf("\nRecibiendo peticiones...\n");

  pthread_create(&schedulerid, NULL, scheduler, NULL);   /* create the thread */

  listen_admin_container();

  return 1;
}
