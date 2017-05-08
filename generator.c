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
#include <signal.h>

#define PEDIDO 0
#define REJEITADO 1
#define DESCARTADO 2

#define LINE 100
#define HOUR_SIZE 26

int fd_entrada;
int fd_rejeitados;
int max_utilizacao;
clock_t start,end;
struct tms t;

struct Request {
	int p; //id number 
	char g; //user gender('F' or 'M')
	int t; //duration in sauna 
	int tip; //request state
	int rej;//number of rejections
};


//inst e pid têm valores estranhos nos dois ficheiros
//o pid no nome dos files é o identificador do processo que os cria, tem de ser metido
//falta escrever a ultima informação estatística no fim:
//nº pedidos gerados (total e por género), 
//nº de rejeiçõesrecebidas (total e por género) e
//nº de rejeições descartadas (total e por género).
//escrever as mensagens de cada pedido criado no generator.c no file ger.pid


void * generator_func(void * arg)
{
	int n = *(int*) arg; 
	int filedes = open("/tmp/ger.pid", O_WRONLY | O_SYNC | O_CREAT, 0660);
	int pinc=1;//request increment
	
	srand(time(NULL));
	int gen;
	int tim;
	int militime;
	float instant;
	long ticks =sysconf(_SC_CLK_TCK);
	struct Request request;	
	
	char bf[LINE];
	char *tip_str="PEDIDO";
	puts("here");
	while(n>0){
		
		gen = rand()%2;
		tim = rand()%max_utilizacao + 5  ;
		militime= tim;
		
		request.p = pinc++;
		if(gen==0) request.g = 'F';
		if(gen==1) request.g = 'M';
		request.t = militime;
		request.tip=PEDIDO;
		request.rej=0;
		end=times(&t);
		instant=(float)(end-start)/ticks;
		write(fd_entrada, &request, sizeof(request));
		printf("id: %d, genero: %c, duracao: %d, tip: %d, rej: %d\n", request.p, request.g, request.t,request.tip, request.rej);
		sprintf(bf,"%4.2f - %6d - %3d: %c - %9d - %10s\n",instant, (int)getpid(), request.p, request.g, request.t, tip_str);
		write(filedes, bf,LINE);
		n--;
	}
	close(fd_entrada);
	

	return NULL;
}

void * rejected_func(void * arg)
{
	int size;
	struct Request r;
	
	
	int filedes = open("/tmp/ger.pid", O_WRONLY | O_SYNC | O_CREAT, 0660);  //José
	char bf[LINE]; // José
	float instant; // José
	long ticks =sysconf(_SC_CLK_TCK); // José
	
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Error on opening the fifo entrada");
		exit(3);
	}

	while((size = read(fd_rejeitados, &r, sizeof(r))) > 0){
		if (r.p == -1){//about to end
			printf("No more rejected requests\n");
			return NULL;
		} 
		
		if(r.rej<3){
			
			end=times(&t); //José
			instant=(float)(end-start)/ticks; //José
			char *tip_str="REJEITADO"; //José
			printf("Rejeitado(%d) - id: %d, genero: %c, duracao: %d, tip: %d\n", r.rej, r.p, r.g, r.t,r.tip); //José
			sprintf(bf,"%4.2f - %6d - %3d: %c - %9d - %10s\n",instant, (int)getpid(), r.p, r.g, r.t, tip_str); //José
			write(filedes, bf,LINE); //José
			
			r.rej++;
			write(fd_entrada, &r, sizeof(r));
		}
		else{ //José
			end=times(&t); //José
			instant=(float)(end-start)/ticks; //José
			char *tip_str="DESCARTADO"; //José
			r.tip = DESCARTADO; //José
			printf("Descartado(%d) - id: %d, genero: %c, duracao: %d, tip: %d\n", r.rej, r.p, r.g, r.t,r.tip); //José
			sprintf(bf,"%4.2f - %6d - %3d: %c - %9d - %10s\n",instant, (int)getpid(), r.p, r.g, r.t, tip_str); //José
			write(filedes, bf,LINE); //José
		} //José
	}
	
	return NULL;
}

int main(int argc, char* argv[]){
	start = times(&t);
	if(argc!=3){
		printf("Usage: %s <n. pedidos> <max. utilizacao>\n",argv[0]);
		exit(0);
	}
	
	int n_pedidos= atoi(argv[1]);
	int max_util= atoi(argv[2]);//mins
	max_utilizacao=max_util;
	
	
	//open fifos
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Error on opening the fifo entrada");
		exit(3);
	}
	if ((fd_rejeitados=open("/tmp/rejeitados",O_RDWR)) == -1){
		perror("Error on opening the fifo rejeitados");
		exit(4);
	}	

	write(fd_entrada, &n_pedidos, sizeof(int));	

	//Thread1 - Request Generator 
	pthread_t pid1;
	if(pthread_create(&pid1,NULL, generator_func,&n_pedidos)<0) perror("Error creating the Generator Thread");
	if(pthread_join(pid1,NULL)<0) perror("Error on join of the Generator Thread");
	
	//Thread2 - Rejected Requests
	pthread_t pid2;
	if(pthread_create(&pid2,NULL, rejected_func,NULL)<0) perror("Error creating the Rejected Thread");
	if(pthread_join(pid2,NULL)<0) perror("Error on join of the Rejected Thread");
	
	//Close and delete fifos
	close(fd_rejeitados);
	unlink("/tmp/rejeitados");
	unlink("/tmp/entrada");
	return 0;	
}
