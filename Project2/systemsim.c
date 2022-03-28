
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define MAXTHREADS  20	    /* max number of threads */
#define MAXFILENAME 50		/* max length of a filename */

char[] global_arguments;
bool flag = false;
bool flag_io1 = true;
bool flag_io2 = true;

// Declaration of thread condition variable
pthread_cond_t scheduler_cond_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_io1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_io2 = PTHREAD_COND_INITIALIZER;
 
// declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t io1lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t io2lock = PTHREAD_MUTEX_INITIALIZER;

struct cpu {

}

struct ioDevice {
	service_time;
}

struct pcb {
    int pid;
	int burst;
	int remaining_time;
    char state[20];
}

//Ready Queue ---------------------------------------------
int ready_queue[MAXTHREADS];
int head_index = 0;
int tail_index = 0;

void insertqueue(struct pcb value){
    ready_queue[tail_index] = value;
    tail_index = tail_index + 1;
    printf("Inserted to ready queue: %d\n", value.pid);
}

struct pcb dequeue(){
    struct pcb value = ready_queue[head_index];
    head_index = head_index + 1;
    printf("Dequeued: %d\n", value.pid);
    return value;
}

void printqueue(char* attr){
    for(int i = 0 ; i < tail_index - head_index ; i++){
        if(attr == "pid"){
			printf("%d, ", ready_queue[i].pid);
		}
		else if(attr == "burst"){
			printf("%d, ", ready_queue[i].burst);
		}
		else if(attr = "state"){
			printf("%d, ", ready_queue[i].state);
		}
		
    }
}
//---------------------------------------------------------

static void *process_task(void *pcb_ptr)
{
	char algo[] = global_arguments[1];
	
	int pid = ((struct pcb *) pcb_ptr)->pid;
	int burst = ((struct pcb *) pcb_ptr)->burst;
	int remaining_time = ((struct pcb *) pcb_ptr)->remaining_time;
	char state[] = ((struct pcb *) pcb_ptr)->state;

	double prob_terminate = atof(global_arguments[9]);
	double prob_io1 = atof(global_arguments[10]);
	double prob_io2 = atof(global_arguments[11]);

	int random;
	bool check = false;
	
	while(state != "TERMINATED"){
		random = rand() % 100;

		if( state == "RUNNING" ){
			if(algo == "RR"){
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
					ready_queue.insertqueue(*((struct pcb *) pcb_ptr));
				}
			}
			else if(algo == "FCFS" | algo == "SJF"){
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
					
					//Degisecek burst eklenecek

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
					

					pthread_cond_signal(&cond_io2);
					pthread_mutex_unlock(&io2lock);
				}
			}
		}
	}
	
	retreason = malloc (200); 
	strcpy (retreason, "normal termination of thread"); 
	pthread_exit(retreason);  // just tell a reason to the thread that is waiting in join
}

static void *generate_process(){
	pthread_t tids[MAXTHREADS];	/*thread ids*/
	int count;
	int random;		        /*number of threads*/
	struct pcb t_args[MAXTHREADS];	/*thread function arguments*/
	
	int pg, maxp, allp;
	int i;
	int ret;
	char *retmsg; 
    
	//flag = false;
    count = 10;	/* number of threads to create initially*/
	
	//Create 10 threads immediately
	for (i = 0 ; i < count+1 ; ++i) {
		t_args[i].pid = i+1;
		t_args[i].state = "READY";

		ret = pthread_create(&(tids[i]), NULL, process_task, (void *) &(t_args[i]));

		if (ret != 0) {
			printf("thread create failed \n");
			exit(1);
		}
		ready_queue.insertqueue(t_args[i]);
		printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
	}

	//Parameters 12, 13, 14 -> pg, MAXP, ALLP
	allp = atoi(global_arguments[14]);
	maxp = atoi(global_arguments[13]);
	pg = atof(global_arguments[12]);

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
			if(random < pg*coeff){
				//Create threads
				t_args[i].pid = i;
				t_args[i].state = "READY";

				ret = pthread_create(&(tids[i]), NULL, process_task, (void *) &(t_args[i]));

				if (ret != 0) {
					printf("thread create failed \n");
					exit(1);
				}
				printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
			}
		}
	}

	for (i = 0; i < allp; ++i) {
	    ret = pthread_join(tids[i], (void **)&retmsg);
		if (ret != 0) {
			printf("thread join failed \n");
			exit(1);
		}
		printf ("thread terminated, msg = %s\n", retmsg);
		// we got the reason as the string pointed by retmsg
		// space for that was allocated in thread function; now freeing. 
		free (retmsg); 
	}

	printf("main: all process threads terminated\n");

    retreason = malloc (200); 
	strcpy (retreason, "normal termination of thread"); 
	pthread_exit(retreason); 
}

static void *schedule(){
	// acquire a lock
    pthread_mutex_lock(&lock);
	while(!flag){
		printf("Waiting on condition variable scheduler_cond_var\n");
        pthread_cond_wait(&scheduler_cond_var, &lock);
		char algo[] = global_arguments[1];
		
		//SCHEDULING ALGORITHMS
		if( algo == "FCFS" ){
			struct pcb process = ready_queue.dequeue();
			process.state = "RUNNING";
		}
		else if( algo == "SJF" ){
			struct pcb process;
			int min_burst = INT_MAX;
			for(int i = 0 ; i < tail_index - head_index ; i++){
				if(ready_queue[i].printqueue(burst) < min_burst){
					min_burst = ready_queue[i].printqueue(burst);
					process = ready_queue[i];
				}
			}
			process.state = "RUNNING";
		}
		else if( algo == "RR" ){
			int quantum = atoi(global_arguments[2]);
			struct pcb process = ready_queue.dequeue();
			process.state = "RUNNING";
		}
		else{
			priintf("\nERROR: Undefined Scheduling Algorithm");
		}
	}

	if(flag){
		printf("Signaling condition variable scheduler_cond_var\n");
		pthread_cond_broadcast(&scheduler_cond_var);
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
    








    //-----------------------------------------------------------------------------------------------------------------------------------------------------
	
    printf("main: waiting all threads to terminate\n");
    pthread_join(generator_id, NULL);
    pthread_join(scheduler_id, NULL);

	printf("generator and scheduler threads terminated\n");
    
    return 0;
}
