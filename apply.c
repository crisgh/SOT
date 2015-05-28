//apply.c		Samuel Rey

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>


//CONSTS
char* Npaths[] = {"/bin", "/usr/bin", "/usr/local/bin", NULL};


int static
istxt(char* name){
	char* lasttxt;

	lasttxt = strrchr(name, '.');
	if ( (lasttxt != NULL) && (strcmp(lasttxt, ".txt") == 0)){
		return 0;
	}else{
		return 1;
	}
}


int static
lookstat (char* dname){
	struct stat st;

	if (stat(dname, &st) < 0){
		warn ("stat: ");
		return -1;
	}
	if ((st.st_mode & S_IFMT) == S_IFREG){
		return 0;
	}else{
		return 1;
	}
}


int static
searchpath (char* cmd, char* path){
	int i;

	for (i=0; Npaths[i] != NULL; i++){
		if (sprintf(path, "%s/%s", Npaths[i], cmd) < 0){
			warn("sprintf: ");
			return -1;
		}
		if (access (path, X_OK) == 0){
			return 0;
		}
	}
	return -1;
}


int static
execcomand (int outfd, char* fname, char* argv[], char path[]){
	int pid, sts, infd;

	infd = open (fname, O_RDONLY);
	if(infd < 0){
		warn("open: ");
		return -1;
	}
	pid = fork();
	switch (pid){
	case -1:
		warn("fork: ");
		return -1;
	case 0:
		if(dup2 (infd, 0) < 0){
			err(1, "dup2: ");
		}
		if(dup2 (outfd, 1) < 0){
			err(1, "dup2: ");
		}
		if(execv (path, argv)<0){
			err(1, "execv: ");
		}
	default:
		while(wait(&sts) != pid){
			;
		}		
		if (close (infd) < 0){
			warn("close: ");
			return -1;
		}
	}
	return 0;
}


int static
searchindir (int fd, char* argv[], char path []){
	DIR* d;
	struct dirent* de;
	char* dname;

	d = opendir(".");
	if (d == NULL){
		warn("open dir:");
		return -1;
	}
	while((de = readdir(d)) != NULL){
		dname = (*de).d_name;
		if ((lookstat(dname) == 0) && (istxt (dname)== 0)){
			if(execcomand(fd, dname, argv, path) == -1){
				return -1;
			}			
		}	
	}
	return 0;
} 


int
main (int argc, char* argv[]){
	int fd;
	char path[1024];

	if (argc < 2){
		fprintf(stderr, "wrong arguments: apply comand [args]\n");
		exit(1);
	}
	argc--;
	argv++;
	if(searchpath (argv[0], path) != 0){
		fprintf (stderr, "comand not found\n");
		exit(1);
	}
	fd = creat("apply.output", 0660);
	if (fd < 0){
		err(1, "creat: ");
	}
	if(searchindir (fd, argv, path) < 0){
		exit(1);
	}
	if (close (fd) < 0){
		err(1, "close: ");
	}
	exit (0);
}