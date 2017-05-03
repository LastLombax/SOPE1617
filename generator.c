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

#define LINE 100

int fd_entrada;
int fd_rejeitados;
int max_utilizacao;

struct Request {
	int p;// numero de série do pedido
	char g;// género do utilizador ('F' ou 'M')
	int t;// duracao da utilizacao pedida
	int tip; // estado do pedido
};

void * generator_func(void * arg)
{
	int n = *(int*) arg; 
	//int filedes = open("/tmp/ger.pid", O_WRONLY,O_SYNC | O_CREAT | O_APPEND);
	int pinc=1;//incrementados de pedidos
	
	srand(time(NULL));
	int gen = rand()%2;
	int tim = rand()%max_utilizacao + 5  ;//5 minutos - min//60 min - max
	int militime= tim*60*1000;
	
	struct Request request;
	time_t rawtime;
   	struct tm *info;
	char hour_buffer[LINE];
	char bf[LINE];
	char tip_str[10];
	puts("here");
	while(n>0){
		time( &rawtime );
		info = localtime( &rawtime );
		sprintf(hour_buffer,"%d - %d - %d", info->tm_hour, info->tm_min,info->tm_sec);
		
		request.p = pinc++;
		if(gen==0) request.g = 'F';
		if(gen==1) request.g = 'M';
		request.t = militime;
		
		request.tip=PEDIDO;
		switch(request.tip){
			case PEDIDO:
				strcat(tip_str,"PEDIDO");
			break;
			case REJEITADO:
				strcat(tip_str,"REJEITADO");
			break;
			case DESCARTADO:
				strcat(tip_str,"DESCARTADO");
			break;
		}
		
		write(fd_entrada, &request, sizeof(request));
		printf("id: %d, genero: %c, duracao: %d, tip: %d\n", request.p, request.g, request.t,request.tip);
		sprintf(bf,"%s - %d - %d: %c - %d - %s\n",(char*)hour_buffer, (int)getpid(), request.p, request.g, request.t, tip_str);
		//write(filedes, bf,LINE);
		n--;
	}
	
	
	close(fd_entrada);
	return NULL;
}

void * rejected_func(void * arg)
{
	
	return NULL;
}

int main(int argc, char* argv[]){
	
	if(argc!=4){
		printf("Utilizacao: %s <n. pedidos> <max. utilizacao> <un. tempo>\n",argv[0]);
		exit(0);
	}
	
	int n_pedidos= atoi(argv[1]);
	int max_util= atoi(argv[2]);//mins
	max_utilizacao=max_util;
	//char un_tempo = *(char*)argv[3];//s,m, u - unidade de tempo/milisegundos	
		
	//Abrir fifos
	if((fd_entrada=open("/tmp/entrada",0660))==-1){
		perror("Erro na abretura fifo entrada\n");
		exit(1);		
	}
	
	if((fd_rejeitados=open("/tmp/rejeitados",0660))==-1){
		perror("Erro na abretura fifo rejeitados\n");
		exit(2);	
	}
	
	//Thread1 - Gerador de pedidos 
	pthread_t pid1;
	pthread_create(&pid1,NULL, generator_func,&n_pedidos);

	puts("11");
	/*
	//Thread2 - Pedidos Rejetados
	int pid2;
	pthread_create(&pid2,NULL, rejected_func,NULL);
	*/
	
	//Join de ThreadsS
	if(pthread_join(pid1,NULL)<0) perror("Erro no join do thread generator\n");
	
	//Close and delete fifos
	
	close(fd_rejeitados);
	return 0;	
}