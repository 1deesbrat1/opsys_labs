#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define ARRAY_SIZE 500000
#define MAX_THREAD_LIMIT 64
#define PARALLEL_THRESHOLD 4096

static int values[ARRAY_SIZE];

static pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;

static int active_threads = 1;
static int thread_limit = 1;

struct sort_task {
    int *array;
    int low;
    int high;
};

static void quick_sort_parallel(int array[], int low, int high);
static int reserve_thread_slot(void);
static void release_thread_slot(void);

static int partition(int array[], int low, int high) {
    int pivot = array[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (array[j] <= pivot) {
            i++;


            int temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }

    int temp = array[i + 1];
    array[i + 1] = array[high];
    array[high] = temp;

    return i + 1;
}


void quick_sort(int array[], int low, int high) {
    if (low < high) {
        int pivot_index = partition(array, low, high);

        quick_sort(array, low, pivot_index - 1);
        quick_sort(array, pivot_index + 1, high);
    }
}

static void* sort_worker(void* arg) {
    struct sort_task *task = arg;

    quick_sort_parallel(task->array, task->low, task->high);

    return 0;
}

static void quick_sort_parallel(int array[], int low, int high) {
    if (low >= high) {
        return;
    }

    int pivot_index = partition(array, low, high);

    if (pivot_index - low >= PARALLEL_THRESHOLD && reserve_thread_slot()) {

        struct sort_task left_task = {
            array, low, pivot_index - 1
        };

        pthread_t worker_thread;

        int error = pthread_create(
            &worker_thread, 0, sort_worker, &left_task
        );

        if (error == 0) {
            quick_sort_parallel(array, pivot_index + 1, high);

            error = pthread_join(worker_thread, 0);

            if (error != 0) {
                const char message[] = "pthread_join failed\n";
                (void)write(
                    STDERR_FILENO, message, sizeof(message) - 1
                );
                _exit(1);
            }

            release_thread_slot();
            return;
        }

        release_thread_slot();
    }

    quick_sort_parallel(array, low, pivot_index - 1);
    quick_sort_parallel(array, pivot_index + 1, high);

}


static int reserve_thread_slot(void) {

    int reserved = 0;

    if (pthread_mutex_lock(&thread_count_mutex) != 0) {
        _exit(1);
    }

    if (active_threads < thread_limit) {
        active_threads++;
        reserved = 1;
    }

    if (pthread_mutex_unlock(&thread_count_mutex) != 0) {
        _exit(1);
    }


    return reserved;
}

static void release_thread_slot(void) {
    int error = pthread_mutex_lock(&thread_count_mutex);

    if (error != 0) {
        _exit(1);
    }

    active_threads--;

    error = pthread_mutex_unlock(&thread_count_mutex);

    if (error != 0) {
        _exit(1);
    }
}


static void fill_array(int array[], int length) {
    unsigned int state = 123456789u;

    for (int i = 0; i < length; i++) {
        state = state * 1664525u + 1013904223u;
        array[i] = (int)(state % 1000000u);
    }
}

static int is_sorted(const int array[], int length) {
    for (int i = 1; i < length; i++) {
        if (array[i - 1] > array[i]) {
            return 0;
        }
    }


    return 1;
}


static void write_unsigned(unsigned long long value) {
    char digits[20];
    int count = 0;

    do {
        digits[count++] = (char)('0' + value % 10ULL);
        value /= 10ULL;
    } while (value != 0ULL);

    while (count > 0) {
        count--;
        (void)write(STDOUT_FILENO, &digits[count], 1);
    }
    
    
}

int parse_thread_limit(const char* text, int* result) {
    if (text[0] == '\0') {
        return 0;
    }

    int value = 0;

    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }

        int digit = text[i] - '0';

        if (value > (MAX_THREAD_LIMIT - digit) / 10) {
            return 0;
        }


        value = value * 10 + digit;

    }

    if (value < 1) {
        return 0;
    }

    *result = value;
    return 1;
}

