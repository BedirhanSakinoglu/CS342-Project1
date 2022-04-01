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
#include <sys/time.h>

#define MAXTHREADS  500	    /* max number of threads */
#define MAXFILENAME 50		/* max length of a filename */
#define ANALYSISSIZE 30		/* 30 threads to analyze*/

char **global_arguments;
bool flag_io1 = true;
bool flag_io2 = true;
int total_process_count = 0;
int running_pid = -1;
int analysis_count = 0;
all_finish_times[ANALYSISSIZE];
all_turnaround_times[ANALYSISSIZE];
all_waiting_times[ANALYSISSIZE];

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
	int arrival_time;
	int finish_time;
	int turnaround_time;
	int total_wait_time;
};


// Structure of a linked list
struct node {
    struct pcb my_pcb;
    struct node* next;
};
 
//Head node
struct node* head = NULL;
struct timeval current_time;
 
void addatlast(struct pcb value)
{

    // Initialize a new node
	struct node* p;
    struct node* temp;
    temp = (struct node*)malloc(sizeof(struct node));
	temp->my_pcb = value;
	temp->next = NULL;

    if (head == NULL) {
		head = temp;
    }
 
    else {
		p  = head;//assign head to p 
        while(p->next != NULL){
            p = p->next;
        }
        p->next = temp;
    }
}

struct pcb *remove_head() {
    if (head == NULL) {
        return NULL;
	}

    struct node* temp = head;
    head = head->next;
 
    return temp;
}

void remove_by_id(int id_value)
{
	struct node* current = head;	
	struct node* prev = head;
	
	if (head == NULL)
	{
		return;
	}
	else if(head->next == NULL) {
		head = NULL;
		return;
	}
	else {
		while(current != NULL) {
			if(current->my_pcb.pid != id_value) {
				prev = current;
				current = current->next;
			}
			else {
				prev = current->next;
				break;
			}
		}
	}
}

void deleteNode(struct node** head_ref, int key)
{
    // Store head node
    struct node *temp = *head_ref, *prev;

    if (temp != NULL && temp->my_pcb.pid == key) {
        *head_ref = temp->next; 
        free(temp); 
        return;
    }
 
    while (temp != NULL && temp->my_pcb.pid != key) {
        prev = temp;
        temp = temp->next;
    }
 
    // If key was not present in linked list
    if (temp == NULL)
        return;
 
    // Unlink the node from linked list
    prev->next = temp->next;
 
    free(temp);
}


