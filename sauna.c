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
struct analiseReq{
	struct Request request;
	int index;
};

int vec_size; //tamanho do vetor 
int n_pessoas; //numero de pessoas na sauna

struct Request* requests;

void *request_func(void * arg){
//	if (n_pessoas == 5)
		//wait for a spot
	
	struct analiseReq ar = *(struct analiseReq *) arg;	
	requests[ar.index] = ar.request;
	usleep(requests[ar.index].t * 1000);	
	for (int i = ar.index-1; i < vec_size-1; i++)
	{
		//requests[ar.index] = requests[ar.index+1];
		requests[i] = requests[i+1];// acho que é assim
		n_pessoas--;
		printf("There is a clear spot in the sauna\n");
	}	
	
	pthread_t tid;
	tid=pthread_self();
	ar.request.tip = "SERVIDO";
	//mensag(ar.request, tid);
	
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

}

int main(int argc, char* argv[]){
	
	if(argc!=3){
		printf("Utilizacao: %s <n. lugares> <un. tempo>\n",argv[0]);
		exit(0);
	}
	
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
	
	struct Request r;		
	struct analiseReq ar;
	int n_lugares= atoi(argv[1]);	
	requests = malloc(n_lugares *sizeof(r));	
	vec_size = n_lugares;	
	int size,index = 0, n_pessoas = 0;
	pthread_t tid;
	tid=pthread_self();	
	puts("Waiting for Generator Data . . .");
	
	while( (size = read(fd_entrada, &r, sizeof(r))) > 0){
		
		printf("id: %d, genero: %c, duracao: %d, tip: %d\n", r.p, r.g, r.t,r.tip);
		ar.request = r;
		ar.index = index;
		ar.request.tip = "RECEBIDO";		
		//mensag(ar.request, tid);
		//a sauna está vazia
		//printf("pe: %d\n", n_pessoas);
		if (n_pessoas == 0){
			n_pessoas++;
			pthread_create(&tid, NULL, request_func, &ar);
			index++;
		}
		//existe alguém na sauna
		else{
			if (requests[0].g == r.g){
					n_pessoas++;
					pthread_create(&tid, NULL, request_func, &ar);
					index++;
				}
		//rejeita pedido
			else{
				ar.request.tip = "REJEITADO";				
				//mensag(ar.request, tid);
				
				int filedes = open("/tmp/bar.pid", O_WRONLY | O_SYNC | O_CREAT, 0660);
				char bf[LINE];
				write(fd_rejeitados, &r, sizeof(r));
				sprintf(bf,"%d - %d: %c - %d - %s\n",(int)getpid(), ar.request.p, ar.request.g, ar.request.t, ar.request.tip);
				write(filedes, bf,LINE);				
			}
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