/*
 * Copyright (c) 2010, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#undef errno
extern int errno;

typedef void (*sighandler_t)(int);
static char *my_argv[100], *my_envp[100];
static char *search_path[10];

void handle_signal(int signo)
{
    printf("\n[MY_SHELL ]");
    fflush(stdout);
}

void fill_argv(char *tmp_argv)
{
    char *foo = tmp_argv;
    int index = 0;
    char ret[100];
    bzero(ret, 100);
    while(*foo != '\0') {
        if (index == 10) { break; }
        if (*foo == ' ') {
            if (my_argv[index] == NULL) {
                my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
            } else {
                bzero(my_argv[index], strlen(my_argv[index]));
            }
            strncpy(my_argv[index], ret, strlen(ret));
            strncat(my_argv[index], "\0", 1);
            bzero(ret, 100);
            index++;
        } else {
            strncat(ret, foo, 1);
        }
        foo++;
        printf("foo is %c\n", *foo);
    }
    my_argv[index] = (char *)malloc(sizeof(char) * strlen(ret) + 1);
    strncpy(my_argv[index], ret, strlen(ret));
	strncat(my_argv[index], "\0", 1);
}

void copy_envp(char **envp)
{
	int index = 0;
	for(;envp[index] != NULL; index++) {
		my_envp[index] = (char *)malloc(sizeof(char) * (strlen(envp[index]) + 1));
		memcpy(my_envp[index], envp[index], strlen(envp[index]));
	}
}

void get_path_string(char **tmp_envp, char *bin_path)
{
	int count = 0;
	char *tmp;
	while(1) {
		tmp = strstr(tmp_envp[count], "PATH");
		if(tmp == NULL) {
			count++;
		} else {
			break;
		}
	}
        strncpy(bin_path, tmp, strlen(tmp));
}

void insert_path_str_to_search(char *path_str) 
{
	int index=0;
	char *tmp = path_str;
	char ret[100];

	while(*tmp != '=')
		tmp++;
	tmp++;

	while(*tmp != '\0') {
		if(*tmp == ':') {
			strncat(ret, "/", 1);
			search_path[index] = (char *) malloc(sizeof(char) * (strlen(ret) + 1));
			strncat(search_path[index], ret, strlen(ret));
			strncat(search_path[index], "\0", 1);
			index++;
			bzero(ret, 100);
		} else {
			strncat(ret, tmp, 1);
		}
		tmp++;
	}
}

int attach_path(char *cmd)
{
	char ret[100];
	int index;
	int fd;
	bzero(ret, 100);
	for(index=0;search_path[index]!=NULL;index++) {
		strcpy(ret, search_path[index]);
		strncat(ret, cmd, strlen(cmd));
		if((fd = open(ret, O_RDONLY)) > 0) {
			strncpy(cmd, ret, strlen(ret));
			close(fd);
			return 0;
		}
	}
	return 0;
}

void call_execve(char *cmd)
{
	int i;
	printf("cmd is %s\n", cmd);
	if(fork() == 0) {
		i = execve(cmd, my_argv, my_envp);
		printf("errno is %d\n", errno);
		if(i < 0) {
			printf("%s: %s\n", cmd, "command not found");
			exit(1);		
		}
	} else {
		wait(NULL);
	}
}

void free_argv()
{
	int index;
	for(index=0;my_argv[index]!=NULL;index++) {
		bzero(my_argv[index], strlen(my_argv[index])+1);
		my_argv[index] = NULL;
		free(my_argv[index]);
	}
}

int main(int argc, char** argv)
{
    char c;
    int i, fd;
    char *tmp = (char *)malloc(sizeof(char) * 100);
	char *path_str = (char *)malloc(sizeof(char) * 256);
	char *cmd = (char *)malloc(sizeof(char) * 100);
    
    signal(SIGINT, SIG_IGN);
    signal(SIGINT, handle_signal);
    
    //copy_envp(envp);
    //get_path_string(my_envp, path_str);
    //insert_path_str_to_search(path_str);
    
    if(fork() == 0) {
		//execve("/usr/bin/clear", argv, my_envp);
		exit(1);
	} else {
		wait(NULL);
	}
}
