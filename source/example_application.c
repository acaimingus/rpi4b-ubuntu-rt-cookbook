/**
 * @file    example_application.c
 * @brief   A short demonstration application for a real-time task using POSIX libraries on Real-Time Ubuntu.
 *
 * @details Please refer to the README.md
 *
 * @author  clpter
 * @date    07.06.2025
 * @version 1.0
 *
 * @note    This file is part of the Raspberry Pi PREEMPT_RT tuning demo.
 * @warning This is for educational purposes; timing results may differ
 *          depending on hardware and kernel version.
 */


#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <gpiod.h>
#include <signal.h>

/*
       Lib for gpio: sudo apt install libgpiod-dev
       When compiling link against libgpiod library: gcc program.c -lgpiod
       To run on Isolated core: taskset -c 1 sudo ./a.out
*/

/*
        This program initializes two pthreads.
        One pthread sets the GPIO output to 1.
        One pthread sets the GPIO output to 0.
*/

/*
       Define task period and GPIO port.
*/
#define PERIOD_NS 1000000       // Period of 1000 microseconds.
#define GPIO_LINE 23            // Defines GPIO port to use.

/*
       Global variables for GPIO.
*/
struct gpiod_chip *chip;
struct gpiod_line *line;

/*
       Called when the program receives a signal to terminate.
*/
void shutdown(int signum){
        gpiod_chip_close(chip);
        printf("Program finnished \n");
        exit(0);
}

/*
       Initialize global variables for GPIO.
*/
static void init_GPIO(){
        //open chip
        chip = gpiod_chip_open("/dev/gpiochip0");

        //open line
        line = gpiod_chip_get_line(chip, GPIO_LINE);
        gpiod_line_request_output(line, "square_wave", 0);
}

 /*
        This struct specifies parameter for a pthread.
        - next_period defines an absolute time as a timespec object (The time when a task will be woken up). 
        - period_ns specifies how long a period will be.
        - value specifies if the GPIO pin is 0 or 1.
 */
struct task_info {
        struct timespec next_period;
        long period_ns;
        int value;
};
 
/*
        - Increases the period of the task.
        - Sets time when the Task has to wake up next.
*/
static void inc_period(struct task_info *pinfo) 
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;
 
        // timespec nanoseconds overflow
        // 1000000000ns = 1s
        while (pinfo->next_period.tv_nsec >= 1000000000) {
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}
 
/*
        Defines what the task has to do.
*/
static void do_rt_task(int value)
{
        gpiod_line_set_value(line, value);   
}
 
/*
       Let the task wait until the period ends
*/ 
static void wait_rest_of_period(struct task_info *pinfo)
{
        inc_period(pinfo);
 
        // Sleeps until the absolute time specified by "pinfo"
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}

/*
        Main function of the pthread
*/
void *simple_cyclic_task(void* args)
{
        // Get arguments as task_info object
        struct task_info* pinfo = (struct task_info*)args;

        printf("created pthread, value = %i starttime = %i\n",pinfo->value, (int)pinfo->next_period.tv_nsec);
        
        while (1) {
                do_rt_task(pinfo->value);
                wait_rest_of_period(pinfo);
        }
        return 0;
}

 /*
        Set period and start time of two threads: 
        - thread1 sets gpio outut to 1 and thread two sets output to 0
        - *attr sets the priority and sceduling policy
*/
int tasks_init(pthread_attr_t *attr){

        // Define starting time of threads
        int ret;
        struct task_info start1, start2;

        start1.period_ns = PERIOD_NS;
        clock_gettime(CLOCK_MONOTONIC, &start1.next_period);
        start1.next_period.tv_nsec = 0;
        //Start in two seconds
        start1.next_period.tv_sec += 2;

        start2 = start1;
        // Starts in two seconds + half a period
        start2.next_period.tv_nsec = PERIOD_NS/2;
        //Set task values
        start1.value = 1;
        start2.value = 0;

        //Initialize global GPIO
        init_GPIO();

        // Create two pthreads with the specified attributes
        pthread_t thread1, thread2;
        
        ret = pthread_create(&thread1, attr, simple_cyclic_task, &start1);
        if (ret) {
                printf("create pthread failed\n");
                return ret;
        }

        ret = pthread_create(&thread2, attr, simple_cyclic_task, &start2);
        if (ret) {
                printf("create pthread failed\n");
                return ret;
        }

 
        // Join the threads and wait until it is done
        ret = pthread_join(thread1, NULL);
        if (ret){
                printf("join pthread failed: %m\n");
                return ret;
        }
        ret = pthread_join(thread2, NULL);
        if (ret){
                printf("join pthread failed: %m\n");
                return ret;
        }
}

/*
        Initializes the program and pthreads
*/
int main(int argc, char* argv[])
{       
        struct sched_param param;
        pthread_attr_t attr;
        int ret;
 
        // Set a handler for SIGINT
        signal(SIGINT, shutdown);


        // Lock memory
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }
        
        
        // Initialize pthread attributes (default values)
        ret = pthread_attr_init(&attr);
        if (ret) {
                printf("init pthread attributes failed\n");
                return ret;
        }

 
        // Set scheduler policy of pthread
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) {
                printf("pthread setschedpolicy failed\n");
                return ret;
        }


        // Set priority of pthread
        param.sched_priority = 80;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
                printf("pthread setschedparam failed\n");
                return ret;
        }


        // Use scheduling parameters of attr
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                printf("pthread setinheritsched failed\n");
                return ret;
        }
 
        // Start two threads
        ret = tasks_init(&attr);

        return ret;
}
