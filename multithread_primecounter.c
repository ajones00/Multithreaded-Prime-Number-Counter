#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <stdatomic.h>

#define TRILLION 1000000000
#define MAX_THREADS 16

atomic_int num_prime = 0;

struct pthread_arg {
    int low_num;
    int high_num;
};

// decide if n is a prime number by trying all possible factors not greater than sqrt(n), assumption n >= 2
int is_prime(int n)
{
int i;

    for (i = 2; i <= sqrt(n); i++)
        if ( n%i == 0 )
            return(0);
    return(1);
}


// count the number of prime numbers in the range [low, high]
//  @return - the number of prime numbers found in this range
int prime_cnt(int low, int high)
{
    int i;
    int nums_found_in_range = 0;

    for ( i = low; i <= high; i++){
        if (is_prime(i)){
            num_prime++;
            nums_found_in_range++;
        }
    }

    return nums_found_in_range;
}

//for use with linux pthread_create
void* prime_cnt_forpthread(void* rawArgsPtr) {
    struct pthread_arg* argsPtr = rawArgsPtr;
    int low = argsPtr->low_num;
    int high = argsPtr->high_num;
    int primes_in_range;

    primes_in_range = prime_cnt(low, high);
    printf("primes in range [%d, %d]: %d\n", low, high, primes_in_range);


    return NULL;
}


pthread_t pthread[MAX_THREADS];

int main(int argc, char** argv)
{
    int low, high, i, thread_creation_status;
    int num_thread;
    int total_num = 0;

    struct timespec start_time, end_time;
    struct pthread_arg parg[MAX_THREADS];


    num_thread = atoi(argv[3]);
    total_num = 0;

    if (argc < 3){
        printf("Usage: %s low high num_thread\n", argv[0]);
        return(1);
    }

// take in low and high
    low = atoi(argv[1]);
    high = atoi(argv[2]);

// simple sanity check
    if ( low > high ){
        i = low;
        low = high;
        high = i;
    }

    if ( low < 2 ){
        low = 2;
    }

//the number of integers between low to high, inclusive
    total_num = ((high + 1) - low);

    //divide those amount of numbers evenly amongst the defined number of threads, this will give a certain amount of numbers to each thread
    int nums_per = total_num/num_thread;


// record start time
    if ( clock_gettime(CLOCK_MONOTONIC, &start_time) ){
        perror("get start_time");
    }

    // whether or not the number was evenly divisible over threads,
    //  otherwise we need to add an additional number per thread
    int uneven_range = total_num % num_thread;

    for (i = 0; i < num_thread; i++) {
        parg[i].low_num = low;
        parg[i].high_num = low + nums_per -1;

        if (uneven_range) {
            parg[i].high_num++;
        }

        // NOT DIRECTLY RELATED TO if (uneven_range)
        //  this might also occurr if uneven_range is true
        //  and previous high_num modifications added up
        //  over several pivot points
        if (parg[i].high_num > high) {
            parg[i].high_num = high;
        }

        printf("From thread %u: low == %d high == %d\n", i, parg[i].low_num, parg[i].high_num);
        thread_creation_status = pthread_create(&pthread[i], NULL, prime_cnt_forpthread, (void *)&parg[i]);
        
        if (thread_creation_status != 0) {
            perror("pthread_create");
            return(1);
        }
        
        low = parg[i].high_num + 1;

        
    }

// wait for all threads to finish
    for (int i = 0; i < num_thread; i++) {
        // NULL because don't need the returned result here
        pthread_join(pthread[i], NULL);
    }
    
    printf("\nhigh == %d\n", high);
    low = atoi(argv[1]);
    high = atoi(argv[2]);


    printf("%u prime numbers between %d and %d\n", num_prime, low, high);


// record end time
    if ( clock_gettime(CLOCK_MONOTONIC, &end_time) ){
        perror("get end_time");
    }

    printf(" Counting took %f seconds \n", ((end_time.tv_sec - start_time.tv_sec) * TRILLION + (end_time.tv_nsec - start_time.tv_nsec))*1.0/TRILLION);


    return(0);
}
