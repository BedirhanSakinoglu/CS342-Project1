
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>  
#include <limits.h>
#include <math.h>

#define MAXTHREADS  500	    /* max number of threads */
#define MAXFILENAME 50		/* max length of a filename */

char **global_arguments;
//bool flag = false;
bool flag_io1 = true;
bool flag_io2 = true;
int total_process_count = -1;

// Declaration of thread condition variable
pthread_cond_t scheduler_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t process_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_io1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_io2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_waiting = PTHREAD_COND_INITIALIZER;

// declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t plock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t io1lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t io2lock = PTHREAD_MUTEX_INITIALIZER;

struct cpu {

};

struct ioDevice {
	int service_time;
};

struct pcb {
    int pid;
	int burst;
	int remaining_time;
    char *state;
};

//struct pcb t_args[MAXTHREADS];
//pthread_t tids[MAXTHREADS];	

//Ready Queue ---------------------------------------------
struct pcb ready_queue[MAXTHREADS];
int running_pid = -1;
int head_index = 0;
int tail_index = 0;

void insertqueue(struct pcb value){
    ready_queue[tail_index] = value;
    tail_index = tail_index + 1;
    printf("Inserted to ready queue: %d\n", value.pid);
}

struct pcb *dequeue(){
    struct pcb *value = &ready_queue[head_index];
    head_index = head_index + 1;
    printf("Dequeued: %d, State of Process: %s\n", value->pid, value->state);
    return value;
};

/*
void printqueue(char* attr, int index){
    
    if(strcmp(attr,"pid") == 0){
		printf("%d, ", ready_queue[index].pid);
	}
	else if(strcmp(attr,"burst") == 0){
		printf("%d, ", ready_queue[index].burst);
	}
	else if(strcmp(attr,"state") == 0){
		printf("%s, ", ready_queue[index].state);
	}
}
*/

int getburst(int index){
	return ready_queue[index].burst;
}

/*
int getpid(int index){
	return ready_queue[index].pid;
}
*/

char* getstate(int index){
	return ready_queue[index].state;
}
//---------------------------------------------------------

