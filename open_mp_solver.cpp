#include "open_mp_solver.h"

double * open_mp_parallel_solver(linear_system_of_equations lse, int number_of_threads){
    
    bool is_done[lse.unknowns_no];
    for(int i = 0; i < lse.unknowns_no; i++){
        is_done[i] = false;
    }

    double solution[lse.unknowns_no];

    #pragma omp parallel for schedule(static, 1)
    {
        int tid = omp_get_thread_num();
        for(int equation_index = 0; equation_index < lse.unknowns_no; equation_index++){
            printf("Hello world %d %d\n", tid, equation_index);
        }
    }

}