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

#define SHMSZ 27
#define MAX_HOSTS 2
#define MAX_CONT 100

int num_containers = 0;
container contenedores[MAX_CONT];

/* name of the shared memory object */
const char *shared_memory_object = "HOSTS";

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

  fd = shm_open(shared_memory_object, O_CREAT | O_RDWR, 0666);
  if (fd == -1) {
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

  fd = shm_open(shared_memory_object, O_RDONLY, 0666);
  if (fd == -1) {
    perror("open");
    return data;
  }

  /* configure the size of the shared memory object */
  ftruncate (fd, SM_SIZE);

  ptr = (struct host_struct *) mmap (NULL, SM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
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

	return data;
}

void suscribe_host(){
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;

  int host_nro = 0;
	host host_information;
  host host_list[MAX_HOSTS];

	int active_connection = 0;

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc == -1){
		printf("\nError al inicializar el servicio suscribe_host");
	}

	puts("\nsuscribe_host iniciado");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(477);

	if(bind(socket_desc, (struct sockaddr *) & server, sizeof(server)) < 0){
		perror("Error al crear el socket");
		return;
	}

	puts("Socket iniciado");

	listen(socket_desc, 10);

	char message[512] = "Prueba";

	puts("Esperando por conexiones entrantes...");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);

	if(client_sock < 0){
		perror("Error al establecer la conexion");
		return;
	}

	active_connection = 1;
	puts("\nConexion establecida a suscribe_host\n\n");

	while(active_connection && host_nro < MAX_HOSTS){
		memset((void*)&host_information, 0, sizeof(struct host_struct));

		if(recv(client_sock, (struct host_struct*)&host_information, sizeof(struct host_struct), 0) > 0){
			// Recibimos los servidores que se levantan
      printf("Mensaje de host %d\n\n", host_nro + 1);
      printf("Ip: %s\n", host_information.ip);
      printf("Port: %d\n", host_information.port);
      printf("Cont: %d\n\n", host_information.cont);

      host_list[host_nro] = host_information;
      host_nro++;

      /* remove the shared memory object */
      shm_unlink(shared_memory_object);

    	// Enviamos estos servidores a admin_container
      if(send_hosts(host_list)){
      	printf("Escrito (suscribe_host) en shared memory\n");
      }else{
      	printf("Error al escribir en shared memory (suscribe_host)\n");
      }

			send(client_sock, (struct host_struct*)&host_information, sizeof(host_information), 0);
		}else{
    	c = sizeof(struct sockaddr_in);

    	client_sock = accept(socket_desc, (struct sockaddr*)&client, (socklen_t*)&c);

			// active_connection = 0;
		}
	}

  printf("NUMERO MAXIMO DE HOSTS ALCANZADO\n");
}

char* create_container(char* name){
	char command[512];
  char* response;
  host* available_hosts;
  int min_containers = 0;

  available_hosts = receive_hosts();

  strcpy(command, "-c");
  strcat(command, name);

  for (size_t i = 0; i < MAX_HOSTS; i++) {
    if(available_hosts[i].cont < available_hosts[min_containers].cont){
      min_containers = i;
    }
  }

  // Nos conectamos al host escogido e interactuamos con él
  int r_send = send_command(&available_hosts[min_containers], command);

  if(r_send == 1){
    contenedores[num_containers].host = min_containers;

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

void admin_container(){
	int socket_desc, client_sock, c, read_size;
	struct sockaddr_in server, client;
	char client_petition[512], container_name[512];
  char* server_response;

	int active_connection = 0;

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if(socket_desc == -1){
		printf("Error al inicializar el servicio admin_container");
	}

	puts("admin_container iniciado");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(476);

	if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
		perror("Error al crear el socket");
		return;
	}

	puts("Socket iniciado");

	listen(socket_desc, 10);

	puts("Esperando por conexiones entrantes...");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

	if(client_sock < 0){
		perror("Error al establecer la conexion");
		return;
	}

	active_connection = 1;
	puts("\nConexion establecida a admin_container\n");

	while(active_connection){
		memset(client_petition, 0, 512);

		if(recv(client_sock, client_petition, 512, 0) > 0){
      strcpy(container_name, client_petition + 2);

			// Realizamos la accion dependiendo de la peticion del cliente
			if(client_petition[1] == 'c'){
        if(num_containers < MAX_CONT){
  				puts(client_petition);
  				server_response = create_container(container_name);
          strcpy(contenedores[num_containers].name, client_petition + 2);
          num_containers++;
        }else{
          printf("Error: Maximo numero de contenedores creados\n");
        }
			}else if(client_petition[1] == 'l'){
				puts(client_petition);
				server_response = list_containers();
			}else if(client_petition[1] == 's'){
				puts(client_petition);
				server_response = stop_container(container_name);
			}else if(client_petition[1] == 'd'){
				puts(client_petition);
				server_response = delete_container(container_name);
			}

      printf("\nServer response: %s\n", server_response);
			send(client_sock, server_response, 512, 0);
		}else{
    	c = sizeof(struct sockaddr_in);

    	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

			// active_connection = 0;
		}
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

		suscribe_host();
	}

	return 0;
}
