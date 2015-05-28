//logger.c 		Samuel Rey, Tecnologías

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <err.h>
#include <string.h>
#include <signal.h>


//CONSTANTES
char *path = "/tmp/logger";
enum {maxlenbuf=4*1024};


int static
printtime (void){
	time_t t;
	char buftime[1024];
	char* paux;

	if (time(&t)<0){
		warn("time");
		return -1;
	}
	if (ctime_r(&t, buftime)==NULL){
		warn("ctime_r");
		return -1;
	}
	paux = strrchr(buftime, '\n');
	if (paux != NULL){
		*paux = ':';
	}
	printf("%s", buftime);
	return 0;
}

int static
printinfo (char *inf){

	if (printtime() < 0){
		return -1;
	}
	printf (" %s", inf);
	return 0;
}

int static
processevent (FILE *file, char buf[]){
	char *event;

	for (;;){
		event=fgets(buf, maxlenbuf, file);
		if (event==NULL){
			break;
		}
		printinfo(event);
		alarm (0);
		alarm(5);
	 }		
	return 0;
}

int static
makefifo(void){

	if (access(path, F_OK)==0){
		if (unlink(path)<0){
			warn("unlink %s", path);
			return -1;
		}
	}
	if (mkfifo (path, 0664)<0){
		warn("mkfifo %s", path);
		return -1;
	}
	return 0;
}

void static
timeout (int no){
	printinfo ("time out, no events\n");
	alarm (5);
}

int
main (int argc, char* argv[]){
	char buf[maxlenbuf];
	FILE *file;

	if (argc >1){
		fprintf (stderr, "wrong arguments\n");
		exit (1);
	}
	if (makefifo () < 0){
		exit(1);
	}
	signal(SIGALRM, timeout);
	siginterrupt(SIGALRM, 0);
	for (;;){
		if(printinfo("waiting for clients\n") < 0){
			exit(1);
		}
		file = fopen (path, "r");
		if (file == NULL){
			err (1, "open %s", path);
		}
		if(printinfo ("ready to read events\n") < 0){
			exit(1);
		}
		alarm (5);
		processevent (file, buf);
		if (fclose (file)<0){
			err (1, "fclose");
		}
		alarm(0);
	}
	exit (0);
}