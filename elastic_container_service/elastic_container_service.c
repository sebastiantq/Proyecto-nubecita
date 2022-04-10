#include <stdio.h>
#include <string.h>			//strlen
#include <signal.h> 		// signals
#include <unistd.h>			//write
#include <sys/prctl.h> 	// prctl(), PR_SET_PDEATHSIG
#include <arpa/inet.h>	//inet_addr
#include <sys/socket.h>

// Shared memory
// Get
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/mman.h>

// Set
#include <fcntl.h>
#include <sys/stat.h>

#include "../libraries/hostshare.h"

// PThread
#include <pthread.h>

// Semaphore
#include <semaphore.h>

#define SHMSZ 27
#define MAX_HOSTS 10
#define MAX_CONT 100

int num_containers = 0, num_hosts = 0;
container contenedores[MAX_CONT];

/* name of the shared memory object */
const char *shared_memory_object = "HOSTS";

// Variables compartidas admin_container y new_connection para pthreads
int socket_desc_a, c_a;
struct sockaddr_in server_a, client_a;
char* server_response;

// Mutex para el server_response
pthread_mutex_t mutex_server_response = PTHREAD_MUTEX_INITIALIZER;

// Mutex para los new_host
pthread_mutex_t mutex_new_host = PTHREAD_MUTEX_INITIALIZER;

// Semaforo para el server_response
sem_t mutex_shared_memory;

// Variables compartidas suscribe_host y new_host para pthreads
int socket_desc_s, c_s, host_nro = 0;
struct sockaddr_in server_s, client_s;
host host_information;
host host_list[MAX_HOSTS];

void *new_connection(void *ptr); /* threads call this function */
void *new_host(void *ptr); /* threads call this function */

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

int send_hosts(host host_structure[MAX_HOSTS]){
  /* tche size (in bytes) of shared memory object */
  const int SM_SIZE = sizeof(struct host_struct) * MAX_HOSTS;
  /* shared memory file descriptor */
  int fd;
  /* pointer to shared memory obect */
  host *ptr;

  // sem_wait(&mutex_shared_memory);       /* down semaphore */

  fd = shm_open(shared_memory_object, O_CREAT | O_RDWR, 0666);

  if (fd == -1) {
    // sem_post(&mutex_shared_memory);       /* down semaphore */
    perror("open");
    return 10;
  }

  /* configure the size of the shared memory object */
  ftruncate (fd, SM_SIZE);

  ptr = (struct host_struct *) mmap (0, SM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /* write to the shared memory object */
  memcpy(ptr, host_structure, SM_SIZE);
  // sprintf(ptr, "%s",  host_structure);
  // ptr += sizeof(host_structure);

  printf("Shared memory enviado:\n\n");
  for (size_t i = 0; i < MAX_HOSTS; i++) {
    printf("Host %d:\n", i + 1);
    printf("Ip: %s\n", host_structure[i].ip);
    printf("Port: %d\n\n", host_structure[i].port);
  }

  // sem_post(&mutex_shared_memory);       /* up semaphore */

  // printf("\nSemaforo levantado shared memory\n");

	return 1;
}

host* receive_hosts(){
  /* the size (in bytes) of shared memory object */
  const int SM_SIZE = sizeof(struct host_struct) * MAX_HOSTS;
  /* shared memory file descriptor */
  int fd;
  /* pointer to shared memory obect */
  host *ptr;
  /* array to receive the data */
  host *data = malloc(SM_SIZE);

  // printf("ANTES\n");
  // sem_wait(&mutex_shared_memory);       /* down semaphore */
  // printf("DESPUES\n");

  fd = shm_open(shared_memory_object, O_RDONLY, 0666);

  if (fd == -1) {
    // sem_post(&mutex_shared_memory);       /* down semaphore */
    perror("open");
    return data;
  }

  /* configure the size of the shared memory object */
  ftruncate (fd, SM_SIZE);

  ptr = (struct host_struct *) mmap (NULL, SM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    // sem_post(&mutex_shared_memory);       /* down semaphore */
    perror("mmap");
    return data;
  }

  // place data into memory
  memcpy(data, ptr, SM_SIZE);

  printf("Shared memory recibido:\n\n");
  for (size_t i = 0; i < MAX_HOSTS; i++) {
    printf("Host %d:\n", i + 1);
    printf("Ip: %s\n", data[i].ip);
    printf("Port: %d\n\n", data[i].port);
  }

  // sem_post(&mutex_shared_memory);       /* down semaphore */

	return data;
}

void *new_host(void *ptr){
  int active_connection = 1;
  long conn_client_sock = (long) ptr;

	puts("\nConexion establecida a suscribe_host\n\n");

	while(active_connection && host_nro < MAX_HOSTS){
    pthread_mutex_lock(&mutex_new_host);

		memset((void*)&host_information, 0, sizeof(struct host_struct));

		if(recv(conn_client_sock, (struct host_struct*)&host_information, sizeof(struct host_struct), 0) > 0){
      // Recibimos los servidores que se levantan
      printf("Mensaje de host %d\n\n", host_nro + 1);
      printf("Ip: %s\n", host_information.ip);
      printf("Port: %d\n", host_information.port);
      printf("Cont: %d\n\n", host_information.cont);

      host_list[host_nro] = host_information;
      host_nro++;

      pthread_mutex_unlock(&mutex_new_host);

      /* remove the shared memory object */
      shm_unlink(shared_memory_object);

    	// Enviamos estos servidores a admin_container
      if(send_hosts(host_list)){
      	printf("Escrito (suscribe_host) en shared memory\n");
      }else{
      	printf("Error al escribir en shared memory (suscribe_host)\n");
      }

			send(conn_client_sock, (struct host_struct*)&host_information, sizeof(host_information), 0);
		}else{
    	c_s = sizeof(struct sockaddr_in);

      pthread_mutex_unlock(&mutex_new_host);

      pthread_exit(0);    /* used to terminate a thread */

    	// client_sock_s = accept(socket_desc_s, (struct sockaddr*)&client_s, (socklen_t*)&c_s);

			// active_connection = 0;
		}
	}

  printf("NUMERO MAXIMO DE HOSTS ALCANZADO\n");
}

void suscribe_host(){
  pthread_t tid;  /* the thread identifier */

	int listen_connections = 1;
  long client_sock_s;

	socket_desc_s = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc_s == -1){
		printf("\nError al inicializar el servicio suscribe_host");
	}

	puts("\nsuscribe_host iniciado");

	server_s.sin_family = AF_INET;
	server_s.sin_addr.s_addr = INADDR_ANY;
	server_s.sin_port = htons(477);

	if(bind(socket_desc_s, (struct sockaddr *)&server_s, sizeof(server_s)) < 0){
		perror("Error al crear el socket");
		return;
	}

	puts("Socket iniciado");

	listen(socket_desc_s, MAX_HOSTS);

	puts("Esperando por conexiones entrantes...");
	c_s = sizeof(struct sockaddr_in);

	client_sock_s = accept(socket_desc_s, (struct sockaddr*)&client_s, (socklen_t*)&c_s);

  // While para aceptar multiples conexiones
  while(listen_connections){
    server_response = "";

    if(client_sock_s < 0){
      printf("Error al establecer la conexion\n");
    }else{
    	pthread_create(&tid, NULL, new_host, (void *)client_sock_s);   /* create the thread */
    }

    client_sock_s = accept(socket_desc_s, (struct sockaddr *)&client_s, (socklen_t*)&c_s);
  }
}

