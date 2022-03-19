#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include<signal.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAXJOBS 1024

char **environ;

// TODO: you will need some data structure(s) to keep track of jobs
struct Job {
    int jobNumber;
    pid_t PID;
    int status;
    char command[1024];
    char sJobNumber[16];
    char sPID[16];
    struct Job* next;
    struct Job* prev;
};

//HEAD and TAIL nodes initialized at installhandlers
struct Job last = {0,0,-1,"\0", NULL, NULL};
struct Job first = {0,0,-1,"\0", NULL, NULL};


//status 0 - running 1- suspended - 2 - killed 3- background

struct Job* HEAD;
struct Job* TAIL;
int jobsNumber = 1;


void handle_sigchld(int sig) {
    // TODO
    pid_t currPID;
    while( (currPID = waitpid(-1, NULL, WNOHANG)) != -1){
        struct Job* node = HEAD->next;

        while(node->jobNumber != 0 && node->PID != currPID){
            node = node->next;
        }

        if(node->jobNumber == 0){
            return;
        } else {
            node->prev->next=node->next;
            node->next->prev=node->prev;
            free(node);

            write(STDOUT_FILENO, "\n[", 2);
            write(STDOUT_FILENO, node->sJobNumber, strlen(node->sJobNumber));
            write(STDOUT_FILENO, "] ", 2);

            write(STDOUT_FILENO, "(", 1);
            write(STDOUT_FILENO, node->sPID, strlen(node->sPID));
            write(STDOUT_FILENO, ") ", 2);

            write(STDOUT_FILENO, "killed ", 7);
            write(STDOUT_FILENO, node->command, strlen(node->command));
            write(STDOUT_FILENO, "\ncrash>", 7);
        }
        //TODO print if needed
        
    }
}

void handle_sigtstp(int sig) {
    // TODO
}

void handle_sigint(int sig) {
    // TODO
}

void handle_sigquit(int sig) {
    // TODO
}

void install_signal_handlers() {
    // TODO
    HEAD = &first;
    TAIL = &last;

    HEAD->next = TAIL;
    TAIL->prev = HEAD;



    struct sigaction act;
    act.sa_handler = handle_sigchld;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);


}

void spawn(const char **toks, bool bg) { // bg is true iff command ended with &
    // TODO

    pid_t childpid = -1;

    //starting background job


    if(!strcmp(toks[0], "jobs")){
        cmd_jobs(toks);
        return;
    } else if(!strcmp(toks[0], "slay")){
        cmd_slay(toks);
        return;
    }

    if (bg) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);
        childpid = fork();

        if(childpid == 0){
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            execvp(toks[0], toks);
            //executes only if ret value, does not return.
            fprintf(stderr, "ERROR: cannot run %s\n", toks[0]);
        } else if (childpid != -1) {
            //parent adds child process to list. 
            struct Job* newEnd = (struct Job*) malloc(sizeof(struct Job));

            //set new entry
            newEnd->jobNumber = jobsNumber++;
            newEnd->PID = childpid;
            newEnd->status = 0;
            strcpy(newEnd->command, toks[0]);
            sprintf(newEnd->sJobNumber, "%d", newEnd->jobNumber);
            sprintf(newEnd->sPID, "%d", newEnd->PID);
            newEnd->next = TAIL;
            newEnd->prev = TAIL->prev;

            TAIL->prev->next = newEnd;
            TAIL->prev = newEnd;

            printf("[%i] (%i) %s %s \n", newEnd->jobNumber, newEnd->PID, "", newEnd->command);
        

            sigprocmask(SIG_UNBLOCK, &mask, NULL);

            
        } else{
            fprintf(stderr, "ERROR: cannot run %s\n", toks[0]);
        }
    }
}

