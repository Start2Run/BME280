#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <semaphore.h> 
#include <wiringPiI2C.h>
#include "bme280.h"

pthread_mutex_t lockMutex;
sem_t timerLockSemaphore;
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
            pthread_mutex_lock(&lockMutex);
            consoleInput = number;
            pthread_mutex_unlock(&lockMutex);
        }
        sleep(1);
    }
    while(1);
}

void* ReadSensors()
{
    int lastValue=0;
    do
    {
        sem_wait(&timerLockSemaphore);
        int fd = wiringPiI2CSetup(BME280_ADDRESS);
        if(fd < 0) 
        {
            printf("Device not found");
            return;
        }
        bme280_calib_data cal;
        readCalibrationData(fd, &cal);

        wiringPiI2CWriteReg8(fd, 0xf2, 0x01);   // humidity oversampling x 1
        wiringPiI2CWriteReg8(fd, 0xf4, 0x25);   // pressure and temperature oversampling x 1, mode normal

        bme280_raw_data raw;
        getRawData(fd, &raw);
            
        int32_t t_fine = getTemperatureCalibration(&cal, raw.temperature);
        float t = compensateTemperature(t_fine); // C
        float p = compensatePressure(raw.pressure, &cal, t_fine) / 100; // hPa
        float h = compensateHumidity(raw.humidity, &cal, t_fine);       // %
        float a = getAltitude(p);                         // meters

        pthread_mutex_lock(&lockMutex);
        printf("{\"sensor\":\"bme280\", \"humidity\":%.2f, \"pressure\":%.2f,"
        " \"temperature\":%.2f, \"altitude\":%.2f, \"timestamp\":%d}\n",
        h, p, t, a, (int)time(NULL));
        pthread_mutex_unlock(&lockMutex);
    }
    while(1);
}

void* Timer()
{
    while(1)
    {
        sem_post(&timerLockSemaphore);
        sleep(3);
    }
}

int main(void)
{
    pthread_t t1,t2,timer;
    if(pthread_mutex_init(&lockMutex, NULL) != 0)
    {
       printf("Mutex initialization failed.\n");
       return 1;
    }

    sem_init(&timerLockSemaphore, 0, 1);

    pthread_create(&t1, NULL, ReadConsoleInput, NULL);
    pthread_create(&t2, NULL, ReadSensors, NULL);
    pthread_create(&timer, NULL, Timer, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(timer, NULL);

    //sem_destroy(&timerLockSemaphore);
    return 0;
}