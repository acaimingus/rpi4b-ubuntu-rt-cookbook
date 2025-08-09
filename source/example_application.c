/**
 * @file    example_application.c
 * @brief   Demonstration application for a real-time task using POSIX libraries on Real-Time Ubuntu.
 * @details This file is part of the Raspberry Pi PREEMPT_RT tuning demo.
 *          Please refer to the README.md for more information.
 * @author  clpter
 * @date    07.06.2025
 * @version 1.0
 * @note    For educational purposes; timing results may differ depending on hardware and kernel version.
 * @warning This is for educational purposes; timing results may differ depending on hardware and kernel version.
 */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <gpiod.h>
#include <signal.h>

/**
 * @def PERIOD_NS
 * @brief Period of the real-time task in nanoseconds (1000 microseconds).
 */
#define PERIOD_NS 1000000

/**
 * @def GPIO_LINE
 * @brief GPIO port to use.
 */
#define GPIO_LINE 23

/**
 * @var chip
 * @brief Global variable for the GPIO chip.
 */
struct gpiod_chip *chip;

/**
 * @var line
 * @brief Global variable for the GPIO line.
 */
struct gpiod_line *line;

/**
 * @brief Signal handler for program termination.
 * @param signum Signal number.
 * @details Closes the GPIO chip and exits the program.
 */
void shutdown(int signum){
        gpiod_chip_close(chip);
        printf("Program finished \n");
        exit(0);
}

/**
 * @brief Initializes the global variables for GPIO.
 * @details Opens the GPIO chip and requests the output line.
 */
static void init_GPIO(){
        chip = gpiod_chip_open("/dev/gpiochip0");
        line = gpiod_chip_get_line(chip, GPIO_LINE);
        gpiod_line_request_output(line, "square_wave", 0);
}

/**
 * @struct task_info
 * @brief Parameters for a pthread real-time task.
 * @var task_info::next_period Absolute time when the task will be woken up.
 * @var task_info::period_ns Period duration in nanoseconds.
 * @var task_info::value Value to set on the GPIO pin (0 or 1).
 */
struct task_info {
        struct timespec next_period;
        long period_ns;
        int value;
};

/**
 * @brief Increases the period of the task and sets the next wake-up time.
 * @param pinfo Pointer to the task_info structure.
 */
static void inc_period(struct task_info *pinfo) 
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;

        /* Handle nanoseconds overflow (1s = 1,000,000,000ns) */
        while (pinfo->next_period.tv_nsec >= 1000000000) {
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}

/**
 * @brief Sets the GPIO line to the specified value.
 * @param value Value to set (0 or 1).
 */
static void do_rt_task(int value)
{
        gpiod_line_set_value(line, value);   
}

/**
 * @brief Waits until the end of the current period.
 * @param pinfo Pointer to the task_info structure.
 * @details Increments the period and sleeps until the absolute time specified by pinfo.
 */
static void wait_rest_of_period(struct task_info *pinfo)
{
        inc_period(pinfo);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}

/**
 * @brief Main function for the real-time pthread.
 * @param args Pointer to a task_info structure.
 * @return NULL
 * @details Sets the GPIO value and waits for the next period in a loop.
 */
void *simple_cyclic_task(void* args)
{
        struct task_info* pinfo = (struct task_info*)args;
        printf("created pthread, value = %i starttime = %i\n", pinfo->value, (int)pinfo->next_period.tv_nsec);

        while (1) {
                do_rt_task(pinfo->value);
                wait_rest_of_period(pinfo);
        }
        return 0;
}

/**
 * @brief Initializes and starts two real-time pthreads.
 * @param attr Pointer to pthread attributes (priority and scheduling policy).
 * @return 0 on success, non-zero on failure.
 * @details
 * - Thread 1 sets GPIO output to 1.
 * - Thread 2 sets GPIO output to 0.
 * - Both threads are started with specified scheduling attributes.
 */
int tasks_init(pthread_attr_t *attr){

        int ret;
        struct task_info start1, start2;

        start1.period_ns = PERIOD_NS;
        clock_gettime(CLOCK_MONOTONIC, &start1.next_period);
        start1.next_period.tv_nsec = 0;
        start1.next_period.tv_sec += 2; /* Start in two seconds */

        start2 = start1;
        start2.next_period.tv_nsec = PERIOD_NS/2; /* Start in two seconds + half a period */

        start1.value = 1;
        start2.value = 0;

        init_GPIO();

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
        return 0;
}

/**
 * @brief Program entry point. Initializes and starts real-time pthreads.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, non-zero on failure.
 * @details
 * - Sets up signal handler for SIGINT.
 * - Locks memory to prevent paging.
 * - Initializes pthread attributes, scheduling policy, and priority.
 * - Starts two real-time threads.
 */
int main(int argc, char* argv[])
{       
        struct sched_param param;
        pthread_attr_t attr;
        int ret;

        signal(SIGINT, shutdown);

        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }

        ret = pthread_attr_init(&attr);
        if (ret) {
                printf("init pthread attributes failed\n");
                return ret;
        }

        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) {
                printf("pthread setschedpolicy failed\n");
                return ret;
        }

        param.sched_priority = 80;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
                printf("pthread setschedparam failed\n");
                return ret;
        }

        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                printf("pthread setinheritsched failed\n");
                return ret;
        }

        ret = tasks_init(&attr);

        return ret;
}
