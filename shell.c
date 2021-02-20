#include <unistd.h> // getcwd
#include <string.h> //strcopy
#include <stdio.h> //getline, strdup
#include <stdlib.h>
#include <errno.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <ctype.h>

#define HISTORY_LEN 50
#define ENV_VAR 25

struct utsname uinfo;
char present_dir[3][500];
char* env_name[ENV_VAR];
char* env_value[ENV_VAR];
int curr;

struct command
{
  char **argv;
};

void
spawn_proc (int in, int out, struct command *cmd)
{
  pid_t pid;
   if ((pid = fork ()) == 0)
    {
      if (in != 0)
        {
          dup2 (in, 0);
          close (in);
        }

      if (out != 1)
        {
          dup2 (out, 1);
          close (out);
        }

      execvp (cmd->argv[0], (char**) cmd->argv);
    }
	else{
		wait(NULL);
	}
}

void
fork_pipes (int n, struct command *cmd)
{
  int i;
  pid_t pid;
  int in, pipefd [2];

  in = 0;
  for (i = 0; i < n; ++i)
    {
      pipe (pipefd);

      spawn_proc (in, pipefd [1], cmd + i);

      close (pipefd [1]);

      in = pipefd [0];
    }
  if(pid=fork()==0){
	if (in != 0)
    		dup2 (in, 0);
	execvp (cmd[i].argv[0], (char**) cmd[i].argv);
  }
  else{
	wait(NULL);
	}

}

void searchVarEnv(char* token)
{
    char aux[301], i=0;
    for(int i=0;i<ENV_VAR;i++){
       if(env_name[i]){
        if(strcmp(env_name[i], token)==0){
            printf("%s ", env_value[i]);
            return;
        }
       }
    }
    printf("Nu exista variabila de mediu %s\n", token);
    return;
}

void createVarEnv(char* name, char* value)
{
    free(env_value[curr]);
    free(env_name[curr]);
    env_name[curr]=strdup(name);
    env_value[curr]=strdup(value);
    printf("Variabila %s cu val %s\n",env_name[curr],env_value[curr]);
    curr = (curr+1) % ENV_VAR;
    printf("Variabila de mediu creata cu succes!\n");
}
void cd(char *token, char *present_dir)
{
    char s[500];
    printf("Ne aflam pe calea \n%s\n", getcwd(s, 500));
    token = strtok(NULL, " \n\t\r");
    if (token == NULL || strcmp(token, "~") == 0)  //daca aveam doar comanda cd sau cd ~, ramanem in folderul curent
    {
        chdir(present_dir);
        printf("%s\n", getcwd(s, 500));
    }
    else if (strcmp(token, "..") == 0){
        chdir("..");
        printf("%s\n", getcwd(s, 500));
    }
    else if (chdir(token) != 0)
        printf("Nu exista calea respectiva!\n");
    else
    {
        chdir(token);
        printf("%s\n", getcwd(s, 500));
    }
    return;
}

void history(char *hist[], int current)
{
    int i = current, index = 1;
    do {
        if(hist[i]) // daca avem o comanda la acel indice
        {
            printf("%d %s\n", index, hist[i]);
            index += 1;
        }
        i = (i + 1) % HISTORY_LEN;
    } while (i != current);
}

void clear_history(char *hist[])
{
    for (int i = 0; i < HISTORY_LEN; i++)
    {
        free(hist[i]);
        hist[i] = NULL;
    }
}
char *replace_str(char *str, char *orig, char *rep)
{
    static char buffer[4096];
    char *p;

    if (!(p = strstr(str, orig)))
        return str;

    strncpy(buffer, str, p - str);
    buffer[p - str] = '\0';

    sprintf(buffer + (p - str), "%s%s", rep, p + strlen(orig));

    return buffer;
}

void print()
{
    char *cwd = (char *)malloc(sizeof(char) * 1024);
    cwd = getcwd(cwd, 1024);
    char *s = replace_str(cwd, present_dir[2], "~");
    printf("<%s%s@%s%s:%s%s%s>", "\x1B[1;34m", uinfo.nodename, uinfo.sysname, "\x1B[0m", "\x1B[1;32m", s, "\x1B[0m");
    printf("$");
    free(cwd);
}

