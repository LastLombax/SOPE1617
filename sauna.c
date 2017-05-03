#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <wait.h>
#include <signal.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define PEDIDO 0
#define REJEITADO 1
#define DESCARTADO 2

struct Request {
	int p; //número de série
	char g; //género do utilizador('F' ou 'M')
	int t; //duração da utilização pedida
	int tip; //estado do pedido
};

int vec_size, n_pessoas;

struct Request* requests;

void *request_func(void * arg){
//	if (n_pessoas == 5)
		//wait for a spot
	int aux = *(int *) arg;
	usleep(requests[aux].t * 1000);	
	for (int i = aux-1; i < vec_size-1; i++)
	{
		requests[aux] = requests[aux+1];
		n_pessoas--;
		printf("There is a clear spot in the sauna\n");
	}
	return NULL;		
}
	
	
int main(int argc, char* argv[]){
	
	if(argc!=3){
		printf("Utilizacao: %s <n. lugares> <un. tempo>\n",argv[0]);
		exit(0);
	}
	
	
	int size;
	struct Request r;
	pthread_t tid;
	
	int n_lugares= atoi(argv[1]);
	requests = malloc(n_lugares *sizeof(r));
	int aux = 0;
	int n_pessoas = 0;
	vec_size = n_lugares;
	//char un_tempo = *(char*) argv[2];//s,m, u - unidade de tempo
	int fd_entrada, fd_rejeitados;
	
	//criar fifos
	if (mkfifo("/tmp/entrada", 0660) == -1){
		perror("Erro na abertura do fifo de entrada\n");
		exit(3);
	}
	if(mkfifo("/tmp/rejeitados", 0660) == -1){
		perror("Erro na abertura do fifo de rejeitados\n");
		exit(4);
	}
	
	//Abrir fifos
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1)
	{
		perror("Erro na abertura do fifo de entrada");
		exit(3);
	}
	if ((fd_rejeitados=open("/tmp/rejeitados",O_RDWR)) == -1){
		perror("Erro na abertura do fifo de rejeitados");
		exit(4);
	}		
	puts("Waiting for Generator Data . . .");
	while( (size = read(fd_entrada, &r, sizeof(r))) > 0){
		printf("id: %d, genero: %c, duracao: %d, tip: %d\n", r.p, r.g, r.t,r.tip);
		//a sauna está vazia
		if (n_pessoas == 0){
			requests[aux] = r;
			n_pessoas ++;
			aux++;
		}
		//existe alguém na sauna
		else{
			if (requests[0].g == r.g){
					requests[aux] = r;		
					pthread_create(&tid, NULL, request_func, &aux);
					aux++;
				}
			//rejeita pedido
			else
				write(fd_rejeitados, &r, sizeof(r));
		}		
		
	}
	pthread_join(tid,NULL);
	//Close and delete fifos
	close(fd_entrada);
	close(fd_rejeitados);
	unlink("/tmp/entrada");
	unlink("/tmp/rejeitados");
	
	return 0;
}