int main(int argc, char* argv[]) {
    int max_threads = 0;

    if (argc != 2 || !parse_thread_limit(argv[1], &max_threads)) {
        const char message[] = "Usase: ./lab 1...64\n";
        (void)write(STDERR_FILENO, message, sizeof(message) - 1);

        return 1;
    }

    thread_limit = max_threads;

    fill_array(values, ARRAY_SIZE);

    struct timespec start;
    struct timespec end;

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        const char message[] = "Start timer fail\n";
        (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    quick_sort(values, 0, ARRAY_SIZE - 1);

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        const char message[] = "Stop timer fail\n";
        (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    long long seconds = (long long)end.tv_sec - (long long)start.tv_sec;

    long long nanoseconds = (long long)end.tv_nsec - (long long)start.tv_nsec;

    long long elapsed_ns = seconds * 1000000000LL + nanoseconds;

    if (elapsed_ns < 0) {
        const char message[] = "not valid time\n";
        (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    const char size_message[] = "Size: ";
    (void)write(STDOUT_FILENO, size_message, sizeof(size_message) - 1);

    write_unsigned(ARRAY_SIZE);

    const char elements_message[] = " elements\n";
    (void)write(STDOUT_FILENO, elements_message, sizeof(elements_message) - 1);

    const char time_message[] = "Sequential QuickSort time: ";
    (void)write(STDOUT_FILENO, time_message, sizeof(time_message) - 1);

    write_unsigned((unsigned long long)(elapsed_ns / 1000000000LL));

    const char dot[] = ".";
    (void)write(STDOUT_FILENO, dot, sizeof(dot) - 1);

    unsigned long long fraction = (unsigned long long)((elapsed_ns % 1000000000LL) / 1000LL);

    unsigned long long divisor = 100000ULL;

    for (int i = 0; i < 6; i++) {
        char digit = (char)('0' + (fraction / divisor) % 10ULL);
        (void)write(STDOUT_FILENO, &digit, 1);
        divisor /= 10ULL;
    }

    const char seconds_message[] = " seconds\n";
    (void)write(STDOUT_FILENO, seconds_message, sizeof(seconds_message) - 1);

    if (!is_sorted(values, ARRAY_SIZE)) {
        const char message[] = "Sorting verification failed\n";
        (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    const char success_message[] = "Sorted correctly: yes\n";
    (void)write(STDOUT_FILENO, success_message, sizeof(success_message) - 1);
    


    fill_array(values, ARRAY_SIZE);

    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        const char message[] = "Timer start failed\n";
        (void)write(STDERR_FILENO, message, sizeof(message) - 1);
        return 1;
    }


    active_threads = 1;

    quick_sort_parallel(values, 0, ARRAY_SIZE - 1);

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        const char message[] = "Timer stop failed\n";
        (void)write(STDERR_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    seconds = (long long)end.tv_sec - (long long)start.tv_sec;

    nanoseconds = (long long)end.tv_nsec - (long long)start.tv_nsec;

    elapsed_ns = seconds * 1000000000LL + nanoseconds;

    if (elapsed_ns < 0) {
        const char message[] = "Invalid elapsed time\n";
        (void)write(STDERR_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    const char selected_time_message[] = "QuickSort time with thread limit ";

    (void)write(STDOUT_FILENO, selected_time_message, sizeof(selected_time_message) - 1);

    write_unsigned((unsigned long long)max_threads);

    const char separator[] = ": ";
    (void)write(STDOUT_FILENO, separator, sizeof(separator) - 1);

    write_unsigned((unsigned long long)(elapsed_ns / 1000000000LL));

    const char parallel_dot[] = ".";
    (void)write(STDOUT_FILENO, parallel_dot, sizeof(parallel_dot) - 1);

    fraction = (unsigned long long) ((elapsed_ns % 1000000000LL) / 1000LL);

    divisor = 100000ULL;

    for (int i = 0; i < 6; i++) {
        char digit = (char)('0' + (fraction / divisor) % 10ULL);
        (void)write(STDOUT_FILENO, &digit, 1);
        divisor /= 10ULL;
    }

    const char parallel_seconds_message[] = " seconds\n";
    (void)write(STDOUT_FILENO, parallel_seconds_message, sizeof(parallel_seconds_message) - 1);

    if (!is_sorted(values, ARRAY_SIZE)) {
        const char message[] = "Parallel sorting fail\n";
        (void)write(STDERR_FILENO, message, sizeof(message) - 1);
        return 1;
    }

    const char parallel_success_message[] = "Sorting verification passed: yes\n";

    (void)write(STDOUT_FILENO, parallel_success_message, sizeof(parallel_success_message) - 1);

    return 0;
}