void lsh_loop()
{
    if (uname(&uinfo) != 0)
    {
        perror("uname");
        exit(EXIT_FAILURE);
    }

    char *cwd = (char *)malloc(sizeof(char) * 1024);
    cwd = getcwd(cwd, 1024);
    strcpy(present_dir[0], uinfo.nodename);
    strcpy(present_dir[1], uinfo.sysname);
    strcpy(present_dir[2], cwd);

    free(cwd);

    print();

    char *hist[HISTORY_LEN];    //cream istoricul
    int current = 0;    //index-ul cu care il vom parcurge

    for(int i = 0; i < HISTORY_LEN; i ++)
        hist[i] = NULL;    //ne asiguram ca este gol
    for(int i = 0; i < ENV_VAR ; i++){
    env_name[i] = NULL;
    env_value[i] = NULL;
    }
    //intram in bucla de shell
    int status = 1;
    while (status)
    {
        int childpid=-1;
        //pointer unde vom retine adresa bufferului care contine textul citit
        char *com;
        char *token_arr[200];
        ssize_t size = 0;
        //fiind null la apelul getline, trebuie sa dam free(com) la final (fiind useri)
        //citim comanda din streamul de input si o salvam in com, iar deoarece com e NULL initial, se face abstractie de size
        getline(&com, &size, stdin);


        write(1, ">>", 3);

        if (strcmp(com, "\n") == 0) //daca avem doar o linie noua, reluam bucla de la inceput
        {
            print();

            continue;
        }
        int i = 0, j = 0;
        //in cazul in care citim mai multe comenzi legate, acestea trebuie separate prin ;, iar apoi le salvam intr-un vector de comenzi
        token_arr[j] = strtok(com, ";");
        free(hist[current]);    //eliberam istoricul curent
        hist[current] = strdup(token_arr[j]);   //incarcam comanda in istoric prin functia de duplicare
        current = (current + 1) % HISTORY_LEN;  //actualizam index-ul din istoric
        while (token_arr[j] != NULL)
        {
            j++;
            token_arr[j] = strtok(NULL, ";");
        if(token_arr[j] != NULL){
            free(hist[current]);
            hist[current] = strdup(token_arr[j]);
            current = (current + 1) % HISTORY_LEN;
        }
        }


        for (i = 0; i < j; i++)
        {
            char *token;
            char st[100][100];
            char *pipe_check;
            pipe_check = strstr(token_arr[i]," | ");
            if(pipe_check){
                token = strtok(token_arr[i], " ");
                //vector de comenzi
                struct command *comm = (struct command*) malloc(10*sizeof(struct command));
            for(int k=0;k<10;k++)
                {
            comm[k].argv = (char **) malloc(100*sizeof(char*));
            for(int cuv=0;cuv<100;cuv++)
                comm[k].argv[cuv]=(char*) malloc(20*sizeof(char));
            }
            int nr_com = 0;
                int nr_arg = 0;
                int lit = 0;
                while(token){

                    if (strcmp(token,"|")==0){
                        comm[nr_com].argv[nr_arg] = NULL;
                        nr_com++;
                        nr_arg = 0;
                    }

                    else{
                        strncpy(comm[nr_com].argv[nr_arg], token,strlen(token));
                        comm[nr_com].argv[nr_arg][strlen(token)]='\0';
                        nr_arg++;
                            }
                            token = strtok(NULL, " ");
                }
                        comm[nr_com].argv[nr_arg]=NULL;
                        fork_pipes(nr_com, comm);
                            for(int k = 0 ; k <10; k++){
                                for(int c =0; c<100; c++)
                                    free(comm[0].argv[c]);
                          	free(comm[k].argv);
                		}
                		free(comm);
            }
            else{
                token = strtok(token_arr[i], " \n\r\t");
                if (token == NULL)
                    continue;

                if (strcmp(token, "quit") == 0)
                    exit(0);
                else if (strcmp(token, "cd") == 0)
                    cd(token, present_dir[2]);
                else if(strcmp(token, "pwd") == 0){
                    childpid=fork();
                    if(childpid==0){
                        char* argv[] = {"pwd", NULL};
                        execv("/bin/pwd", argv);
                    }
                }
                else if(strcmp(token, "ls") == 0){
                    childpid=fork();
                    if(childpid==0){
                    char* argv[] = {"ls", NULL};
                    execv("/bin/ls", argv);
                    }
                }
                else if(strcmp(token, "history") == 0){
                        history(hist, current);
                }
                else if(strcmp(token, "clear_history") == 0)
                    clear_history(hist);
                else if(strcmp(token, "mkdir") == 0){
                    token = strtok(NULL, " \0");
                    childpid=fork();
                    if(childpid==0){
                        char* argv[] = {"mkdir", token, NULL};
                        execv("/bin/mkdir", argv);
                    }
                }
                else if(strcmp(token, "rmdir") == 0)
                {
                    token = strtok(NULL, "\0");
                    childpid=fork();
                    if(childpid==0){
                        char* argv[] = {"rmdir", token, NULL};
                        execv("/bin/rmdir", argv);
                    }
                }
                else if(strcmp(token, "echo") == 0)
                {
                    token = strtok(NULL," \0");
                    while(token){
                        if(token[0]=='$'){
                            char aux[301];
                            strcpy(aux, token+1);
                        searchVarEnv(aux);
                        }
                        else{
                            printf("%s ", token);
                        }
                        token = strtok(NULL, " ");
                    }
                }
                else
                {
                    if(token != NULL)
                    {
                        char name[100];
                        char value[100];
                        int count=0;
                        if(token[0]!='='&&token[strlen(token)-1]!='=')
                        {
                            for (i=1;i<strlen(token)-1;i++){
                                if(token[i]=='=')
                                    count+=1;
                            }
                            if(count==1){
                                token = strtok(token, "=");
                                strcpy(name,token);
                                token = strtok(NULL, "\0");
                                strcpy(value,token);
                                createVarEnv(name,value);
                            }
                            else{
                                write(1, "Comanda nu a fost gasita\n", 25);
                            }
                        }
                        else{
                            write(1, "Comanda nu a fost gasita\n", 25);
                        }
                    }
                }
            }
        }

        free(com);
        print();

    }
}

void welcome_message(void)
{
    write(1, "BINE ATI VENIT!\n", 16);

}


int main(int argc, char **argv)
{
    welcome_message();
    // start shell loop
    lsh_loop();
    // char *echo[] = {"echo", "Test", 0 };
    // char *grep[] = { "grep", "es", 0 };

    //struct command cmd [] = { {echo}, {grep}};

    //fork_pipes (1, cmd);
    // return 0;
    return EXIT_SUCCESS;
}
