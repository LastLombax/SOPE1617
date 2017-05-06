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

struct aux{
	struct Request r;
	int ind;
};


int n_lugares; //tamanho do vetor de semáforos
int n_pessoas=0; //numero de pessoas na sauna
struct Request* requests; //fila de pedidos
sem_t* semArray;//array com semáforos para cada lugar

int n_pedidos;

void *request_func(void * arg){	
	struct aux reInd = *(struct aux *) arg;			
	printf("entrou na thread %d\n", reInd.ind);
	sleep(reInd.r.t); //time in sauna
	
	reInd.r.tip = SERVIDO;	
	printf("There is a clear spot in the sauna\n");
	n_pessoas--;
	
	//pthread_t tid;
	//tid=pthread_self();	
	
	sem_post(&semArray[reInd.ind]);
	return NULL;		
}
	
/*void mensag(struct Request r, pthread_t tid){
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
*/
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
	if (mkfifo("/tmp/rejeitados", 0660) == -1){
		perror("Erro na criação do fifo de rejeitados\n");
		exit(4);
	}
	
	//Abrir fifos
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Erro na abertura do fifo de entrada");
		exit(3);
	}
	if ((fd_rejeitados=open("/tmp/rejeitados",O_RDWR)) == -1){
		perror("Erro na abertura do fifo de rejeitados");
		exit(4);
	}	
			
	n_lugares= atoi(argv[1]);
	int size, i, valid, l = 0;
	struct Request r;

	semArray = malloc(n_lugares*sizeof(*semArray));

	//initialização do array de semáforos
	for (i = 0; i < n_lugares; i++)
	     if(sem_init(&semArray[i], 0, 1) == -1){
			 perror("Erro na initialização do array de semáforos\n");
			 exit(5);
		 }
			
	
	//one thread per spot
	pthread_t threads[n_lugares];
		
	//pthread_t tid;
	//tid=pthread_self();

	//gets number of requests before getting the requests
	
	puts("Waiting for Generator Data . . .\n");
	read(fd_entrada, &n_pedidos, sizeof(int));
	requests = malloc(n_pedidos*sizeof(*requests));	
	
	int aux = n_pedidos;
	//saves requests into array
	while( (size = read(fd_entrada, &r, sizeof(r))) > 0 || l != aux-1 ){		
		r.tip = RECEBIDO;
		requests[l] = r;
		printf("id: %d, genero: %c, duracao: %d, tip: %d\n", r.p, r.g, r.t, r.tip);
		l++;
		if (l == aux){
		   close(fd_entrada);
		   break;
		}
	}
	
	
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Erro na abertura do fifo de entrada");
		exit(3);
	}
	
	printf("\nDealing with queue\n\n");
	sleep(2);

	int hasRejected = 0,index, currSize = n_pedidos;
	char gender;	
	
	
	//problema quando a fila vai ficar vazia e ele entra no hasRejected
	
	
	while (currSize > 0){
		printf("currSizeini: %d\n", currSize);
		if (requests[0].p == 0){
			printf("yiss\n");
			hasRejected = 0;
		}
		//verifica se pode acrescentar um pedido que foi rejeitado
		//se foi rejeitado, hasRejected = 1
		if (hasRejected)
		{ 
			printf("r: %d\n", hasRejected);
			size = read(fd_entrada, &r, sizeof(r));
			hasRejected = 0;
			if(size < 0)
			{
				perror("Erro na abertura do fifo de entrada");
				exit(3);
			}
			else if (size == 0)
				continue;
			else{
				//procura o primeiro local após o ultimo membro
				requests[currSize] = r;
				currSize++;
				}
		}
		for (index = 0; index < n_lugares; index++)
		{ 			
			if (n_pessoas > 0 && requests[0].g != gender){
				printf("rejected \n");
				requests[0].tip = REJEITADO;	
				hasRejected=1;
				currSize--;
				write(fd_rejeitados, &requests[0], sizeof(requests[0])); //write to fifo rejeitados
				for (i = 0; i < n_pedidos; i++)
					*(requests+i) = *(requests+i+1);
				requests[n_pedidos-1].p = 0;
				for (i = 0; i < n_pedidos; i++)					
					printf("id: %d\n", requests[i].p);	
				break;
			}						
			
			printf("n_pessoas: %d\n",n_pessoas);
			//se a sauna estiver cheia
			while(n_pessoas == n_lugares) {sleep(1); printf("parado\n");} //pára enquanto estiver cheio
		
			sem_getvalue(&semArray[index],&valid);
			
			//se a sauna estiver vazia
			if (n_pessoas == 0){	
					printf("sauna is empty\n");
					struct aux reInd;
					reInd.r = requests[0];
					reInd.ind = index;
					gender = requests[0].g;	
					n_pessoas++;
					currSize--;				
					sem_wait(&semArray[index]);
					pthread_create(&threads[index], NULL, request_func, &reInd);
				//shifts the requests
					for (i = 0; i < n_pedidos; i++)
						*(requests+i) = *(requests+i+1);
					requests[n_pedidos-1].p = 0;
					for (i = 0; i < n_pedidos; i++)					
						printf("id: %d\n", requests[i].p);
				break;
			}		
			//existe alguém na sauna		
			else if (requests[0].g == gender && valid == 1){
					printf("someone in \n");
					struct aux reInd;
					reInd.r = requests[0];
					reInd.ind = index;
					n_pessoas++;
					currSize--;	
					sem_wait(&semArray[index]);
					pthread_create(&threads[index], NULL, request_func, &reInd);	
					//shifts the requests
					for (i = 0; i < n_pedidos; i++)
						*(requests+i) = *(requests+i+1);
					requests[n_pedidos-1].p = 0;
					for (i = 0; i < n_pedidos; i++)					
						printf("id: %d\n", requests[i].p);		
				break;
			}			
		}
		printf("currSizefim: %d\n", currSize);
	}
	
	//espera por possíveis threads
	int v;
	for (v = 0; v < n_lugares;v++)
	{
		sem_getvalue(&semArray[v],&v); //guarda valor atual em v
		if (v == 0) //thread em execução
			pthread_join(threads[v],NULL);
	}
	puts("finished\n");
	//Close and delete fifos
	close(fd_entrada);
	close(fd_rejeitados);
	unlink("/tmp/entrada");
	unlink("/tmp/rejeitados");
	
	return 0;
}
