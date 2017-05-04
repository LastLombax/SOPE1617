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
#include <semaphore.h>

#define RECEBIDO 0
#define REJEITADO 1
#define SERVIDO 2

#define LINE 100


struct Request {
	int p; //número de série
	char g; //género do utilizador('F' ou 'M')
	int t; //duração da utilização pedida
	int tip; //estado do pedido
	int rej;//numero de rejeicoes
};

int vec_size; //tamanho do vetor 
int n_pessoas=0; //numero de pessoas na sauna
clock_t start,end;
struct tms t;

struct Request* requests;

sem_t* semArray;

void *request_func(void * arg){	
	int i;	
	int index = *(int *) arg;		
	n_pessoas++;	
	printf("pet: %d\n", n_pessoas);	
	usleep(requests[index].t * 1000);
	requests[index].tip = SERVIDO;
	for (i = index; i < vec_size; i++)
		requests[i] = requests[i+1];		

	printf("There is a clear spot in the sauna\n");
	n_pessoas--;
	
	//pthread_t tid;
	//tid=pthread_self();	
	
	sem_post(&semArray[index]);
	return NULL;		
}
	
void mensag(struct Request r, pthread_t tid){
	//guardar mensagens em bal.pid
	int filedes = open("/tmp/bal.pid", O_WRONLY,O_SYNC);
	
	time_t rawtime;
   	struct tm *info;
	char inst[LINE];
	char bf[LINE];
	char tip_str[10];
	int pid = (int) getpid();
	
	sprintf(inst,"%d - %d - %d", info->tm_hour, info->tm_min,info->tm_sec); //inst
	
	switch(r.tip){
		case RECEBIDO:		
			strcat(tip_str,"RECEBIDO");			
			break;				
		case REJEITADO:			
			strcat(tip_str,"REJEITADO");
			break;
		case SERVIDO:
			strcat(tip_str,"SERVIDO");
			break;
	}
	
	sprintf(bf,"%s - %d - %u - %d: %c - %d - %s\n",(char*)inst, pid, tid, r.p, r.g, r.t, r.tip);
	
	return NULL;

}

int main(int argc, char* argv[]){
	
	//start = times(&t); //starts counting
	
	if(argc!=2){
		printf("Utilizacao: %s <n. lugares>\n",argv[0]);
		exit(0);
	}
	
	int fd_entrada, fd_rejeitados;		
	//criar fifos
	if (mkfifo("/tmp/entrada", 0660) == -1){
		perror("Erro na criação do fifo de entrada\n");
		exit(3);
	}
	if(mkfifo("/tmp/rejeitados", 0660) == -1){
		perror("Erro na criação do fifo de rejeitados\n");
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
	
	struct Request r;		
	int n_lugares= atoi(argv[1]);	
	int last_pos = n_lugares-1;
	int size, i, index=0, valid;
	
	semArray = malloc(n_lugares*sizeof(r));
	requests = malloc(n_lugares*sizeof(r));	
	vec_size = n_lugares;	
	for (i = 0; i < vec_size; i++)
		sem_init(&semArray[i], 0, 1);
		
	

	pthread_t tid;
	tid=pthread_self();	
	
	puts("Waiting for Generator Data . . .");
	
	while( (size = read(fd_entrada, &r, sizeof(r))) > 0){
		r.tip = RECEBIDO;		
		printf("id: %d, genero: %c, duracao: %d, tip: %d\n", r.p, r.g, r.t,r.tip);
		printf("pem: %d\n", n_pessoas);
		
		if (index == last_pos)
			index = 0;
		
		//a sauna está vazia
		if (n_pessoas == 0){			
			index = 0;
			sem_wait(&semArray[index]);
			requests[index] = r;
			pthread_create(&tid, NULL, request_func, &index);
		}
		sem_getValue(sem_wait(&semArray[index]),&valid);
		//existe alguém na sauna		
		else if (n_pessoas > 0 && n_pessoas < vec_size && requests[0].g == r.g && valid == 1){
			index++;
			sem_wait(&semArray[index]);
			requests[index] = r;
			pthread_create(&tid, NULL, request_func, &index);
		}			
		else {
			r.tip = REJEITADO;			
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