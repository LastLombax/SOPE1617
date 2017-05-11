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
#include <signal.h>

#define RECEBIDO 0
#define REJEITADO 1
#define SERVIDO 2

#define LINE 100


struct Request {
	int p; //id number 
	char g; //user gender('F' or 'M')
	int t; //duration in sauna 
	int tip; //request state
	int rej;//number of rejections
};

struct aux{
	struct Request r;
	int ind;
};

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int n_lugares; //semaphore array size and number of places in the sauna
int n_pessoas=0; //number of people in sauna
struct Request* requests; //request queue
struct Request* descarted; //for the descarted requests
sem_t* semArray;//semaphore array where each place of the sauna is controlled by a semaphore
int n_pedidos; //number of requests sent by generator
//to control instants for the register messages
clock_t start,end;
struct tms t;
int fd_entrada, fd_rejeitados; //file descriptors

int nRecebidos[2] = {0 , 0};
int nRejeitados[2] = {0 , 0};
int nServidos[2] = {0 , 0};
char file[20] = "/tmp/bal.";


void sighandler(int signalno){
	if (signalno == SIGINT)
	{
		printf("Program ended abruptally, shutting down...\n");
		close(fd_entrada);
		close(fd_rejeitados);
		unlink("/tmp/entrada");
		unlink("/tmp/rejeitados");
		exit(0);
	}
}

void *request_func(void * arg){	
	struct aux reInd = *(struct aux *) arg;			
	sleep(reInd.r.t); //time in sauna
	
	reInd.r.tip = SERVIDO;
	
	int filedes = open(file, O_WRONLY | O_SYNC | O_CREAT | O_APPEND, 0660);
	float instant;
	long ticks =sysconf(_SC_CLK_TCK);
	
	char bf[LINE];
	char *tip_str="SERVIDO";
	end=times(&t);
	instant=(float)(end-start)/ticks;
	sleep(1);
	sprintf(bf,"%4.2f - %6d - %lu - %3d: %c - %9d - %10s\n",instant, getpid(), pthread_self(), reInd.r.p, reInd.r.g, reInd.r.t, tip_str);
	write(filedes, bf,strlen(bf));
	
	if (reInd.r.g == 'M')
		nServidos[0] += 1;
	else
		nServidos[1] += 1;
	pthread_mutex_lock(&mut);
	printf("Request %d has been completed and ", reInd.r.p);	
	if (reInd.r.g == 'M')
		printf("he left the sauna!\n");
	else
		printf("she left the sauna!\n");
	n_pessoas--;
	pthread_mutex_unlock(&mut);
	sem_post(&semArray[reInd.ind]);
	return NULL;		
}
	

