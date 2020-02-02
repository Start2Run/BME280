#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock;
int consoleInput;

void *ReadConsoleInput()
{
    do
    {
        int number;
        printf("Enter an integer beetween 5 and 30: "); 
        scanf("%d",&number);
        if(number<5 || number>30)
        {
            printf("\nThe entered value is not correct!\n");
        }
        else
        {
            pthread_mutex_lock(&lock);
            consoleInput=number;
            pthread_mutex_unlock(&lock);
        }
        sleep(1);
    }
    while(1);
}

void* DisplayValueToConsole()
{
    int lastValue=0;
    do
    {
        pthread_mutex_lock(&lock);
        if(consoleInput != lastValue)
        {
            lastValue = consoleInput;
            printf("\nThe value changed to %d \n\n",lastValue);
        }
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
    while(1);
}

int main(void)
{
    pthread_t t1,t2;
    if(pthread_mutex_init(&lock, NULL) != 0)
    {
       printf("Mutex initialization failed.\n");
       return 1;
    }

    pthread_create(&t1, NULL, ReadConsoleInput, NULL);
    pthread_create(&t2, NULL, DisplayValueToConsole, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    return 0;
}