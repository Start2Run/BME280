//Some of this code was created base on examples found at:  https://docs.oracle.com/cd/E19455-01/806-5257/attrib-16/index.html

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
#include "tcp-client.h"

pthread_mutex_t lockTempDesMutex,lockSensorValuesMutex,lockSendMessageMutex;
sem_t timerLockSensorSemaphore, timerLockPowerSemaphore;
float tempDesired,tempRoom=0;

void *ReadConsoleInput()
{
    do
    {
        float number;
        printf("\nEnter an integer beetween 5 and 30: "); 
        scanf("%f", &number);
        if(number<5 || number>30)
        {
            printf("\nThe entered value %.2f is not correct!\n", number);
        }
        else
        {
            pthread_mutex_lock(&lockTempDesMutex);
            tempDesired = number;
            pthread_mutex_unlock(&lockTempDesMutex);

            char tempDesString[10];
            sprintf(tempDesString,"TD%.2f\n",number);
            
            pthread_mutex_lock(&lockSendMessageMutex);
                sendMessage(tempDesString);
            pthread_mutex_unlock(&lockSendMessageMutex);
            
            memset(tempDesString, 0, strlen(tempDesString));
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
        sem_wait(&timerLockSensorSemaphore);
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

        pthread_mutex_lock(&lockSensorValuesMutex);
            tempRoom=t;
        pthread_mutex_unlock(&lockSensorValuesMutex);

        char temperature[10], pressure[10], humidity[10];
        sprintf(temperature,"TP%.2f",t);
        sprintf(pressure,"PR%.2f",p);
        sprintf(humidity,"HU%.2f",h);
      
        // Build message
        char message[100];
        strcat(message,temperature);
        strcat(message,"\n");
        strcat(message,pressure);
        strcat(message,"\n");
        strcat(message,humidity);
        strcat(message,"\n");
        
        pthread_mutex_lock(&lockSendMessageMutex);
                sendMessage(message);
        pthread_mutex_unlock(&lockSendMessageMutex);
       
        //Clear message
        memset(message, 0, strlen(message));
    }
    while(1);
}

void* Power()
{
    do
    {
        sem_wait(&timerLockPowerSemaphore);
        float tempR,tempD,power;
        pthread_mutex_lock(&lockTempDesMutex);
            tempD=tempDesired;
        pthread_mutex_unlock(&lockTempDesMutex);

        pthread_mutex_lock(&lockSensorValuesMutex);
           tempR=tempRoom;
        pthread_mutex_unlock(&lockSensorValuesMutex);
        power=((tempD-tempR)/6)*100;
        if(power<0)
        {
            power=0;
        }
        else if(power>100)
        {
            power=100;
        }
        char powerString[10];
        sprintf(powerString,"PW%.2f\n",power);
        
        pthread_mutex_lock(&lockSendMessageMutex);
                sendMessage(powerString);
        pthread_mutex_unlock(&lockSendMessageMutex);
        
        memset(powerString, 0, strlen(powerString));
    }
    while(1);
}

void* Timer()
{
    while(1)
    {
        sem_post(&timerLockSensorSemaphore);
        sem_post(&timerLockPowerSemaphore);
        sleep(5);
    }
}

int main(void)
{
    printf("Enter the sever IP address: "); 
	scanf("%s", &address);

	printf("Enter port number: "); 
	scanf("%d", &port);

    if(connectClient(address,port)==-1)
	{
		return -1;
	}

    pthread_t t1,t2,t3,t4;
    if(pthread_mutex_init(&lockTempDesMutex, NULL) != 0 || pthread_mutex_init(&lockSensorValuesMutex, NULL) != 0 || pthread_mutex_init(&lockSendMessageMutex, NULL) != 0  )
    {
       printf("Mutex initialization failed.\n");
       return 1;
    }

    sem_init(&timerLockSensorSemaphore, 0, 1);
    sem_init(&timerLockPowerSemaphore, 0, 1);

    pthread_attr_t custom_sched_attr_H,custom_sched_attr_M,custom_sched_attr_L;	
    int fifo_max_prio, fifo_min_prio, fifo_mid_prio;	
    struct sched_param fifo_param_H,fifo_param_M,fifo_param_L;

    pthread_attr_init(&custom_sched_attr_H);	
    pthread_attr_init(&custom_sched_attr_M);
    pthread_attr_init(&custom_sched_attr_L);

    pthread_attr_setinheritsched(&custom_sched_attr_H, PTHREAD_EXPLICIT_SCHED);	
    pthread_attr_setinheritsched(&custom_sched_attr_M, PTHREAD_EXPLICIT_SCHED);
    // pthread_attr_setinheritsched(&custom_sched_attr_L, PTHREAD_EXPLICIT_SCHED);

    pthread_attr_setschedpolicy(&custom_sched_attr_H, SCHED_OTHER);	
    pthread_attr_setschedpolicy(&custom_sched_attr_M, SCHED_OTHER);	
    //pthread_attr_setschedpolicy(&custom_sched_attr_L, SCHED_OTHER);	

    fifo_max_prio = sched_get_priority_max(SCHED_OTHER);	
    fifo_min_prio = sched_get_priority_min(SCHED_OTHER);	
    fifo_mid_prio = (fifo_min_prio + fifo_max_prio)/2;	
    
    fifo_param_H.sched_priority = fifo_max_prio;	
    pthread_attr_setschedparam(&custom_sched_attr_H, &fifo_param_H);

    fifo_param_M.sched_priority = fifo_mid_prio;	
    pthread_attr_setschedparam(&custom_sched_attr_M, &fifo_param_M);

    // fifo_param_L.sched_priority = fifo_min_prio;	
    // pthread_attr_setschedparam(&custom_sched_attr_L, &fifo_param_L);
    
    pthread_create(&t1, NULL, ReadConsoleInput, NULL);
    pthread_create(&t2, &custom_sched_attr_M, ReadSensors, NULL);
    pthread_create(&t3, &custom_sched_attr_H, Timer, NULL);
    pthread_create(&t4, &custom_sched_attr_H, Power, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    sem_destroy(&timerLockSensorSemaphore);
    sem_destroy(&timerLockPowerSemaphore);
    return 0;
}