//pipeline.c

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

//CONSTANTS
enum {maxcmds = 20, minarg = 5};

struct cmdtype{
	char* cmd;
	int fdin ;
	int fdout;
	int nextfdin;
};

typedef struct cmdtype cmdt;

struct linetype{
	cmdt cmds[maxcmds];
	int cmdc;
	char* stdin;
	char* stdout;
};

typedef struct linetype linet;

int static
initline (linet *l, int argc, char* argv[]){

	(*l).stdin = argv[1];
	(*l).stdout = argv[3];
	argc = argc - 4;
	argv = argv + 4;
	for ((*l).cmdc=0; (*l).cmdc<(argc); (*l).cmdc++){		
		if ((*l).cmdc >= maxcmds){
			fprintf (stderr, "To many commands. Max cmds = %d.\n", maxcmds);
			return -1;
		}
		(*l).cmds[(*l).cmdc].cmd = argv[(*l).cmdc];
		(*l).cmds[(*l).cmdc].fdin = 0;
		(*l).cmds[(*l).cmdc].fdout = 0;
		(*l).cmds[(*l).cmdc].nextfdin = 0;
	}
	return 0;
}


int static
inoutredirect (cmdt cmd){

	if (cmd.fdin != 0){
		if(dup2 (cmd.fdin, 0)<0){
			err(1, "dup2 fdin: %d", cmd.fdin);
		}
		if (close (cmd.fdin)<0){
			err(1, "close fdin: %d", cmd.fdin);
		}
	}
	if (cmd.fdout != 0){
		if (dup2(cmd.fdout, 1)<0){
			err(1, "dup2 fdout: %d", cmd.fdout);
		}
		if (close(cmd.fdout)<0){
			err(1, "close fdout: %d", cmd.fdout);
		}
	}
	if (cmd.nextfdin != 0){
		if (close(cmd.nextfdin)<0){
			err(1, "close nextfdin: %d", cmd.nextfdin);
		}
	}
	return 0;
}


int static
closefds (cmdt cmds[], int i){

	if (cmds[i].fdin != 0){
		if (close(cmds[i].fdin)<0){
			warn("Close fdin: %d", cmds[i].fdin);
			return -1;
		}
	}
	if (cmds[i].fdout != 0){
		if (close(cmds[i].fdout)<0){
			warn("Close fdout: %d", cmds[i].fdout);
			return -1;
		}
	}
	return 0;
}


int static
execcmd (cmdt cmds[], char* path, int i, int *pid){

	*pid =fork ();
	switch (*pid){
	case -1:
		warn("fork");
		return -1;
	case 0:
		inoutredirect (cmds[i]);
		execl (path, cmds[i].cmd, NULL);
		warn("exec %s", cmds[i].cmd);
		return -1;
	default:
		if (closefds (cmds, i) < 0){
			return -1;
		}
		return 0;
	}
}


int static
getpath (char* cmd, char** path){
	char* pathaux, *paux;
	char buf[1024], newpath[1024];

	pathaux = strncpy(buf, getenv("PATH"), sizeof(buf));
	for (;;pathaux=NULL){
		*path=strtok_r(pathaux, ":", &paux);
		if (*path==NULL){
			break;
		}
		snprintf(newpath, sizeof(newpath), "%s/%s", *path, cmd);
		if (access (newpath, X_OK)==0){
			*path = newpath;
			return 0;
		}
	}
	return -1;
}


int static
runcmd (cmdt cmds[], int i, int *pid){
	char* path;

	path = NULL;
	if (getpath(cmds[i].cmd, &path)<0){
		fprintf (stderr, "command %s not found\n", cmds[i].cmd);
		return -1;
	}
	if (execcmd (cmds, path, i, pid)!= 0){
			return -1; 	
	}
	return 0;
}


int static
pipeto (linet *l, int i){
	int fds[2];

	if (i < (*l).cmdc-1){
		if (pipe (fds)<0){
			warn("pipe");
			return -1;
		}
		(*l).cmds[i].fdout = fds[1];
		(*l).cmds[i].nextfdin = fds[0];
	}
	if (i!=0){
		(*l).cmds[i].fdin = (*l).cmds[i-1].nextfdin;
	}
	return 0;
}


int static
runcmdline (linet l){
	int i, sts, pid;

	for (i=0; i<l.cmdc; i++){
		if (i==0){	
			l.cmds[i].fdin = open(l.stdin, O_RDONLY);
			if (l.cmds[i].fdin < 0){
				warn("open %s", l.stdin);
				return -1;
			}
		}
		if (i == l.cmdc-1){
			l.cmds[i].fdout = creat(l.stdout, 0664);
			if (l.cmds[i].fdout<0){
				warn("open %s", l.stdout);
				return -1;
			}
		}
		if (l.cmdc > 1){
			if (pipeto (&l, i) < 0){
				return -1;
			}
		}
		if(runcmd (l.cmds, i, &pid) < 0){
			return -1;
		}
	}
	while(wait(&sts) != pid){
		;
	}
	return 0;
}


int
main (int argc, char* argv[]){
	linet line;

	argc--;
	argv++;
	if (argc < minarg || (strcmp (argv[0],"-i") != 0) || (strcmp (argv[2], "-o")) != 0){
		fprintf (stderr, "wrong arguments: pipeline -i fichin -o fichout cmd1 cmd2...\n");
		exit (1);
	}
	if(initline (&line, argc, argv) < 0){
		exit(1);
	}
	if (runcmdline (line) < 0){
		exit(1);
	}
	exit (0);
}