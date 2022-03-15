#ifndef HSIZE
#define HSIZE 512
#endif

typedef struct host_struct{
  char ip[HSIZE];
	int port;
  int cont;
} host;

typedef struct container_struct{
  char name[HSIZE];
	int host;
} container;
