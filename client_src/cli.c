#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <pwd.h>

#include "cli.h"
#include "parse.h"
#include "client.h"

extern char* readline(const char*);

static struct passwd *pw;
static char prompt[SIZE];
static char curr_dir[SIZE];
static char *homedir;

static void set_prompt(void){
  sprintf(prompt, "\n%s $> ", curr_dir);
}

void init_cli(void){
  printf("Welcome to simple FTP Utility\n");
  pw = getpwuid(getuid());
  homedir = pw->pw_dir;

  chdir(homedir);
  getcwd(curr_dir, SIZE);
  set_prompt();
}

static void execute(char *cmd_line){
  pid_t child_pid;
  char *argv[SIZE];
  int argc, status;

  char *cmd = parse_command(cmd_line, argv, &argc);
  //print_cmd(argc, argv);

  if (strcmp(cmd, "ls") == 0){
    child_pid = fork();
    
    if(child_pid == 0)
      execvp("/bin/ls", argv);
    else
      waitpid(child_pid, &status, 0);
  }
  else if(strcmp(cmd, "cd") == 0){
    chdir(argv[1]);
    getcwd(curr_dir, SIZE);
    set_prompt();
  }
  else if(strcmp(cmd, "exit") == 0){
    exit(0);
  }
  else if(strcmp(cmd, "get") == 0){
    get(argc, argv);
    set_prompt();
  }
  else if(strcmp(cmd, "put") == 0){
    put(argc, argv);
    set_prompt();
  }
  else if(strcmp(cmd, "mget") == 0){
    mget(argc, argv);
    set_prompt();
  }
  else if(strcmp(cmd, "mput") == 0){
    mput(argc, argv);
    set_prompt();
  }
  else{
    printf("\nError: Invalid Command.\n");
    set_prompt();
  }
  free(cmd_line);
}

void cli(void){
  /* Takes input from console, parses it, handles it and repeat. */
  while(1){
    char *cmd_line;

    cmd_line = readline(prompt);
    if (strcmp(cmd_line,"") != 0)
      execute(cmd_line);
  }
}