static void *process_task(void *pcb_ptr)
{
	//((struct pcb *) pcb_ptr)->state = "TERMINATED";
	//pthread_cond_broadcast(&scheduler_cond_var);
	
	char* retreason;
	
	char *algo = global_arguments[1];
	
	int remaining_time = ((struct pcb *) pcb_ptr)->remaining_time;
	char *state = ((struct pcb *) pcb_ptr)->state;

	double prob_terminate = atof(global_arguments[9]);
	double prob_io1 = atof(global_arguments[10]);
	double prob_io2 = atof(global_arguments[11]);

	srand ( time(NULL) );
	int random = rand()%100;
	printf("\nRANDOM: %d", random);
	bool check = false;

	printf("\n*********DIŞARISI********** running_pid: %d ", running_pid);
	printf("\n*********DIŞARISI********** pid: %d, state: %s\n", ((struct pcb *) pcb_ptr)->pid, ((struct pcb *) pcb_ptr)->state);

	pthread_mutex_lock(&plock);
	
	
	while(true){
		pthread_cond_broadcast(&scheduler_cond_var);
		printf("\nzır");
		pthread_cond_wait(&process_cond, &plock);
		printf("\nrunnig_pid--------------İÇERİSİ----------------: %d ", running_pid);
		printf("\npid--------------İÇERİSİ----------------: %d\n", ((struct pcb *) pcb_ptr)->pid);

		//BU CONDITION SAĞLANMIYOR
		if(((struct pcb *) pcb_ptr)->pid == running_pid){
			//_________________________________________________________________________________________________________________________________________________________________________

			printf("\n\nRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGINGRUNNIGNIGNIGNGING\n\n");
			if(strcmp(algo,"RR") == 0){
				int quantum = atoi(global_arguments[2]);
				if( remaining_time < quantum ){
					usleep(remaining_time*1000);
					//((struct pcb *) pcb_ptr)->state = "TERMINATED";
					check = true;
				}
				else if( remaining_time == quantum ){
					usleep(quantum*1000);
					//((struct pcb *) pcb_ptr)->state = "TERMINATED";
					check = true;
				}
				else{
					usleep(quantum*1000);
					((struct pcb *) pcb_ptr)->state = "READY";
					insertqueue(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
				}
			}
			else if( (strcmp(algo,"FCFS") == 0)  || (strcmp(algo,"SJF") == 0)){
				usleep(remaining_time*1000);
				//((struct pcb *) pcb_ptr)->state = "TERMINATED";
				check = true;
			}
			else{
				printf("\nERROR: Undefined Scheduling Algorithm");
				((struct pcb *) pcb_ptr)->state = "TERMINATED";
			}

			if(check){
				
				if( random < prob_terminate*100 ){
					//terminate
					printf("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");
					((struct pcb *) pcb_ptr)->state = "TERMINATED";
					total_process_count = total_process_count - 1;
					pthread_cond_broadcast(&scheduler_cond_var);
					pthread_exit("zaxDe---------");
				}
				else if( random < prob_terminate*100 + prob_io1*100 ){
					//io1
					pthread_mutex_lock(&io1lock);
					printf("INSIDE OF IO111111111111111111111111111\n");
					if(!flag_io1){
						printf("\nfffffffffff");
						pthread_cond_wait(&cond_io1, &io1lock);
						printf("\nrrrrrrrrrrrr");
					}
					else if(flag_io1){
						flag_io1 = false;
					}
					((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[3]));
					
					//*******************************************************************************
					if(strcmp(global_arguments[5],"fixed") == 0){
						((struct pcb *) pcb_ptr)->burst = atoi(global_arguments[6]);
						((struct pcb *) pcb_ptr)->remaining_time = atoi(global_arguments[6]);
					}
					else if(strcmp(global_arguments[5],"uniform") == 0){
						int max = atoi(global_arguments[8]);
						int min = atoi(global_arguments[7]);
						int random = rand() % (max-min);

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;
					}
					else if(strcmp(global_arguments[5],"exponential") == 0){
						double u;
						int random = -INT_MAX;
						
						while( (random < atoi(global_arguments[7])) || (random > atoi(global_arguments[8])) ){
							int lambda = atoi(global_arguments[6]);
							u  = (double)rand() / (RAND_MAX);
							random = -log(1-u)/lambda;
						}

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;

					}
					((struct pcb *) pcb_ptr)->state = "READY";
					insertqueue(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
					flag_io1 = true;
					pthread_cond_signal(&cond_io1);
					pthread_mutex_unlock(&io1lock);

					//*******************************************************************************

					pthread_cond_signal(&cond_io1);
					pthread_mutex_unlock(&io1lock);
				}
				else if( random < prob_terminate*100 + prob_io1*100 + prob_io2*100 ){
					//io2
					pthread_mutex_lock(&io2lock);
					printf("INSIDE OF IO22222222222222222222222222222\n");
					printf("\nflag io2: %d", flag_io2);
					if(!flag_io2){
						printf("\nfffffffffff");
						pthread_cond_wait(&cond_io2, &io2lock);
						printf("\nrrrrrrrrrrrrrrrr");
					}
					else if(flag_io2){
						flag_io2 = false;
					}
					((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[4]));
					
					//*******************************************************************************
					if(strcmp(global_arguments[5],"fixed") == 0){
						((struct pcb *) pcb_ptr)->burst = atoi(global_arguments[6]);
						((struct pcb *) pcb_ptr)->remaining_time = atoi(global_arguments[6]);
					}
					else if(strcmp(global_arguments[5],"uniform") == 0){
						int max = atoi(global_arguments[8]);
						int min = atoi(global_arguments[7]);
						int random = rand() % (max-min);

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;
					}
					else if(strcmp(global_arguments[5],"exponential") == 0){
						double u;
						int random = -INT_MAX;
						
						while( (random < atoi(global_arguments[7])) || (random > atoi(global_arguments[8])) ){
							int lambda = atoi(global_arguments[6]);
							u  = (double)rand() / (RAND_MAX);
							random = -log(1-u)/lambda;
						}

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;

					}
					//*******************************************************************************
					((struct pcb *) pcb_ptr)->state = "READY";
					insertqueue(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
					flag_io2 = true;
					pthread_cond_signal(&cond_io2);
					pthread_mutex_unlock(&io2lock);
				}
				//flag = true;
			}
			printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
			//_________________________________________________________________________________________________________________________________________________________________________
		}
	}

    // release lock
    pthread_mutex_unlock(&plock);
	
	/*
	char* retreason;
	
	char *algo = global_arguments[1];
	
	//int pid = ((struct pcb *) pcb_ptr)->pid;
	//int burst = ((struct pcb *) pcb_ptr)->burst;
	int remaining_time = ((struct pcb *) pcb_ptr)->remaining_time;
	char *state = ((struct pcb *) pcb_ptr)->state;

	double prob_terminate = atof(global_arguments[9]);
	double prob_io1 = atof(global_arguments[10]);
	double prob_io2 = atof(global_arguments[11]);

	

	int random;
	bool check = false;

	printf("SELAM ");
	
	while( strcmp(state,"TERMINATED") != 0 ){
		random = rand() % 100;

		if( strcmp(state,"RUNNING") == 0 ){
			printf("\n\nRUNNIGNIGNIGNGING\n\n");
			if(strcmp(algo,"RR") == 0){
				int quantum = atoi(global_arguments[2]);
				if( remaining_time < quantum ){
					usleep(remaining_time*1000);
					//((struct pcb *) pcb_ptr)->state = "TERMINATED";
					check = true;
				}
				else if( remaining_time == quantum ){
					usleep(quantum*1000);
					//((struct pcb *) pcb_ptr)->state = "TERMINATED";
					check = true;
				}
				else{
					usleep(quantum*1000);
					((struct pcb *) pcb_ptr)->state = "READY";
					insertqueue(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
				}
			}
			else if( (strcmp(algo,"FCFS") == 0)  || (strcmp(algo,"SJF") == 0)){
				usleep(remaining_time*1000);
				//((struct pcb *) pcb_ptr)->state = "TERMINATED";
				check = true;
			}
			else{
				printf("\nERROR: Undefined Scheduling Algorithm");
				((struct pcb *) pcb_ptr)->state = "TERMINATED";
			}

			if(check){
				pthread_cond_broadcast(&scheduler_cond_var);
				if( random < prob_terminate*100 ){
					//terminate
					((struct pcb *) pcb_ptr)->state = "TERMINATED";
				}
				else if( random < prob_terminate*100 + prob_io1*100 ){
					//io1
					pthread_mutex_lock(&io1lock);
					if(!flag_io1){
						pthread_cond_wait(&cond_io1, &io1lock);
					}
					else if(flag_io1){
						flag_io1 = false;
					}
					((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[3]));
					
					//*******************************************************************************
					if(strcmp(global_arguments[5],"fixed") == 0){
						((struct pcb *) pcb_ptr)->burst = atoi(global_arguments[6]);
						((struct pcb *) pcb_ptr)->remaining_time = atoi(global_arguments[6]);
					}
					else if(strcmp(global_arguments[5],"uniform") == 0){
						int max = atoi(global_arguments[8]);
						int min = atoi(global_arguments[7]);
						int random = rand() % (max-min);

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;
					}
					else if(strcmp(global_arguments[5],"exponential") == 0){
						double u;
						int random = -INT_MAX;
						
						while( (random < atoi(global_arguments[7])) || (random > atoi(global_arguments[8])) ){
							int lambda = atoi(global_arguments[6]);
							u  = (double)rand() / (RAND_MAX);
							random = -log(1-u)/lambda;
						}

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;

					}
					//*******************************************************************************

					pthread_cond_signal(&cond_io1);
					pthread_mutex_unlock(&io1lock);
				}
				else if( random < prob_terminate*100 + prob_io1*100 + prob_io2*100 ){
					//io2
					pthread_mutex_lock(&io2lock);
					if(!flag_io2){
						pthread_cond_wait(&cond_io2, &io2lock);
					}
					else if(flag_io2){
						flag_io2 = false;
					}
					((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[4]));
					
					//*******************************************************************************
					if(strcmp(global_arguments[5],"fixed") == 0){
						((struct pcb *) pcb_ptr)->burst = atoi(global_arguments[6]);
						((struct pcb *) pcb_ptr)->remaining_time = atoi(global_arguments[6]);
					}
					else if(strcmp(global_arguments[5],"uniform") == 0){
						int max = atoi(global_arguments[8]);
						int min = atoi(global_arguments[7]);
						int random = rand() % (max-min);

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;
					}
					else if(strcmp(global_arguments[5],"exponential") == 0){
						double u;
						int random = -INT_MAX;
						
						while( (random < atoi(global_arguments[7])) || (random > atoi(global_arguments[8])) ){
							int lambda = atoi(global_arguments[6]);
							u  = (double)rand() / (RAND_MAX);
							random = -log(1-u)/lambda;
						}

						((struct pcb *) pcb_ptr)->burst = random;
						((struct pcb *) pcb_ptr)->remaining_time = random;

					}
					//*******************************************************************************
					((struct pcb *) pcb_ptr)->state = "READY";
					insertqueue(*((struct pcb *) pcb_ptr));

					pthread_cond_signal(&cond_io2);
					pthread_mutex_unlock(&io2lock);
				}
				flag = true;
			}
		}
	}
	
	retreason = malloc (200); 
	strcpy (retreason, "normal termination of thread"); 
	
	printf("\nEXIT\n");
	pthread_exit(retreason);  // just tell a reason to the thread that is waiting in join
	*/
	
}

static void *generate_process(){
	pthread_t tids[MAXTHREADS];	/*thread ids*/
	int count;
	int random;		        /*number of threads*/
	struct pcb t_args[MAXTHREADS];	/*thread function arguments*/
	
	int maxp, allp;
	double pg;
	int i;
	int ret;
	char *retmsg;
	char* retreason;
	int counter = 0;
    
	//flag = false;
    count = 0;	/* number of threads to create initially*/
	
	//Create 10 threads immediately
	for (i = 0 ; i < count+1 ; ++i) {

		void* pcb_ptr = &(t_args[i]);

		t_args[i].pid = i+1;
		t_args[i].state = "READY";
		if(strcmp(global_arguments[5],"fixed") == 0){
			t_args[i].burst = atoi(global_arguments[6]);
			t_args[i].remaining_time = atoi(global_arguments[6]);
		}
		else if(strcmp(global_arguments[5],"uniform") == 0){
			int max = atoi(global_arguments[8]);
			int min = atoi(global_arguments[7]);
			int random = rand() % (max-min);

			t_args[i].burst = random;
			t_args[i].remaining_time = random;

		}
		else if(strcmp(global_arguments[5],"exponential") == 0){
			double u;
			int random = -INT_MAX;
			
			while( (random < atoi(global_arguments[7])) || (random > atoi(global_arguments[8])) ){
				int lambda = atoi(global_arguments[6]);
				u  = (double)rand() / (RAND_MAX);
				random = -log(1-u)/lambda;
			}

			t_args[i].burst = random;
			t_args[i].remaining_time = random;

		}

		ret = pthread_create(&(tids[counter]), NULL, process_task, (void *) &(t_args[i]));

		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		insertqueue(t_args[i]);
		printf("thread %i with tid %u created\n", i, (unsigned int) (unsigned int) tids[counter]);
		counter = counter+1;

		
		
		
	}

	//Parameters 12, 13, 14 -> pg, MAXP, ALLP
	allp = atoi(global_arguments[14]);
	maxp = atoi(global_arguments[13]);
	pg = atof(global_arguments[12]);

	printf("\nallp: %d", allp);
	printf("\nmaxp: %d", maxp);
	printf("\npg: %f", pg);

	/*
	//Create threads with probability
	if( count < maxp){
		int random;
		int coeff = 10;
		int multiplier = 1;
		
		while(pg < 0.1/multiplier){
			multiplier = multiplier*10;
		}
		
		coeff = coeff*multiplier;

		for (i = 0; i < allp; i++){
			usleep(5000);
		
			random = rand() % coeff;
			//rintf("random: %d", random);
			if(random < pg*coeff){
				//Create threads
				void* pcb_ptr = &(t_args[i+11]);

				t_args[i+11].pid = i+11;
				t_args[i+11].state = "READY";

				//*******************************************************************************
				if(strcmp(global_arguments[5],"fixed") == 0){
					//printf("\nFIXED");
					((struct pcb *) pcb_ptr)->burst = atoi(global_arguments[6]);
					((struct pcb *) pcb_ptr)->remaining_time = atoi(global_arguments[6]);
				}
				else if(strcmp(global_arguments[5],"uniform") == 0){
					int max = atoi(global_arguments[8]);
					int min = atoi(global_arguments[7]);
					int random = rand() % (max-min);

					((struct pcb *) pcb_ptr)->burst = random;
					((struct pcb *) pcb_ptr)->remaining_time = random;
				}
				else if(strcmp(global_arguments[5],"exponential") == 0){
					double u;
					int random = -INT_MAX;
						
					while( random < atoi(global_arguments[7]) || random > atoi(global_arguments[8]) ){
						int lambda = atoi(global_arguments[6]);
						u  = (double)rand() / (RAND_MAX);
						random = -log(1-u)/lambda;
					}

					((struct pcb *) pcb_ptr)->burst = random;
					((struct pcb *) pcb_ptr)->remaining_time = random;
				}
				//*******************************************************************************

				ret = pthread_create(&(tids[counter]), NULL, process_task, (void *) &(t_args[i+11]));
				if (ret != 0) {
					printf("thread create failed \n");
					exit(1);
				}
				printf("Additional thread %i with tid %u created\n", i, (unsigned int) (unsigned int) tids[counter]);
				insertqueue(t_args[i+11]);
				counter = counter +1;
				pthread_cond_broadcast(&scheduler_cond_var);
			}
		}
		total_process_count = counter;
	}
	printf("\n**********************************************************************************\n");
	*/
	total_process_count = 1; //sil bunu
	
	for (i = 0; i < counter; ++i) {
		printf("\nTERMINATION ");
	    ret = pthread_join(tids[i], NULL);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(1);
		}
		printf ("thread terminated");
		// we got the reason as the string pointed by retmsg
		// space for that was allocated in thread function; now freeing. 
		//free (retmsg); 
	}
	printf("main: all process threads terminated\n");
    retreason = malloc (200); 
	strcpy (retreason, "normal termination of thread"); 
	pthread_exit(retreason); 
	
}

static void *schedule(){
	// acquire a lock
	printf("SCHEDULE \n\n");
    pthread_mutex_lock(&lock);
	while(total_process_count != 0){
		printf("Waiting on condition variable scheduler_cond_var\n");
		pthread_cond_wait(&scheduler_cond_var, &lock);
		sleep(1);
		char *algo = global_arguments[1];
		printf("\n\nALGORITMA: %s", algo);
			
		//SCHEDULING ALGORITHMS
		printf("\n\n PASSED COND VAR \n\n");
		if( strcmp(algo,"FCFS") == 0){
			struct pcb *process = dequeue();
			//printf("**************************Dequeued: %d, State of Process: %s\n", process->pid, process->state);
			process->state = "RUNNING";
			running_pid = process->pid;
			printf("--------------------------Dequeued: %d, State of Process: %s\n", process->pid, process->state);
			pthread_cond_broadcast(&process_cond);
			printf("FCFS İÇERİSİ*****\n");
		}
		else if( strcmp(algo,"SJF") == 0 ){
			struct pcb process;
			int min_burst = INT_MAX;
			for(int i = head_index ; i < tail_index; i++){
				if(getburst(i) < min_burst){
					min_burst = getburst(i) ;
					process = ready_queue[i];
				}
			}
			process.state = "RUNNING";
		}
		else if( strcmp(algo,"RR") == 0){
			int quantum = atoi(global_arguments[2]);
			struct pcb *process = dequeue();
			process->state = "RUNNING";
			pthread_cond_broadcast(&process_cond);
		}
		else{
			printf("\nERROR: Undefined Scheduling Algorithm");
		}
	}

    // release lock
    pthread_mutex_unlock(&lock);
	
}

int main(int argc, char *argv[])
{
	
	global_arguments = argv;
	pthread_t generator_id, scheduler_id;
	pthread_create(&scheduler_id, NULL, schedule, NULL);
    pthread_create(&generator_id, NULL, generate_process, NULL);
	

    //---------------------------------------------------------------------------------------------------------------------------------------------------
    








    //-----------------------------------------------------------------------------------------------------------------------------------------------------
	
    printf("main: waiting all threads to terminate\n");
    
	pthread_join(generator_id, NULL);
    pthread_join(scheduler_id, NULL);

	printf("generator and scheduler threads terminated\n");
    
    return 0;
}
