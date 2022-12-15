#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

sem_t *machines;
int withoutwash = 0;
int wastedtime = 0;
pthread_mutex_t wash_lock;
pthread_mutex_t timelock;
pthread_mutex_t wastedtime_lock;

#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

struct studentinfo
{
    int starttime;
    int endtime;
    int patiencetime;
    int studentnumber;
    time_t time0;
    struct timespec timer;
};

void *executethread(void *arg)
{
    struct studentinfo *student = (struct studentinfo *)arg;

    // printf(" entered : %d\n", student->studentnumber);

    pthread_mutex_unlock(&timelock);

    sleep(student->starttime);//sleep till arrival
    printf("%ld: Student %d arrives\n", time(NULL) - student->time0, student->studentnumber);
    if (sem_timedwait(machines, &student->timer) == -1)//wait for machines
    {
        printf(BRED "%ld: Student %d leaves without washing\n" ANSI_RESET, time(NULL) - student->time0 - 1, student->studentnumber);//student didn't get the machine
        pthread_mutex_lock(&wastedtime_lock);
        wastedtime += student->patiencetime;
        pthread_mutex_unlock(&wastedtime_lock);
        pthread_mutex_lock(&wash_lock);
        withoutwash++;
        pthread_mutex_unlock(&wash_lock);
    }

    else
    {
        time_t currtime = time(NULL);//student got the machine
        // printf("%d %ld %d\n", student->studentnumber, currtime - student->starttime, student->patiencetime);
        // if (currtime - student->starttime - student->time0 <= student->patiencetime)
        {

            printf(BGRN "%ld: Student %d starts washing\n" ANSI_RESET, currtime - student->time0, student->studentnumber);

            sleep(student->endtime);

            printf(BYEL "%ld: Student %d leaves after washing\n" ANSI_RESET, time(NULL) - student->time0, student->studentnumber);
            pthread_mutex_lock(&wastedtime_lock);
            wastedtime += currtime - student->time0 - student->starttime;
            // printf("%d %ld\n", student->studentnumber, currtime - student->time0 - student->starttime);
            pthread_mutex_unlock(&wastedtime_lock);
        }
        // else
        // {
        //     // printf("Hi\n");
        //     pthread_mutex_lock(&wash_lock);
        //     withoutwash++;
        //     printf("%ld: Student %d leaves without washing\n", currtime - student->time0, student->studentnumber);

        //     pthread_mutex_unlock(&wash_lock);
        // }
        sem_post(machines);
    }
}

int main()
{
    int n, m;
    scanf("%d %d", &n, &m);
    struct studentinfo students[n];
    pthread_mutex_init(&wash_lock, NULL);
    pthread_mutex_init(&wastedtime_lock, NULL);
    pthread_mutex_init(&timelock, NULL);
    pthread_t sthreads[n];
    sem_destroy(machines);
    machines = (sem_t *)malloc(sizeof(sem_t));
    sem_init(machines, 0, m);
    struct timespec universal;
    if (machines == SEM_FAILED)
    {
        printf("semaphore error\n");
        exit(1);
    }

    for (int i = 0; i < n; i++)
    {
        scanf("%d %d %d", &students[i].starttime, &students[i].endtime, &students[i].patiencetime);
        students[i].studentnumber = i + 1;
    }
    clock_gettime(CLOCK_REALTIME, &universal);//get the time at start
    int time0 = time(NULL);
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_lock(&timelock);
        students[i].time0 = time0;
        students[i].timer.tv_sec = universal.tv_sec + students[i].starttime + students[i].patiencetime + 1 ;
        students[i].timer.tv_nsec = universal.tv_nsec;

        int x = pthread_create(&sthreads[i], NULL, executethread, (void *)&students[i]);

        assert(x == 0);
    }
    for (int i = 0; i < n; i++)
    {
        pthread_join(sthreads[i], NULL);
    }
    sem_destroy(machines);
    printf("%d\n", withoutwash);
    printf("%d\n", wastedtime);
    if (withoutwash * 100.0 / n <= 25)
    {
        printf("No\n");
    }
    else
    {
        printf("Yes\n");
    }
}