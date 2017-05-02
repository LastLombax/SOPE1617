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
		printf("Utilizacao: %s <n. lugares> <un. tempo>\n",argv[0], )
		exit(0);
	}
	
	int n_lugares= atoi(argv[1]);
	char un_tempo = argv[2];//s,m, u - unidade de tempo

	int fd_entrada;
	int fd_rejeitados;
	
	//criar fifos
	mkfifo("/tmp/entrada", 0660);
	mkfifo("/tmp/rejeitados", 0660);
	
	//Abrir fifos
	fd_entrada=open("/tmp/entrada",O_RDWR);
	fd_rejeitados=open("/tmp/rejeitados",O_RDWR);
	
	//gerar os pedidos aleatoriosS
	
	
	return 0;	
}