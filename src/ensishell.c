/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "variante.h"
#include "readcmd.h"

#ifndef VARIANTE
#error "Variante non défini !!"
#endif

#include <string.h>
#include <assert.h>

#define SIZE_ARRAY(a) (sizeof((a))/sizeof((a)[0]))

struct jobs_ls {
    pid_t pid;              // pid of background child
    char  name[40];         // used for display end of jobs
    struct jobs_ls *next;   // next background child
};

void jobs_print(struct jobs_ls *it)
{
    printf("Current jobs :\n");
    while (it != NULL) {
        printf("%8d : %s\n", it->pid, it->name);
        it = it->next;
    }
}

void jobs_endmsg(struct jobs_ls *j)
{
    printf("%8d : %s terminated\n", j->pid, j->name);
}

int main()
{
    struct jobs_ls *jobs = NULL;

    printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

    while (1) {
        struct cmdline *l;
        int i, j;
        char *prompt = "ensishell>";

        l = readcmd(prompt);

        /* If input stream closed, normal termination */
        if (!l) {
            printf("exit\n");
            exit(EXIT_SUCCESS);
        }

        if (l->err) {
            /* Syntax error, read another command */
            printf("error: %s\n", l->err);
            continue;
        }

        if (l->in) {
            printf("in: %s\n", l->in);
        }
        if (l->out) {
            printf("out: %s\n", l->out);
        }
        if (l->bg) {
            printf("background (&)\n");
        }

        /* Display each command of the pipe */
        for (i = 0; l->seq[i] != 0; i++) {
            char **cmd = l->seq[i];

            printf("seq[%d]: ", i);
            for (j = 0; cmd[j] != 0; j++) {
                printf("'%s' ", cmd[j]);
            }
            printf("\n");
        }

        int pid_w, status;

        pid_w = waitpid(0, &status, WNOHANG);
        if (pid_w > 0) {
            struct jobs_ls *it, *it_prev;

            it      = jobs;
            it_prev = jobs;
            while (it != NULL) {
                if (it->pid == pid_w) {
                    jobs_endmsg(it);
                    if ((it_prev == jobs) && (it->next == NULL)) {
                        jobs = NULL;
                    } else {
                        it_prev->next = it->next;
                    }
                    free(it);
                    break;
                }
                it_prev = it;
                it      = it->next;
            }
            assert(it != NULL);
        }

        if (l->seq[0] != 0) {
            if (!strcmp(l->seq[0][0], "jobs")) {
                jobs_print(jobs);
            } else {
                pid_t pid = fork();

                if (pid) {
                    // father
                    int status;

                    if (l->bg) {
                        struct jobs_ls *bg = malloc(sizeof(*bg));
                        strncpy(bg->name, l->seq[0][0], SIZE_ARRAY(bg->name));
                        bg->name[SIZE_ARRAY(bg) - 1] = '\0';
                        bg->pid  = pid;
                        if (jobs) {
                            bg->next = jobs->next;
                            jobs->next = bg;
                        } else {
                            jobs = bg;
                        }
                    } else {
                        waitpid(pid, &status, 0);
                    }

                } else {
                    // child
                    int err;

                    for (i = 0; l->seq[i] != 0; i++) {
                        err = execvp(l->seq[0][0], l->seq[0]);
                    }
                    fprintf(stderr, "error %d\n", err);
                }

            }
        }
    }
}