char* create_container(char* name){
	char command[512];
  char* response;
  host* available_hosts;
  int min_containers = 0;

  available_hosts = receive_hosts();

  strcpy(command, "-c");
  strcat(command, name);

  min_containers = rand() % 2;

  // Nos conectamos al host escogido e interactuamos con él
  int r_send = send_command(&available_hosts[min_containers], command);

  if(r_send == 1){
    contenedores[num_containers].host = min_containers;

    available_hosts[min_containers].cont++;

  	response = "1";
  }else{
  	response = "0";
  }

	return response;
}

char* list_containers(){
	char* response = malloc(512);

  for(size_t i = 0; i < num_containers; i++){
    printf("Nombre contenedor: %s\n", contenedores[i].name);
    strcat(response, contenedores[i].name);
    strcat(response, "\n");
  }

  // strcat(response, '\0');
  printf("RESPONSE: %s\n", response);

	return response;
}

char* stop_container(char* name){
  char* response;
  char command[512];
  host* available_hosts;
  int cont_host = -1;

  available_hosts = receive_hosts();

  strcpy(command, "-s");
  strcat(command, name);

  printf("Comparando NOMBRES STOP: \n");
  for (size_t i = 0; i < MAX_CONT; i++) {
    if(!strcmp(contenedores[i].name, name)){
      cont_host = contenedores[i].host;
    }
  }

  if(cont_host != -1){
    // Nos conectamos al host escogido e interactuamos con él
    int r_send = send_command(&available_hosts[cont_host], command);

    if(r_send == 1){
    	response = "1";
    }else{
    	response = "0";
    }
  }else{
    printf("No se encontro un contenedor con ese nombre\n");
  }

	return response;
}

