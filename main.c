#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <wiringPiI2C.h>
#include "bme280.h"

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

void* ReadSensors()
{
    int lastValue=0;
    do
    {
        pthread_mutex_lock(&lock);
        int fd = wiringPiI2CSetup(BME280_ADDRESS);
        if(fd < 0) 
        {
            printf("Device not found");
            return -1;
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

        // printf("{\"sensor\":\"bme280\", \"humidity\":%.2f, \"pressure\":%.2f,"
        // " \"temperature\":%.2f, \"altitude\":%.2f, \"timestamp\":%d}\n",
        // h, p, t, a, (int)time(NULL));
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
    pthread_create(&t2, NULL, ReadSensors, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}