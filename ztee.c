//ztee.c 	Samuel Rey

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


int static
decompress (int fds[]){
	int pid;

	pid = fork();
	switch (pid){
	case -1:
		warn("fork");
		return -1;
	case 0:
		if (close(fds[0])<0){
			err(1,"close fd: %d\n", fds[0]);
		}
		if (dup2(fds[1],1) < 0){
			err(1, "dup2");
		}
		if(close (fds[1]) < 0){
			err(1,"close fd: %d\n", fds[1]);
		}
		if (execl("/bin/gunzip", "gunzip", NULL)<0){
			err (1, "exec gunzip:");
		}
	default:
		if(close(fds[1]) < 0){
			warn("close fd: %d\n", fds[1]);
			return -1;
		}
		return pid;
	}
}

int static
printresult (int nr, char* file, char buf[], int fd){

	if (write (1, buf, nr) != nr){
		warn("write:");
		return -1;
	}
	if (write (fd, buf, nr) != nr){
		warn("write:");
		return -1;
	}
	return 0;
}

int static
readfrompipe (int fds[], char buf[], char* file){
	int nr, fd;

	fd = creat(file, 0660);
	if(fd < 0){
		warn("create file:");
		return -1;
	}
	for (;;){
		nr = read (fds[0], buf, sizeof(buf));
		if (nr < 0){
			warn("read: ");
			return -1;
		}
		if (nr == 0){
			return 0;
		}
		if(printresult (nr, file, buf, fd) < 0){
			return -1;
		}
	}
	if (close (fd) < 0){
		warn("close fd: %d\n", fd);
		return -1;
	}
	if (close (fds[0]) < 0){
		warn("close fd: %d\n", fds[0]);
		return -1;
	}
	return 0;
}

int
main (int argc, char* argv[]){
	int fds[2], pid, sts;
	char buf[4*1024];

	if(argc != 2){
		fprintf (stderr, "wrong arguments: ztee fichsalida\n");
		exit (1);
	}
	if(pipe(fds) < 0){
		err(1, "pipe:");
	}
	pid = decompress(fds);
	if(pid < 0){
		exit(1);
	}		
	if(readfrompipe (fds, buf, argv[1]) < 0){
		exit(1);
	}
	while(pid != wait(&sts)){
		;
	}
	exit (0);
}