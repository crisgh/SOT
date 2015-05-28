//rmrubbish.c Samuel Rey

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


int static searchindir(char* path);


int static
isrubbish(char* name){
	char* lastrub;

	lastrub = strrchr(name, '.');
	if ((lastrub != NULL) && (strcmp (lastrub,".rubbish")==0)){
		return 1;
	}else{
		return 0;
	}
}


int static
deleterubbish(char* dname, char* path){
	if (isrubbish (dname)){
		if (unlink (path)<0){
			fprintf (stderr, "%d: error removing %s\n", getpid(), path);
			exit(1);
		}else{
			return 1;
		}
	}
	return 0;		
}


int static
lookstat(char* path){
	struct stat st;

	if (stat(path, &st) < 0){
			fprintf (stderr, "%d: can't look stat %s\n", getpid(), path);
			return -1;
	}
	if ((st.st_mode & S_IFMT)==S_IFDIR){
		return 1;
	}else{
		return 0;
	}
}


int static
processelement(char* path, char* dname){
	int removed = 0;

	if(lookstat(path) == 1){
		removed = searchindir(path);
		//An empty folder is considered an error
		if(removed <= 0){
			fprintf(stderr, "%d: no files to remove in %s/%s\n", getpid(), path, dname);
			exit(1);
		}
	}else{
		removed = deleterubbish(dname, path);
	}
	return removed;
}


int static
searchindir(char* path){
	DIR* dir;
	struct dirent* de;
	char* dname;
	char fpath[4*1024];
	int removed;

	removed = 0;
	dir = opendir(path);
	if(dir == NULL){
		fprintf(stderr, "%d: wrong path %s\n", getpid(), path);
		exit(1);
	}
	while((de = readdir(dir)) != NULL){
		dname = (*de).d_name;
		if ((strcmp (dname, ".") != 0) && (strcmp (dname, "..") !=  0)){
			if (snprintf(fpath, (strlen(path)+strlen(dname)+2), "%s/%s", path, dname) < 0){
				fprintf(stderr, "%d: snprintf error:", getpid());
				exit(1);
			}
			removed = removed + processelement(fpath, dname);
			if(removed < 0){
				return 0;
			}
		}
	}
	if (closedir (dir) < 0){
		fprintf(stderr, "%d closedir:", getpid());
		exit(1);
	}
	return removed;
}


int static
printresult(int result, char* path){
	int pid;

	pid = getpid();
	if (result > 0){
		fprintf (stderr, "%d: %s ok\n", pid, path);
		exit (0);
	}else{
		fprintf (stderr, "%d: no files to remove in %s\n", pid, path);
		exit (1);
	}
}


int static
waitandsummarize(int pid, int argc){
	int sts, failed;

	failed = 0;
	while(wait(&sts) != pid){
		if (sts != 0){
			failed++;
		}
	}
	if (sts != 0){
			failed++;
	}
	if (failed == 0){
		printf ("all processes were successful\n");
	}else{
		printf ("%d processes failed\n", failed);
	}
	return 0;
}


int static
parallelizesearch(int argc, char* argv[]){
	int i, pid, result;
	char* path;

	for (i=0; i<argc; i++){
		path = argv[i];
		pid = fork();
		switch (pid){
		case 0:
			result = searchindir(path);
			printresult(result, path);
		case -1:
			warn("fork:");
			return -1;
		}
	}
	return pid;
}


int
main(int argc, char* argv[]){
	int pid;

	if (argc < 2){
		fprintf (stderr, "usage: rmrubbish dir1 [dir2  dir3 ...]\n");
		exit (1);
	}
	argc--;
	argv++;
	pid = parallelizesearch(argc, argv);
	if(pid < 0){
		exit(1);
	}
	waitandsummarize(pid, argc);
	exit (0);
}