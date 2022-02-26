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
        
#include "commondefs.h"
#include "shareddefs.h"

int main(int argc, char **argv)
{

	mqd_t mqReceiveP, mqSendP;
	struct item item;
	struct responseItem responseItem;
	int n;
	char *bufptr;
	int buflen;
	struct mq_attr mq_attr;
	struct item *itemptr;
	struct responseItem *responseItemptr;

	//Receiver
	mqReceiveP = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
	if (mqReceiveP == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqReceiveP);

	mq_getattr(mqReceiveP, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

	//Sender
	mqSendP = mq_open(MQNAME, O_RDWR | O_CREAT, 0666, NULL);
	if (mqSendP == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqSendP);

	mq_getattr(mqSendP, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

	/* allocate large enough space for the buffer to store 
        an incoming message */
    buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);
	
	i = 0; 

	while (1) {
		//---------------------------------------------------
		//Get from consumer (added)
		n = mq_receive(mqReceiveP, (char *) bufptr, buflen, NULL);
		if (n == -1) {
			perror("mq_receive failed\n");
			exit(1);
		}

		printf("mq_receive success, message size = %d\n", n);

		itemptr = (struct item *) bufptr;

		for(int i=0; i < 3; i++){
			printf("item->arguments[%d]  = %d\n", i, itemptr->arguments[i]);
		}
		
		//----------------------------------------------------
		//Send to consumer
		responseItem.value = i;

		n = mq_send(mqSendP, (char *) &responseItem, sizeof(struct responseItem), 0);

		if (n == -1) {
			perror("mq_send failed\n");
			exit(1);
		}

		printf("responseItem->value= %s\n", responseItem.value);

	}
	
	free(bufptr);
	mq_close(mqReceiveP);
	mq_close(mqSendP);
	return 0;
	exit(0);
}
