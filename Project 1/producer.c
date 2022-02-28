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
        
#include "commondefs.h"
#include "shareddefs.h"

int main(int argc, char **argv)
{

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
	int result;
	int counter = 0; //deneme
	int consumerArguments[3];

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

		//Creating child processes
		processCount = consumerArguments[0];

		//mq_close(mqReceiveP);
		//printf("PROCCESS COUNT: %d",  processCount);

		//************************************************************************************************************Send to child
		mqSendToChild = mq_open("/ptoc", O_RDWR | O_CREAT, 0666, NULL);
		if (mqSendToChild == -1) {
			perror("can not create msg queue\n");
			exit(1);
		}
		//printf("mq created, mq id = %d\n", (int) mqSendToChild );

		mq_getattr(mqReceiveFromChild , &mq_attr);
		//printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

		buflen = mq_attr.mq_msgsize;
		bufptr = (char *) malloc(buflen);
		
		//**************************************************************************************************************************
		printf("Process Count: %d\n", processCount);
		n = fork();
		if(n != 0){
			
			parentToChildItem.order = 0;
			nx = mq_send(mqSendToChild, (char *) &parentToChildItem, sizeof(struct parentToChildItem), 0);
			if (nx == -1) {
				perror("mq_send failed\n");
				exit(1);
			}

			sleep(1);

			for(int i=1; i<processCount; i++){
				if(n != 0) {
					//printf("FORK: %d, n: %d\n", i, n);
					n = fork();
					if(n==0){
						break;
					}
				}
				if(n!=0){
					parentToChildItem.order = i;
					printf("\n I:::::: %d\n", i);
					nx = mq_send(mqSendToChild, (char *) &parentToChildItem, sizeof(struct parentToChildItem), 0);
					printf(" SENT NOW::::::: %d\n", parentToChildItem.order);
					if (nx == -1) {
						perror("mq_send failed\n");
						exit(1);
					}
					sleep(1);
					//printf("\nmq_send success, item size = %d\n",(int) sizeof(struct item));
				}
				
			}
		}
		

		if(n!=0){
			//Parent
			mqReceiveFromChild = mq_open("/ctop", O_RDWR | O_CREAT, 0666, NULL);
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
				
				if(childItemptr->isSent == 1){
					printf("RECEIVED :: childItemptr->childValue  = %d\n", childItemptr->childValue);
					result = result + childItemptr->childValue;
				}

				else{
					printf("\nError\n");
				}
			}
			//Send to consumer
				
			responseItem.value = result;

			nx = mq_send(mqSendP, (char *) &responseItem, sizeof(struct responseItem), 0);

			if (nx == -1) {
				perror("mq_send failed\n");
				exit(1);
			}
			printf("Response sent ::: ");
			printf("responseItem->value= %d\n", responseItem.value);
			
			
		}
		else if(n == 0){
			//Child
	
			//*******************************************************************************************************Receive from parent
			mqReceiveFromParent = mq_open("/ptoc", O_RDWR | O_CREAT, 0666, NULL);
			if (mqReceiveFromParent == -1) {
				perror("can not create msg queue\n");
				exit(1);
			}
			//printf("mq created, mq id = %d\n", (int) mqReceiveFromChild);

			mq_getattr(mqReceiveFromParent , &mq_attr);
			//printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);
			buflen = mq_attr.mq_msgsize;
			bufptr = (char *) malloc(buflen);
			//--
			nx = mq_receive(mqReceiveFromParent, (char *) &parentToChildItem, buflen, NULL);
			if (nx == -1) {
				perror("mq_receive failed\n");
				exit(1);
			}
			//printf("child message received, message size = %d\n", n);
			parentItemptr = (struct parentToChildItem *) &parentToChildItem;
			
			//int order = parentItemptr->order;
			printf("!!!!! ORDER = %d\n", parentItemptr->order);

			//**************************************************************************************************************************

			//printf("COUNTER = %d", counter);
			mqSendToParent = mq_open("/ctop", O_RDWR);
			if (mqSendToParent == -1) {
				perror("can not create msg queue\n");
				exit(1);
			}
			//printf("child mq created, mq id = %d\n", (int) mqSendToParent);

			mq_getattr(mqReceiveFromParent, &mq_attr);

			buflen = mq_attr.mq_msgsize;
			bufptr = (char *) malloc(buflen);

			//---------------

			childToParentItem.childValue = consumerArguments[2]+(parentItemptr->order*consumerArguments[1]);
			//childToParentItem.childValue = 99;
			childToParentItem.isSent = 1;

			//childItemptr = (struct childToParentItem *) bufptr;
			nx = mq_send(mqSendToParent, (char *) &childToParentItem, sizeof(struct childToParentItem), 0);
			
			if (nx == -1) {
				perror("mq_send failed\n");
				exit(1);
			}
			printf("child message sent :: ");
			printf("childToParentItem->childValue= %d\n", childToParentItem.childValue);
			printf("childToParentItem->isSent= %d\n", childToParentItem.isSent);

			sleep(10);
	
		}

	//free(bufptr);
	//mq_close(mqReceiveP);
	//mq_close(mqSendP);
	//mq_close(mqReceiveFromParent);
	return 0;
	exit(0);
}
