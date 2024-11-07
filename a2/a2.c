#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#define N 50
pthread_mutex_t lock;
pthread_cond_t cond;
sem_t semaphores[2];
sem_t semaphores3;
sem_t barrier;
sem_t start;
sem_t end;
/// named sema
sem_t *sem1;
sem_t *sem2;

void P(sem_t *sem)
{
    sem_wait(sem);
}
void V(sem_t *sem)
{
    sem_post(sem);
}
void *functieP5(void *th_id)
{
    int thread_id = *((int *)th_id);
    if (thread_id == 3)
    {
        info(BEGIN, 5, thread_id);
        info(END, 5, thread_id);
        sem_post(sem2); /// semaph the processs 4 th 5
    }
    else if (thread_id == 2)
    {
        sem_wait(sem1);
        info(BEGIN, 5, thread_id);
        info(END, 5, thread_id);
    }
    else
    {
        info(BEGIN, 5, thread_id);
        info(END, 5, thread_id);
    }

    return NULL;
}

void *functie(void *th_id) // process 4
{
    int thread_id = *((int *)th_id);

    if (thread_id == 1)
    {
        P(&semaphores[0]); /// 1 0
        info(BEGIN, 4, 1); /// 0 0
        V(&semaphores[1]); /// start sec
        P(&semaphores[0]); // block first

        info(END, 4, 1);
    }
    else if (thread_id == 3)
    {
        P(&semaphores[1]);
        info(BEGIN, 4, 3);
        info(END, 4, 3);
        V(&semaphores[0]);
    }
    else if (thread_id == 5) ///  D
    {
        sem_wait(sem2);
        info(BEGIN, 4, 5);
        info(END, 4, 5);
        sem_post(sem1);
    }
    else
    {
        info(BEGIN, 4, thread_id);
        info(END, 4, thread_id);
    }
    return NULL;
}

int threadNo = 0;
void *lim_area777(void *th_id)
{
    int thread_id = *((int *)th_id);
    if (thread_id == 14)
    {
        info(BEGIN, 7, 14);
        V(&start);
        pthread_mutex_lock(&lock);
        threadNo++;
        while (threadNo < 5)
        {
            pthread_cond_wait(&cond, &lock);
        }
        V(&end);
        threadNo--;
        info(END, 7, 14);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        P(&semaphores3);
        P(&start);
        V(&start);
        info(BEGIN, 7, thread_id);
        /// increment thred nr
        pthread_mutex_lock(&lock);
        threadNo++;
        if (threadNo == 6 )
        {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&lock);

        P(&end);
        V(&end);
        pthread_mutex_lock(&lock);
        threadNo--;
        pthread_mutex_unlock(&lock);
        info(END, 7, thread_id);
        V(&semaphores3);
    }

    return NULL;
}

int main()
{
    init();
    /// semaphore init
    sem_init(&start, 0, 0);
    sem_init(&end, 0, 0);
    sem_init(&semaphores[0], 0, 1);
    sem_init(&semaphores[1], 0, 0);
    /// semaphores for task 3
    if (sem_init(&semaphores3, 0, 5) < 0) /// at most 6 threads can run
    {
        perror("Error creating the semaphore");
        exit(2);
    }
    sem_init(&barrier, 0, 0);

    /// named sema creaion
    sem1 = sem_open("/sema1", O_CREAT, 0600, 0);
    if (sem1 == SEM_FAILED)
    {
        perror("Error creating the semaphore set");
        exit(2);
    }
    sem2 = sem_open("/sema2", O_CREAT, 0600, 0);
    if (sem2 == SEM_FAILED)
    {
        perror("Error creating the semaphore set");
        exit(2);
    }

    info(BEGIN, 1, 0);

    pid_t pid1, pid2, pid3, pid5, pid6, pid7;
    if ((pid1 = fork()) == 0)
    {
        info(BEGIN, 2, 0);
        if ((pid5 = fork()) == 0)
        {
            /// process 4 -- create threads
            info(BEGIN, 4, 0);
            int thId[5];
            pthread_t thread[5];

            /// thread init
            for (int i = 0; i < 5; i++)
            {
                thId[i] = i + 1;
                pthread_create(&thread[i], NULL, &functie, &thId[i]);
            }
            for (int i = 0; i < 5; i++)
            {
                pthread_join(thread[i], NULL);
            }

            info(END, 4, 0);
        }
        if (pid5 > 0)
        {
            if ((pid6 = fork()) == 0)
            {
                info(BEGIN, 5, 0);
                int thIdP5[4];
                pthread_t threadP5[4];
                /// thread
                for (int i = 0; i < 4; i++)
                {
                    thIdP5[i] = i + 1;
                    pthread_create(&threadP5[i], NULL, &functieP5, &thIdP5[i]);
                }
                for (int i = 0; i < 4; i++)
                {
                    pthread_join(threadP5[i], NULL);
                }
                info(END, 5, 0);
            }
            if (pid6 > 0)
            {
                waitpid(pid5, NULL, 0);
                waitpid(pid6, NULL, 0);
                info(END, 2, 0);
            }
        }
    }
    else if (pid1 > 0)
    {

        if ((pid2 = fork()) == 0)
        {
            info(BEGIN, 3, 0);
            if ((pid7 = fork()) == 0)
            {
                /////// process 7  -- 50 threads +  th 14 cond
                info(BEGIN, 7, 0);
                int thId1[N];
                pthread_t thread1[N];
                if (pthread_mutex_init(&lock, NULL) != 0)
                {
                    perror("Cannot initialize the lock");
                    exit(2);
                }
                // cond_init
                if (pthread_cond_init(&cond, NULL) != 0)
                {
                    perror("Cannot initialize the condition variable");
                    exit(3);
                }
                for (int i = 0; i < N; i++)
                {
                    thId1[i] = i + 1;
                    pthread_create(&thread1[i], NULL, &lim_area777, &thId1[i]);
                }

                for (int i = 0; i < N; i++)
                {
                    pthread_join(thread1[i], NULL);
                }
                info(END, 7, 0);
            }
            if (pid7 > 0)
            {
                waitpid(pid7, NULL, 0);
                info(END, 3, 0);
            }
        }
        if (pid2 > 0)
        {
            if ((pid3 = fork()) == 0)
            {
                info(BEGIN, 6, 0);
                info(END, 6, 0);
            }
            if (pid3 > 0)
            {
                waitpid(pid1, NULL, 0);
                waitpid(pid2, NULL, 0);
                waitpid(pid3, NULL, 0);
                info(END, 1, 0);
            }
        }
    }
    /// sem destroy
    sem_destroy(&semaphores[0]);
    sem_destroy(&semaphores[1]);
    sem_destroy(&semaphores3);
    sem_close(sem2);
    sem_close(sem1);
    sem_unlink("/sema1");
    sem_destroy(sem1);
    sem_unlink("/sema2");
    sem_destroy(sem2);
    if (pthread_mutex_destroy(&lock) != 0)
    {
        perror("Cannot destroy the lock");
        exit(6);
    }
    if (pthread_cond_destroy(&cond) != 0)
    {
        perror("Cannot destroy the condition variable");
        exit(9);
    }

    return 0;
}
