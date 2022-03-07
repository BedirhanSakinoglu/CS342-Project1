/* -*- linux-c -*- */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <signal.h>
#include <pthread.h>

#include "shareddefs.h"

#define MAXTHREADS 10

struct arg {
	int order;
	int intervalCount;
	int intervalWidth;
	int intervalStart;
	char * fileName;		
};

int readFile(char *fileName, int * arguments, int k){
	FILE* ptr;
    char* ch;
	size_t len = 0;
	ssize_t read;
	int counter = 0;

	ptr = fopen(fileName, "r");
		
	if (NULL == ptr) {
    	printf("file can't be opened \n");
 	}
	while ((read = getline(&ch, &len, ptr)) != -1) {
		if( arguments[2]+(k*arguments[1]) <= atoi(ch) && arguments[2]+((k+1)*arguments[1]) > atoi(ch) ){
			counter++;
			//printf("%s\n", ch);
			//printf("\nCOUNTER: %d\n", counter);
		}
    }

	fclose(ptr);
    if (ch){
        free(ch);
	}
	return counter;
}

static void *do_task(void *arg_ptr)
{	
	mqd_t mqSendToMain;
	struct childToParentItem childToParentItem;
	int histogramSpecs[3];
	int count[((struct arg *) arg_ptr)->intervalCount];
	int nx;

	histogramSpecs[0] = ((struct arg *) arg_ptr)->intervalCount;
	histogramSpecs[1] = ((struct arg *) arg_ptr)->intervalWidth;
	histogramSpecs[2] = ((struct arg *) arg_ptr)->intervalStart;
	
	for(int i=0 ; i < ((struct arg *) arg_ptr)->intervalCount ; i++){
		count[i] = readFile(((struct arg *) arg_ptr)->fileName, histogramSpecs, i);
	}

	for(int i=0 ; i < ((struct arg *) arg_ptr)->intervalCount ; i++){
		printf("Thread[%d]: %d\n", i, count[i]);
	}

	//-------------------------------------

	mqSendToMain = mq_open("/ctom", O_RDWR);
	if (mqSendToMain == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}

	for(int i=0 ; i < ((struct arg *) arg_ptr)->intervalCount ; i++){
		childToParentItem.childValue[i] = count[i];
	}

	nx = mq_send(mqSendToMain, (char *) &childToParentItem, sizeof(struct childToParentItem), 0);			
	if (nx == -1) {
		perror("mq_send failed\n");
		exit(1);
	}	
}

int main(int argc, char **argv)
{
	pthread_t tids[MAXTHREADS];
	struct arg t_args[MAXTHREADS];
	int ret;
	char *retmsg;

	mqd_t mqReceiveP, mqSendP, mqReceiveFromChild, mqSendToParent, mqReceiveFromParent, mqSendToChild;
	struct item item;
	struct responseItem responseItem;
	struct childToParentItem childToParentItem;
	struct parentToChildItem parentToChildItem;
	int nx;
	int i;
	int n;
	char *bufptr;
	int buflen;
	struct mq_attr mq_attr;
	struct item *itemptr;
	struct childToParentItem *childItemptr;
	struct parentToChildItem *parentItemptr;
	struct responseItem *responseItemptr;
	int processCount;
	int counter = 0;
	int consumerArguments[3];
	char* fileName;
	int least, most;

	//Receiver
	mqReceiveP = mq_open("/consumerToProducer", O_RDWR | O_CREAT, 0666, NULL);
	if (mqReceiveP == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqReceiveP);

	mq_getattr(mqReceiveP, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

	
	//Sender
	mqSendP = mq_open("/producerToConsumer", O_RDWR | O_CREAT, 0666, NULL);
	if (mqSendP == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqSendP);
	

    buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);
	
	i = 0; 
		
		//Get from consumer (added)
		nx = mq_receive(mqReceiveP, (char *) &item, buflen, NULL);
		if (nx == -1) {
			perror("mq_receive failed\n");
			exit(1);
		}

		itemptr = (struct item *) &item;

		for(int i=0; i < 3; i++){
			printf("item->arguments[%d]  = %d\n", i, itemptr->arguments[i]);
			consumerArguments[i] = itemptr->arguments[i];
		}
		
		int count[consumerArguments[0]];
		int result[consumerArguments[0]];
		for(int u = 0 ; u < consumerArguments[0] ; u++){
			result[u] = 0;
		}

		//Creating child processes
		processCount = atoi(argv[1]);

		for (i = 0; i < processCount; ++i) {
			t_args[i].order = i;
			t_args[i].intervalCount = consumerArguments[0];
			t_args[i].intervalWidth = consumerArguments[1];
			t_args[i].intervalStart = consumerArguments[2];
			t_args[i].fileName = argv[i+2];

			ret = pthread_create(&(tids[i]), NULL, do_task, (void *) &(t_args[i]));

			if (ret != 0) {
				printf("thread create failed \n");
				exit(1);
			}
			printf("thread %i with tid %u created\n", i, (unsigned int) tids[i]);
		}

		//-----------------------------------------------------------------
		//Parent
		mqReceiveFromChild = mq_open("/ctom", O_RDWR | O_CREAT, 0666, NULL);
		if (mqReceiveFromChild == -1) {
			perror("can not create msg queue\n");
			exit(1);
		}
		//printf("mq created, mq id = %d\n", (int) mqReceiveFromChild);
		mq_getattr(mqReceiveFromChild, &mq_attr);
		//printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);
		buflen = mq_attr.mq_msgsize;
		bufptr = (char *) malloc(buflen);

		for(int i=0; i < processCount; i++){
			nx = mq_receive(mqReceiveFromChild, (char *) &childToParentItem, buflen, NULL);
			if (nx == -1) {
				perror("mq_receive failed\n");
				exit(1);
			}

			//printf("child message received, message size = %d\n", n);
			childItemptr = (struct childToParentItem *) &childToParentItem;

			
			for(int t = 0 ; t < consumerArguments[0] ; t++){
				result[t] = result[t] + childItemptr->childValue[t];
			}
			
		}

		for(int i = 0 ; i < consumerArguments[0] ; i++){
			responseItem.value[i] = result[i];
		}

		nx = mq_send(mqSendP, (char *) &responseItem, sizeof(struct responseItem), 0);

		if (nx == -1) {
			perror("mq_send failed\n");
			exit(1);
		}
		printf("Response sent ::: ");
		for(int i = 0 ; i < consumerArguments[0] ; i++){
			printf("responseItem->value[%d]= %d\n",i ,responseItem.value[i]);
		}
		
		//----------------------------------------------------------------------------
		
	return 0;
	exit(0);
}