char* delete_container(char* name){
  char* response;
  char command[512];
  host* available_hosts;
  int cont_host = -1;

  available_hosts = receive_hosts();

  strcpy(command, "-d");
  strcat(command, name);

  printf("Comparando NOMBRES DELETE: \n");
  for (size_t i = 0; i < MAX_CONT; i++) {
    if(!strcmp(contenedores[i].name, name)){
      cont_host = contenedores[i].host;
    }
  }

  if(cont_host != -1){
    // Nos conectamos al host escogido e interactuamos con él
    int r_send = send_command(&available_hosts[cont_host], command);

    if(r_send == 1){
    	response = "1";
    }else{
    	response = "0";
    }
  }else{
    printf("No se encontro un contenedor con ese nombre\n");
  }

	return response;
}

void *new_connection(void *ptr){
  int active_connection = 1, receive;
  long conn_client_sock = (long)ptr;
  char client_petition[512], container_name[512];

  printf("\nConexion establecida a admin_container\n");

  while(active_connection){
    memset(client_petition, 0, 512);

    receive = recv(conn_client_sock, client_petition, 512, 0);

    if(receive > 0){
      strcpy(container_name, client_petition + 2);
      printf("Comando: %s", client_petition);

      // Realizamos la accion dependiendo de la peticion del cliente
      if(client_petition[1] == 'c'){
        if(num_containers < MAX_CONT){
          puts(client_petition);

          // Seccion crítica
          pthread_mutex_lock(&mutex_server_response);
          server_response = create_container(container_name);
          pthread_mutex_unlock(&mutex_server_response);

          strcpy(contenedores[num_containers].name, client_petition + 2);
          num_containers++;
        }else{
          printf("Error: Maximo numero de contenedores creados\n");
        }
      }else if(client_petition[1] == 'l'){
        puts(client_petition);

        // Seccion crítica
        pthread_mutex_lock(&mutex_server_response);
        server_response = list_containers();
        pthread_mutex_unlock(&mutex_server_response);
      }else if(client_petition[1] == 's'){
        puts(client_petition);

        // Seccion crítica
        pthread_mutex_lock(&mutex_server_response);
        server_response = stop_container(container_name);
        pthread_mutex_unlock(&mutex_server_response);
      }else if(client_petition[1] == 'd'){
        puts(client_petition);

        // Seccion crítica
        pthread_mutex_lock(&mutex_server_response);
        server_response = delete_container(container_name);
        pthread_mutex_unlock(&mutex_server_response);
      }

      printf("\nServer response: %s\n", server_response);
      send(conn_client_sock, server_response, 512, 0);
    }else{
      c_a = sizeof(struct sockaddr_in);

      pthread_exit(0);    /* used to terminate a thread */

      // active_connection = 0;
    }
  }

  pthread_exit(0);    /* used to terminate a thread */
}

void admin_container(){
	int listen_connections = 1;
  long client_sock_a;

  pthread_t tid;  /* the thread identifier */

	socket_desc_a = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc_a == -1){
		printf("Error al inicializar el servicio admin_container");
	}

	puts("admin_container iniciado");

	server_a.sin_family = AF_INET;
	server_a.sin_addr.s_addr = INADDR_ANY;
	server_a.sin_port = htons(476);

	if(bind(socket_desc_a, (struct sockaddr *)&server_a, sizeof(server_a)) < 0){
		perror("Error al crear el socket");
		return;
	}

	puts("Socket iniciado");

	listen(socket_desc_a, 100);

	puts("Esperando por conexiones entrantes...");
	c_a = sizeof(struct sockaddr_in);

  client_sock_a = accept(socket_desc_a, (struct sockaddr *)&client_a, (socklen_t*)&c_a);

  // While para aceptar multiples conexiones
  while(listen_connections){
    if(client_sock_a < 0){
      printf("Error al establecer la conexion\n");
    }else{
    	pthread_create(&tid, NULL, new_connection, (void *)client_sock_a);   /* create the thread */
    }

    client_sock_a = accept(socket_desc_a, (struct sockaddr *)&client_a, (socklen_t*)&c_a);
  }
}

int main(int argc, char *argv[]) {
	int id = fork();

	if(id == -1){
		perror("Ha habido un error al iniciar el servidor");
	}else if(id){ // Proceso padre, admin_container
		admin_container();
	}else{ 				// Proceso hijo, suscribe_host
		prctl(PR_SET_PDEATHSIG, SIGHUP); // SIGHUP: colgar la señal
																		 // PR_SET_PDEATHSIG: cuando el padre muera

    sem_init(&mutex_shared_memory, 0, 1);      /* initialize mutex to 1 - binary semaphore */
		suscribe_host();
    sem_destroy(&mutex_shared_memory);         /* destroy semaphore */
  }

	return 0;
}
