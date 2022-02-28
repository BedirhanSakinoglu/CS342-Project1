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

	mqd_t mqReceiveC, mqSendC;
	struct mq_attr mq_attr;
	struct item *itemptr;
	struct item item;
	struct responseItem responseItem;
	struct responseItem *responseItemptr;
	int n;
	char *bufptr;
	int buflen;

	//Receiver
	mqReceiveC = mq_open("/producerToConsumer", O_RDWR | O_CREAT, 0666, NULL);
	if (mqReceiveC == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqReceiveC);

	mq_getattr(mqReceiveC, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);

	
	//Sender
	mqSendC = mq_open("/consumerToProducer", O_RDWR | O_CREAT, 0666, NULL);
	if (mqSendC == -1) {
		perror("can not create msg queue\n");
		exit(1);
	}
	printf("mq created, mq id = %d\n", (int) mqSendC);

	mq_getattr(mqReceiveC, &mq_attr);
	printf("mq maximum msgsize = %d\n", (int) mq_attr.mq_msgsize);
	
	/* allocate large enough space for the buffer to store 
        an incoming message */
    buflen = mq_attr.mq_msgsize;
	bufptr = (char *) malloc(buflen);

	//Send to producer(added)
	for(int i=1 ; i < argc ; i++){
		item.arguments[i-1] = atoi(argv[i]);
	}

	n = mq_send(mqSendC, (char *) &item, sizeof(struct item), 0);

	if (n == -1) {
		perror("mq_send failed\n");
		exit(1);
	}

	printf("\nmq_send success, item size = %d\n",
	       (int) sizeof(struct item));
	for(int i=0; i < 3; i++){
		printf("item->arguments[%d]  = %d\n", i, item.arguments[i]);
	}
	
	//while (1) {
		//Get from producer
		n = mq_receive(mqReceiveC, (char *) &responseItem, buflen, NULL);
		if (n == -1) {
			perror("mq_receive failed\n");
			exit(1);
		}

		responseItemptr = (struct responseItem*) &responseItem;

		printf("mq_receive success, message size = %d\n", n);
		//responseItemptr = (struct responseItem *) bufptr;

		printf("Received responseItem->value = %d\n", responseItemptr->value);
	//}

	//free(bufptr);
	//mq_close(mqReceiveC);
	//mq_close(mqSendC);
	return(0);
	exit(0);
}
