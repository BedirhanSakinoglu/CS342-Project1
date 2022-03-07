
all: histserver histclient histserver_th histclient_th

histclient: histclient.c
	gcc -Wall -g -o histclient histclient.c -lrt

histserver: histserver.c
	gcc -Wall -g -o histserver histserver.c -lrt
	
histclient_th: histclient_th.c
	gcc -Wall -g -o histclient_th histclient_th.c -lrt

histserver_th: histserver_th.c
	gcc -Wall -g -o histserver_th histserver_th.c -lrt -lpthread

clean:
	rm -fr histserver histclient histserver_th histclient_th *~ *.o   core*
