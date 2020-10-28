#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>
    
typedef struct {
    int first;
    int second;
} pair;
    
// recursive function to make child processes
void makeChildren(int level, int curPrNum, int mxLvl, int shmid, int prInLvl[])
{
    printf("Executing Process | Level %d | Process %d | Parent %d\n", level, getpid(), getppid());
    
    // temp
    int x;
    
    // final level of tree
    if(level == mxLvl)
    {
        printf("Exiting Process | Level %d | Process %d | Parent %d\n", level, getpid(), getppid());
        return;
    }
    
    // number of processes made in the next level made till now
    int prLvl = (curPrNum - 1) * curPrNum / 2;
    
    // printf("...%d forking...\n", getpid());
    for (int i = 1; i <= curPrNum; i++)
    {
        // child
        if(!fork())
        {
            // array of pairs that contains pid of children and their exit status
            pair *proc = shmat(shmid, 0, 0);
            proc[prLvl + i + prInLvl[1 + level] - 1].first = getpid();
    
            // stop the process untill all the processes in the current level are forked
            kill(getpid(), SIGSTOP);
            // printf("Continued %d\n", getpid());
    
            // call the recursive function to create children
            makeChildren(level + 1, i + prLvl, mxLvl, shmid, prInLvl);
    
            // detach shared memory
            shmdt(proc);
    
            // exit with status = number of the process in current level
            exit(i + prLvl);
        }
    }
    
    for (int i = 1; i <= curPrNum; i++)
    {
        waitpid(-1, &x, WUNTRACED);
        // printf("Waiting for %d\n", waitpid(-1, &x, WUNTRACED));
    }
    
    pair *proc = shmat(shmid, 0, 0);
    
    /* if the process is not the last process of the current level stop it 
    at the last process resume all the other processes of the level*/
    if(curPrNum == prInLvl[level + 1] - prInLvl[level])
    {
        for (int i = prInLvl[level] + 1; i < prInLvl[level] + curPrNum; i++)
        {
            kill(proc[i - 1].first, SIGCONT);
        }
    }
    else
    {
        kill(getpid(), SIGSTOP);
    }
    
    // after all the children are forked continue them
    for (int i = 1; i <= curPrNum; i++)
    {
        // waitpid(proc[i + prInLvl[level]].first, &proc[i + prInLvl[level]].second, WUNTRACED);
        kill(proc[prLvl + i + prInLvl[1 + level] - 1].first, SIGCONT);
    }
    
    // wait for all children to exit
    for (int i = 1; i <= curPrNum; i++)
    {
        waitpid(proc[prLvl + i + prInLvl[1 + level] - 1].first, &proc[prLvl + i + prInLvl[1 + level] - 1].second, 0);
    }
    
    printf("Exiting process | Level %d | Process %d | Parent %d\n", level, getpid(), getppid());
    printf("Children :-\n");
    for (int i = 1; i <= curPrNum; i++)
    {
        printf("Process %d | Exit Status %d\n", proc[prLvl + i + prInLvl[1 + level] - 1].first, proc[prLvl + i + prInLvl[1 + level] - 1].second);
    }    
    
    // detach shared memory
    shmdt(proc);
}
    
int main(int argv, char *argc[])
{
    printf("Executing main process %d\n", getpid());
    
    // number of children of main processes
    int mpc = atoi(argc[1]);
    
    // number of levels
    int lvl = atoi(argc[2]);
    
    // total number of processes
    int totPr = 1 + mpc;
    // temp var
    int x = mpc;
    // number of processes till ith level
    int prInLvl[lvl + 1];
    prInLvl[0] = 0;
    prInLvl[1] = 1;
    
    // calculate total number of processes
    for (int i = 1; i < lvl; i++)
    {
        prInLvl[i + 1] = totPr;
        totPr += x * (x + 1) / 2;
        x = x * (x + 1) / 2;
    }
    printf("Total Processes : %d\n", totPr);
    
    // create a shared memory
    int shmid = shmget(IPC_PRIVATE, totPr * sizeof(pair), 0666 | IPC_CREAT);
    
    // create mpc children for main process
    for (int i = 1; i <= mpc; i++)
    {
        // child
        if(!fork())
        {
            // printf("%d\n", getpid());
            // array of pairs that contains pid of children and their exit status
            pair *proc = shmat(shmid, 0, 0);
            proc[i].first = getpid();
    
            // stop the process untill all the processes in the current level are forked
            kill(getpid(), SIGSTOP);
            // printf("Continued %d\n", getpid());
    
            // call the recursive function to create children
            makeChildren(1, i, lvl, shmid, prInLvl);
    
            // detach shared memory
            shmdt(proc);
    
            // exit with status = number of the process in current level
            exit(i);
        }
    }
    
    for (int i = 1; i <= mpc; i++)
    {
        waitpid(-1, &x, WUNTRACED);
    }
    
    pair *proc = shmat(shmid, 0, 0);
    
    // printf("x %d %d\n", proc[1].first, proc[2].first);
    
    // after all the children are forked continue them
    for (int i = 1; i <= mpc; i++)
    {
        kill(proc[i].first, SIGCONT);
    }
    
    // wait for all children to exit
    for (int i = 1; i <= mpc; i++)
    {
        waitpid(proc[i].first, &proc[i].second, 0);
    }
    
    printf("Exiting main process %d\n", getpid());
    
    // detach shared memory
    shmdt(proc);
    
    // destroy the shared mem segment
    shmctl(shmid, IPC_RMID, NULL);
    
    return 0;
}