static void *process_task(void *pcb_ptr)
{
	bool check = false;
	char* retreason;
	char *algo = global_arguments[1];
	int remaining_time = ((struct pcb *) pcb_ptr)->remaining_time;
	char *state = ((struct pcb *) pcb_ptr)->state;

	double prob_terminate = atof(global_arguments[9]);
	double prob_io1 = atof(global_arguments[10]);
	double prob_io2 = atof(global_arguments[11]);

	pthread_mutex_lock(&plock);
	pthread_cond_broadcast(&scheduler_cond_var);
	
	/*
	BURADA SORUN VAR
	broadcast yaptıktan sonra aşağıdaki wait'e (--pthread_cond_wait(&process_cond, &plock);--) gitmeden schedule işini bitirirse kod patlıyor. 
	*/

	while(true){
		gettimeofday(&current_time, NULL);
		pthread_cond_wait(&process_cond, &plock);


		//RR çalışınca tam burada takılıyor ama anlamadım!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		srand ( time(NULL) );
		int random = rand()%100;
		
		if(((struct pcb *) pcb_ptr)->pid == running_pid){

			printf("\nAN ALGORITHM IS RUNNING NOW\n");
			if(strcmp(algo,"RR") == 0){
				int quantum = atoi(global_arguments[2]);
				if(((struct pcb *) pcb_ptr)->remaining_time < quantum ){
					usleep(remaining_time*1000);
					((struct pcb *) pcb_ptr)->remaining_time = 0;
					check = true;
				}
				else if(((struct pcb *) pcb_ptr)->remaining_time == quantum ){
					usleep(quantum*1000);
					((struct pcb *) pcb_ptr)->remaining_time = 0;
					check = true;
				}
				else{
					usleep(quantum*1000);
					((struct pcb *) pcb_ptr)->state = "READY";
					((struct pcb *) pcb_ptr)->remaining_time = ((struct pcb *) pcb_ptr)->remaining_time - quantum;
					addatlast(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
				}
			}
			else if( (strcmp(algo,"FCFS") == 0)  || (strcmp(algo,"SJF") == 0)){
				usleep(remaining_time*1000);
				check = true;
			}
			else{
				printf("\nERROR: Undefined Scheduling Algorithm");
				((struct pcb *) pcb_ptr)->state = "TERMINATED";
			}

			if(check){
				
				if( random < prob_terminate*100 ){
					//terminate

					int terminatation_time = current_time.tv_usec;
					((struct pcb *) pcb_ptr)->state = "TERMINATED";
					((struct pcb *) pcb_ptr)->finish_time = terminatation_time + ((struct pcb *) pcb_ptr)->total_wait_time;
					((struct pcb *) pcb_ptr)->turnaround_time = terminatation_time -((struct pcb *) pcb_ptr)->arrival_time;
					
					if(analysis_count < ANALYSISSIZE) {
						all_finish_times[analysis_count] = ((struct pcb *) pcb_ptr)->finish_time;
						all_waiting_times[analysis_count] = ((struct pcb *) pcb_ptr)->total_wait_time;
						all_turnaround_times[analysis_count] = ((struct pcb *) pcb_ptr)->finish_time - ((struct pcb *) pcb_ptr)->arrival_time;
						analysis_count = analysis_count + 1;
					}
											
					//------------GET TIMER RESULT------------
					if(((struct pcb *) pcb_ptr) != NULL) {
						//OUTMODE == 0
						if(atof(global_arguments[15]) == 0) {
							//do nothing
						}

						//OUTMODE == 1
						else if(atof(global_arguments[15]) == 1) {
							printf("%d   %d   %s\n", terminatation_time, ((struct pcb *) pcb_ptr)->pid, ((struct pcb *) pcb_ptr)->state);
						}
								
						//OUTMODE == 2
						if(atof(global_arguments[15]) == 2) {
							//-----------TODO-----------
						}
					}	
					total_process_count = total_process_count - 1;
					//printf("A THREAD IS TERMINATED");
					//printf("\ntotal_process_count is: %d\n", total_process_count);
					pthread_cond_broadcast(&scheduler_cond_var);
					//printf("AFTER CALLING scheduler_cond_var\n");

					pthread_mutex_unlock(&plock);
					pthread_exit("test");
				}
				else if( random < prob_terminate*100 + prob_io1*100 ){
					//io1
					pthread_mutex_lock(&io1lock);
											
					//------------GET TIMER RESULT------------
					if(((struct pcb *) pcb_ptr) != NULL) {
						//OUTMODE == 0
						if(atof(global_arguments[15]) == 0) {
							//do nothing
						}

						//OUTMODE == 1
						else if(atof(global_arguments[15]) == 1) {
							printf("%d   %d   %s\n", current_time.tv_usec, ((struct pcb *) pcb_ptr)->pid, "DEVICE1");
						}
								
						//OUTMODE == 2
						if(atof(global_arguments[15]) == 2) {
							//-----------TODO-----------
						}
					}
					//printf("INSIDE OF IO1 DEVICE\n");
					if(!flag_io1){
						printf("\nfffffffffff");
						pthread_cond_wait(&cond_io1, &io1lock);
						printf("\nrrrrrrrrrrrr");
					}
					else if(flag_io1){
						flag_io1 = false;
					}
					//((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[3]));
					((struct pcb *) pcb_ptr)->total_wait_time = ((struct pcb *) pcb_ptr)->total_wait_time + atoi(global_arguments[3]);
					
					
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
					if(((struct pcb *) pcb_ptr) != NULL) {
						((struct pcb *) pcb_ptr)->state = "READY";
						addatlast(*((struct pcb *) pcb_ptr));
						pthread_cond_broadcast(&scheduler_cond_var);
						flag_io1 = true;
						pthread_cond_signal(&cond_io1);
						pthread_mutex_unlock(&io1lock);
					}
					//*******************************************************************************

					pthread_cond_signal(&cond_io1);
					pthread_mutex_unlock(&io1lock);
				}
				else if( random < prob_terminate*100 + prob_io1*100 + prob_io2*100 ){
					//io2
					pthread_mutex_lock(&io2lock);
										
					//------------GET TIMER RESULT------------

					if(((struct pcb *) pcb_ptr) != NULL) {
						//OUTMODE == 0
						if(atof(global_arguments[15]) == 0) {
							//do nothing
						}

						//OUTMODE == 1
						else if(atof(global_arguments[15]) == 1) {
							printf("%d   %d   %s\n", current_time.tv_usec, ((struct pcb *) pcb_ptr)->pid, ((struct pcb *) pcb_ptr)->state);
						}
								
						//OUTMODE == 2
						if(atof(global_arguments[15]) == 2) {
							//-----------TODO-----------
						}
					}
					//printf("INSIDE OF IO2 DEVICE\n");
					if(!flag_io2){
						printf("\nfffffffffff");
						pthread_cond_wait(&cond_io2, &io2lock);
						printf("\nrrrrrrrrrrrrrrrr");
					}
					else if(flag_io2){
						flag_io2 = false;
					}
					//((struct pcb *) pcb_ptr)->state = "WAITING";
					usleep(atoi(global_arguments[4]));
					((struct pcb *) pcb_ptr)->total_wait_time = ((struct pcb *) pcb_ptr)->total_wait_time + atoi(global_arguments[4]);
					
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
					addatlast(*((struct pcb *) pcb_ptr));
					pthread_cond_broadcast(&scheduler_cond_var);
					flag_io2 = true;
					pthread_cond_signal(&cond_io2);
					pthread_mutex_unlock(&io2lock);
				}
				//flag = true;
			}
		}
	}
	// release lock
    pthread_mutex_unlock(&plock);
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
    count = 1;	/* number of threads to create initially*/
	
	//Create 10 threads immediately
	for (i = 0 ; i < count+1 ; ++i) {

		void* pcb_ptr = &(t_args[i]);

		t_args[i].pid = i+1;
		t_args[i].state = "READY";
		if(strcmp(global_arguments[5],"fixed") == 0){
			t_args[i].burst = atoi(global_arguments[6]);
			t_args[i].remaining_time = atoi(global_arguments[6]);
			t_args[i].arrival_time = 0;
		}
		else if(strcmp(global_arguments[5],"uniform") == 0){
			int max = atoi(global_arguments[8]);
			int min = atoi(global_arguments[7]);
			int random = rand() % (max-min);

			t_args[i].burst = random;
			t_args[i].remaining_time = random;
			t_args[i].arrival_time = 0;
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
			t_args[i].arrival_time = 0;
		}
		total_process_count = total_process_count + 1;
		ret = pthread_create(&(tids[counter]), NULL, process_task, (void *) &(t_args[i]));

		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		addatlast(t_args[i]);
		//insertqueue(t_args[i]);
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
				addatlast(t_args[i+11]);
				//insertqueue(t_args[i+11]);
				counter = counter +1;
				pthread_cond_broadcast(&scheduler_cond_var);
			}
		}
		total_process_count = counter;
	}
	printf("\n**********************************************************************************\n");

	
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
	while(total_process_count > 0){
		printf("Waiting on condition variable scheduler_cond_var\n");
		pthread_cond_wait(&scheduler_cond_var, &lock);
		usleep(100000);
		char *algo = global_arguments[1];
		printf("\n\nALGORITMA: %s", algo);
			
		//SCHEDULING ALGORITHMS
		printf("\n\n PASSED COND VAR \n\n");
		if( strcmp(algo,"FCFS") == 0){
			struct pcb *process  = remove_head();
			if(process != NULL) {
				//struct pcb *process = dequeue();
				process->state = "RUNNING";
				running_pid = process->pid;
				pthread_cond_broadcast(&process_cond);
			}
			else {
				pthread_cond_broadcast(&process_cond);
			}
		}
		else if( strcmp(algo,"SJF") == 0 ){
			struct pcb *process;
			int min_burst = INT_MAX;
			struct node *temp = head;
			while(temp != NULL){
		
				if(temp->my_pcb.burst < min_burst){
					min_burst = temp->my_pcb.burst;
					process = temp;
				}
				temp= temp->next;
			}
			running_pid = process->pid;
			process->state = "RUNNING";
			deleteNode(&head, running_pid);
			min_burst = INT_MAX;
			pthread_cond_broadcast(&process_cond);
		}
		
		else if( strcmp(algo,"RR") == 0){
			int quantum = atoi(global_arguments[2]);
			struct pcb *process = remove_head();
			if(process != NULL) {
				//struct pcb *process = dequeue();
				process->state = "RUNNING";
				running_pid = process->pid;
				printf("--------------------------Dequeued: %d, State of Process: %s, Remaining time:%d\n", process->pid, process->state, process->remaining_time);
				pthread_cond_broadcast(&process_cond);
				printf("*****RR İÇERİSİ*****\n");
			}
			else {
				pthread_cond_broadcast(&process_cond);
			}
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
	pthread_create(&generator_id, NULL, generate_process, NULL);
	pthread_create(&scheduler_id, NULL, schedule, NULL);	

    //---------------------------------------------------------------------------------------------------------------------------------------------------

    //---------------------------------------------------------------------------------------------------------------------------------------------------
	
    printf("\nmain: waiting all threads to terminate\n");
    
	pthread_join(generator_id, NULL);
    pthread_join(scheduler_id, NULL);

	int avg_finish_time = 0;
	int avg_waiting_time = 0;
	int avg_turnaround_time = 0;
	for(int i = 0; i < ANALYSISSIZE; i++) {
		printf("TEST ZAMANI: %d\n", all_finish_times[i]);
		avg_finish_time = all_finish_times[i] + avg_finish_time;
	}
	for(int j = 0; j < ANALYSISSIZE; j++) {
		printf("TEST ZAMANI**************: %d\n", all_waiting_times[j]);
		avg_waiting_time = all_waiting_times[j] + avg_waiting_time;
	}
	for(int k = 0; k < ANALYSISSIZE; k++) {
		avg_turnaround_time = all_turnaround_times[k] + avg_turnaround_time;
	}
	avg_finish_time = avg_finish_time / 30;
	avg_waiting_time = avg_waiting_time / 30;
	avg_turnaround_time = avg_turnaround_time / 30;
	printf("Average finish time is: %d\n", avg_finish_time);
	printf("Average waiting time is: %d\n", avg_waiting_time);
	printf("Average turnaround time is: %d\n", avg_turnaround_time);
    
    return 0;
}