void cmd_jobs(const char **toks) {
    if(toks[1] != NULL){
        fprintf(stderr, "ERROR: %s takes no arguments \n", toks[0]);
        return;
    }
    
    // TODO BLOCK signals   
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);


    struct Job* currentNode = HEAD->next;
    while(currentNode->jobNumber != 0){
        if(currentNode->status == 0){
            printf("[%i] (%i) %s %s \n", currentNode->jobNumber, currentNode->PID, "running", currentNode->command);
        } else if (currentNode->status == 1) {
            printf("[%i] (%i) %s %s \n", currentNode->jobNumber, currentNode->PID, "suspended", currentNode->command);
        } else if (currentNode->status == 2) {
            printf("[%i] (%i) %s %s \n", currentNode->jobNumber, currentNode->PID, "killed", currentNode->command);
        } else if (currentNode->status == 3) {
            printf("[%i] (%i) %s %s \n", currentNode->jobNumber, currentNode->PID, "", currentNode->command);
        }
        currentNode=currentNode->next;
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void cmd_fg(const char **toks) {
    // TODO
}

void cmd_bg(const char **toks) {
    // TODO
}

void cmd_slay(const char **toks) {
    // TODO
    
    if(toks[2] != NULL || toks[1] == NULL ){
        fprintf(stderr, "ERROR: slay takes exactly one argument \n", toks[0]);
    }

    char* end;
    int i = 0;

    if(toks[1][0] == '%'){
        i++;
    }

    int convertedPID = strtol(toks[1]+i, &end, 10);
    
    if(*end == '\0'){
        //parse as jobid
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        if (i == 1){

            struct Job* node = HEAD->next;

            while(node->jobNumber != 0 && node->jobNumber != convertedPID){
                node=node->next;
            }
            if (node->jobNumber == convertedPID){
                kill(node->PID, SIGKILL);
            } else {
                fprintf(stderr, "ERROR: no job: %s \n", toks[1]); 
            }
        } else if (i == 0){
            struct Job* node = HEAD->next;

            while(node->jobNumber != 0 && node->PID != convertedPID){
                node=node->next;
            }
            if (node->PID == convertedPID){
                kill(node->PID, SIGKILL);
            } else {
                fprintf(stderr, "ERROR: no PID: %s \n", toks[1]); 
            }
        }
        sigprocmask(SIG_UNBLOCK, &mask, NULL);

       
    } else {
        fprintf(stderr, "ERROR: bad argument for slay: %s \n", toks[1]); 
        return;
    }
}

void cmd_quit(const char **toks) {
    if (toks[1] != NULL) {
        fprintf(stderr, "ERROR: quit takes no arguments\n");
    } else {
        exit(0);
    }
}

void eval(const char **toks, bool bg) { // bg is true iff command ended with &
    assert(toks);
    if (*toks == NULL) return;
    if (strcmp(toks[0], "quit") == 0) {
        cmd_quit(toks);
    // TODO: other commands
    } else {
        spawn(toks, bg);
    }
}

// you don't need to touch this unless you are submitting the bonus task
void parse_and_eval(char *s) {
    assert(s);
    const char *toks[MAXLINE+1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == ' ' || *s == '\t' || *s == '\n') ++s;
            if (*s != '&' && *s != ';' && *s != '\0') toks[t++] = s;
            while (strchr("&; \t\n", *s) == NULL) ++s;
            switch (*s) {
            case '&':
                bg = true;
                end = true;
                break;
            case ';':
                end = true;
                break;
            }
            *s++ = '\0';
        }
        toks[t] = NULL;
        eval(toks, bg);
    }
}

// you don't need to touch this unless you are submitting the bonus task
void prompt() {
    printf("crash> ");
    fflush(stdout);
}

// you don't need to touch this unless you are submitting the bonus task
int repl() {
    char *line = NULL;
    size_t len = 0;
    while (prompt(), getline(&line, &len, stdin) != -1) {
        parse_and_eval(line);
    }

    if (line != NULL) free(line);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    return 0;
}

// you don't need to touch this unless you want to add debugging options
int main(int argc, char **argv) {
    install_signal_handlers();
    return repl();
}