#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include "threads.hpp"

#define TIME_LOG_FILE "time.log"
#define TEMP_LOG_FILE "temp.log"

#define TIMER_INVALID -1
#define TIMER_PAUSE 0
#define TIMER_START 1

int timer_state = TIMER_START;
Mutex mutex;
Condition condition;

void update_json(json_t *j_array, json_t *end)
{
    struct timeval tv;
    FILE *fp = fopen(TEMP_LOG_FILE, "w");
    gettimeofday(&tv, NULL);
    json_integer_set(end, tv.tv_sec);
    json_dumpf(j_array, fp, JSON_ENCODE_ANY);
    fclose(fp);
    system("/bin/cp " TEMP_LOG_FILE " " TIME_LOG_FILE "\n");
}

int update(json_t *j_array, json_t *end)
{
    int lock = 0;

    printf("Start timer.");
    while (1) {
        fflush(stdout);
        update_json(j_array, end);
        lock = mutex.trylock();
        if (lock == 0) {        // state changed
            switch (timer_state) {
                case TIMER_PAUSE:
                    condition.wait(mutex);
                    return 0;
                default:
                    mutex.unlock();
                    return -1;
            }
        }
        printf(".");
        sleep(1);
    }
    return 0;
}

void *timer_loop(void *args)
{
    struct timeval tv;
    json_t *j_array, *record, *end;

    while (1) {
        gettimeofday(&tv, NULL);
        j_array = json_load_file(TIME_LOG_FILE, JSON_DECODE_ANY, NULL);
        if (!j_array)
            j_array = json_array();
        record = json_pack("{sisi}", "start", tv.tv_sec, "end", tv.tv_sec);
        end = json_object_get(record, "end");
        json_array_append_new(j_array, record);

        update(j_array, end);
        json_decref(j_array);
    }

    return NULL;
}

void change_timer(int state)
{
    if (state == TIMER_START) {
        int lock = mutex.trylock();
        if (lock == 0) {
            timer_state = state;
            condition.signal();
            mutex.unlock();
        } else {
            printf("%s(%d): %s\n", __func__, __LINE__, strerror(lock));
        }
    } else {
        timer_state = state;
        int unlock = mutex.unlock();
        if (unlock != 0)
            printf("%s(%d): %s\n", __func__, __LINE__, strerror(unlock));
    }
}

void switch_timer()
{
    if (timer_state == TIMER_PAUSE) {
        change_timer(TIMER_START);
    } else {
        printf("Pause timer\n");
        change_timer(TIMER_PAUSE);
    }
}

void *key_command(void *args)
{
    int ch = 0;
    pthread_t timer = *(pthread_t*)args;

    while (1) {
        ch = getchar();
        switch (ch) {
            case 10:                /* enter */
                switch_timer();
                break;
            case 'q':
            case 'Q':
                printf("Terminate timer\n");
                pthread_cancel(timer);
                return NULL;
            default:
                break;
        }
    }

    return NULL;
}

int main()
{
    struct termios oldt, newt;
    pthread_t timer, cmd;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);          
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    mutex.lock();

    pthread_create(&timer, NULL, timer_loop, NULL);
    pthread_create(&cmd, NULL, key_command, &timer);

    pthread_join(timer, NULL);
    pthread_join(cmd, NULL);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return 0;
}