int main(int argc, char* argv[]){
	//in case something goes wrong, press Ctrl-C to close fifos and end program
	struct sigaction action;
	action.sa_handler = sighandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT,&action,NULL);	
	
	int pid = getpid();
	char spid[20];
	sprintf(spid, "%d", pid);
	strcat(file,spid);	
	int filedes = open(file, O_WRONLY | O_SYNC | O_CREAT | O_APPEND, 0660);
	
	float instant;
	long ticks =sysconf(_SC_CLK_TCK);	
	
	start = times(&t); //starts counting
	
	if(argc!=2){
		printf("Usage: %s <n. lugares>\n",argv[0]);
		exit(0);
	}
	
	//create fifos
	if (mkfifo("/tmp/entrada", 0660) == -1){
		perror("Error on creating the fifo entrada\n");
		exit(3);
	}
	if (mkfifo("/tmp/rejeitados", 0660) == -1){
		perror("Error on creating the fifo rejeitados\n");
		exit(4);
	}
	
	//open fifos
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Error on opening the fifo entrada");
		exit(3);
	}
	if ((fd_rejeitados=open("/tmp/rejeitados",O_RDWR)) == -1){
		perror("Error on opening the fifo rejeitados");
		exit(4);
	}	
			
	n_lugares= atoi(argv[1]);
	int size, i, valid, l = 0;
	struct Request r;

	semArray = malloc(n_lugares*sizeof(*semArray));

	//array initialization
	for (i = 0; i < n_lugares; i++)
	     if(sem_init(&semArray[i], 0, 1) == -1){
			 perror("Error on the initialization of the semaphore array\n");
			 exit(5);
		 }
		
	//one thread per spot
	pthread_t threads[n_lugares];
		
	//gets number of requests before getting the requests	
	puts("Waiting for Generator Data . . .\n");
	read(fd_entrada, &n_pedidos, sizeof(int));
	requests = malloc(n_pedidos*sizeof(*requests));	
	descarted = malloc(n_pedidos*sizeof(*descarted));
	//saves requests into array
	int aux = n_pedidos;
	while( (size = read(fd_entrada, &r, sizeof(r))) > 0 || l != aux-1 ){		
		r.tip = RECEBIDO;
		requests[l] = r;
	
		char bf[LINE];
		char *tip_str="PEDIDO";
		end=times(&t);
		instant=(float)(end-start)/ticks;
		sprintf(bf,"%4.2f - %6d - %lu - %3d: %c - %9d - %10s\n",instant, getpid(), pthread_self(), r.p, r.g, r.t, tip_str);
		write(filedes, bf,strlen(bf));
		
		if (r.g == 'M')
			nRecebidos[0] += 1;
		else
			nRecebidos[1] += 1;
		
		l++;
		if (l == aux){
		   close(fd_entrada);
		   break;
		}
	}
		
	if ((fd_entrada=open("/tmp/entrada",O_RDWR)) == -1){
		perror("Error on opening the fifo entrada");
		exit(3);
	}
	
	printf("\nDealing with queue\n");
	sleep(2);

	int hasRejected = 0,index, currSize = n_pedidos, flag = 0;
	char gender;		
	
	while (1){
		printf("\nCurrent queue status: \n");
		for (i = 0; i < currSize; i++)					
			printf("id: %d\n", requests[i].p);		
		
		for(i = 0; i < currSize; i++)
            if(requests[i].rej == 3)
               	flag++;   		
			

		//the loop breaks here
		if(flag == currSize) {
			printf("All the remaining requests will be rejected\n");
			for (i = 0; i < currSize; i++){	
				if (requests[i].g == 'M')
					nRejeitados[0] += 1;
				else
					nRejeitados[1] += 1;
				write(fd_rejeitados,&requests[i],sizeof(r));
			}				 
			r.p = -1;
			write(fd_rejeitados, &r, sizeof(r)); //write to fifo rejeitados to end 
			break;
		}
	
		flag = 0;
	
		//if someone was rejected, it will read from the fifo and insert the rejected request in the queue
		if (hasRejected) { 
			size = read(fd_entrada, &r, sizeof(r));
			hasRejected = 0;
			if(size < 0)
			{
				perror("Error on opening the fifo entrada");
				exit(3);
			}
			else if (size == 0)
				continue;
			else{
				//finds the element in the queue after the last member
				printf("Inserting rejected on the end of the queue\n");
				requests[currSize] = r;
				currSize++;
				}
		}
		
		for (index = 0; index < n_lugares; index++)	{ 		
			
			//rejects request
			if (n_pessoas > 0 && requests[0].g != gender){
				printf("Request rejected \n");
				requests[0].tip = REJEITADO;	
				hasRejected=1;
				currSize--;
				write(fd_rejeitados, &requests[0], sizeof(requests[0])); //write to fifo rejeitados
				
				char bf[LINE];
				char *tip_str="REJEITADO";
				end=times(&t);
				instant=(float)(end-start)/ticks;
				
				sprintf(bf,"%4.2f - %6d - %lu - %3d: %c - %9d - %10s\n",instant, getpid(), pthread_self(), requests[0].p, requests[0].g, requests[0].t, tip_str);
				write(filedes, bf,strlen(bf));
				
				if (requests[0].g == 'M')
					nRejeitados[0] += 1;
				else
					nRejeitados[1] += 1;
				
				//adjusts the queue
				for (i = 0; i < n_pedidos; i++)
					*(requests+i) = *(requests+i+1);
				requests[n_pedidos-1].p = 0;
				break;
			}						
			
			//stops execution while sauna is full
			if (n_pessoas == n_lugares){
				printf("The sauna is full, please wait for an available spot\n");
				while(n_pessoas == n_lugares) {sleep(1);} 
			}
			
			sem_getvalue(&semArray[index],&valid);
			
			//if sauna is empty
			if (n_pessoas == 0){	
					printf("Empty sauna: you can come in\n");
					struct aux reInd;
					reInd.r = requests[0];
					reInd.ind = index;
					gender = requests[0].g;	
					n_pessoas++;
					currSize--;				
					sem_wait(&semArray[index]);
					pthread_create(&threads[index], NULL, request_func, &reInd);
				
					//adjusts the queue
					for (i = 0; i < n_pedidos; i++)
						*(requests+i) = *(requests+i+1);
					requests[n_pedidos-1].p = 0;
				break;
			}		
			
			//there's someone in the sauna	
			else if (requests[0].g == gender && valid == 1){
					printf("You can enter\n");
					struct aux reInd;
					reInd.r = requests[0];
					reInd.ind = index;
					n_pessoas++;
					currSize--;	
					sem_wait(&semArray[index]);
					pthread_create(&threads[index], NULL, request_func, &reInd);	
				
					//adjusts the queue
					for (i = 0; i < n_pedidos; i++)
						*(requests+i) = *(requests+i+1);
					requests[n_pedidos-1].p = 0;					
				break;
			}
		}
		sleep(3);
	}
	printf("Checking if there's anyone left in the sauna...\n");
	
	int v, value;
	
	//waits for possible threads	
	for (v = 0; v < n_lugares;v++) {
		sem_getvalue(&semArray[v],&value); //current value of the semaphore v in value
		if (value == 0) //thread in execution		
			pthread_join(threads[v],NULL);		
	}
	puts("All requests were served. Thank you and come again!\n");
	
	sleep(3);
	printf("Here's today's statistics:\n");
	sleep(2);
	printf("Number of received requests: %d (%dM + %dF)\n", nRecebidos[0]+nRecebidos[1] , nRecebidos[0], nRecebidos[1]);
	printf("Number of rejected requests: %d (%dM + %dF)\n", nRejeitados[0]+nRejeitados[1] , nRejeitados[0], nRejeitados[1]);
	printf("Number of served requests: %d (%dM + %dF)\n", nServidos[0]+nServidos[1] , nServidos[0], nServidos[1]);
	
	printf("\nAll the information regarding the requests is in %s\n", file);

	//Close and delete fifos
	close(fd_entrada);
	close(fd_rejeitados);
	unlink("/tmp/entrada");
	unlink("/tmp/rejeitados");
	pthread_mutex_destroy(&mut);
	
	return 0;
}
