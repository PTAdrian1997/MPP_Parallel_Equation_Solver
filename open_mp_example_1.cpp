#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#define NUM_THREADS 4

int main(int argc, char * argv[]){

    int * values = new int[10];
    int thread_index = 0;
    int solution_index = 0;
    int solved_index = 0;
    int start_index = 0;

    for(solved_index = 10 - 1; solved_index > -1; solved_index --){
        #pragma omp parallel default(none) private(thread_index, solution_index, start_index) shared(values, solved_index) num_threads(NUM_THREADS)
        {
            thread_index = omp_get_thread_num();
            if(solved_index % NUM_THREADS == thread_index){
                values[solved_index] = 1;
                // printf("thread_index=%d: value %d has been set to %d\n", thread_index, solved_index, 1);
            }

            start_index = solved_index - 1;
            while(start_index > -1 && start_index % NUM_THREADS != thread_index){
                printf("thread index = %d, start_index = %d\n", thread_index, start_index);
                start_index --;
            }
            // for(solution_index = start_index ; solution_index > -1; solution_index = solution_index - NUM_THREADS){
            //     if(solution_index % NUM_THREADS == thread_index) printf("solution %d\n", solution_index);
            // }

            #pragma omp barrier
        }
    }

    return 0;
}