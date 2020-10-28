#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <string.h>

int **attachSHM(int shmid, int shmids[], int n)
{
    int **maze = (int **) shmat(shmid, 0, 0);

    for (int i = 0; i < n; i++)
    {
        maze[i] = (int *)shmat(shmids[i], 0, 0);
    }
    
    return maze;
}

void detachSHM(int **maze, int n)
{
    for (int i = 0; i < n; i++)
    {
        shmdt(maze[i]);
    }
    shmdt(maze);
}

int searchMJ(int shmid, int shmids[], int n, int r, int c)
{
    // printf("%d Searching on (%d,%d)...\n", getpid(), r, c);

    int **maze = attachSHM(shmid, shmids, n);

    int rc = 0, bc = 0;

    if(r == c && c == n - 1 && maze[r][c] == 0)
    {
        printf("(%d,%d) -> ", r, c);
        maze[r][c] = 2;
        return 1;
    }

    if(r < n - 1 && maze[r + 1][c] == 0)
    {
        if(!fork())
        {
            int stat = searchMJ(shmid, shmids, n, r + 1, c);

            if(stat) exit(1);
            exit(0);
        }
    }

    if(c < n - 1 && maze[r][c + 1] == 0)
    {
        if (!fork())
        {
            int stat = searchMJ(shmid, shmids, n, r, c + 1);

            if(stat) exit(1);
            exit(0);
        }
    }

    wait(&rc);
    wait(&bc);

    maze = attachSHM(shmid, shmids, n);

    // printf("%d %d %d\n", getpid(), rc, bc);

    if(rc == 256 || bc == 256 && maze[r][c] != 2)
    {
        printf("(%d,%d) -> ", r, c);
        maze[r][c] = 2;
        return 1;
    }
    else if(maze[r][c] != 2)
    {
        maze[r][c] = -1;
    }

    detachSHM(maze, n);

    return 0;
}

int main()
{
    int n;
    scanf("%d", &n);

    int shmid = shmget(IPC_PRIVATE, n * sizeof(int *), 0666 | IPC_CREAT);

    int shmids[n];

    for (int i = 0; i < n; i++)
    {
        shmids[i] = shmget(IPC_PRIVATE, n * sizeof(int), 0666 | IPC_CREAT);
    }
    
    int **maze = attachSHM(shmid, shmids, n);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            scanf("%d", &maze[i][j]);
        }
    }

    detachSHM(maze, n);

    int x;

    if(!fork())
    {
        int stat = searchMJ(shmid, shmids, n, 0, 0);

        if(stat) exit(1);
        exit(0);
    }

    wait(&x);
    // printf("x = %d\n", x);

    if(x == 256) printf("(0,0)\nFOUND :)\n");
    else printf("NOT FOUND :(\n");

    maze = attachSHM(shmid, shmids, n);

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            printf("%d ", maze[i][j]);
        }
        printf("\n");
    }

    detachSHM(maze, n);

    for (int i = 0; i < n; i++)
    {
        shmctl(shmids[i], IPC_RMID, 0);
    }
    shmctl(shmid, IPC_RMID, 0);

    return 0;
}

// 0 0 0 1
// 0 1 0 0
// 0 1 1 1
// 0 0 0 0