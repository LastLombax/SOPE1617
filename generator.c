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

int main(int argc, char* argv[]){
	
	if(argc!=3){
		printf("Utilizacao: %s <n. pedidos> <max. utilizacao> <un. tempo>\n",argv[0], )
		exit(0);
	}
	
	int n_pedidos= atoi(argv[1]);
	int max_util= atoi(argv[2]);
	char un_tempo = argv[3];//s,m, u - unidade de tempo
	int fd_entrada;
		
	//open fifo
	fd_entrada=open("/tmp/entrada",O_RDWR);
	fd_entrada=open("/tmp/rejeitados",O_RDWR);
	
	//gerar os pedidos aleatoriosS
	
	
	return 0;	
}