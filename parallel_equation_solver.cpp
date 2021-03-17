#include <thread>
#include "parallel_equation_solver.h"
#include <stdio.h>
#include <string.h>
#include <vector>

void manager_thread(int thread_index, linear_system_of_equations lse, double * solution, int number_of_threads, int * needed_counter){

    int number_sums = lse.unknowns_no / number_of_threads;
    if(thread_index < lse.unknowns_no % number_of_threads) number_sums += 1;

    bool * has_been_checked = new bool[lse.unknowns_no];
    memset(has_been_checked, false, lse.unknowns_no);

    int managed_sums_count = 0;
    do {
        for(int sum_index = lse.unknowns_no - 1; sum_index > -1; sum_index--){
            if(!has_been_checked[sum_index] && needed_counter[sum_index] + sum_index == lse.unknowns_no - 1){
                for(int local_sum_index = thread_index; local_sum_index < lse.unknowns_no; local_sum_index += number_of_threads){
                    if(sum_index > local_sum_index){
                        solution[local_sum_index] -= solution[sum_index] * lse.coefficients[local_sum_index][sum_index] / lse.coefficients[local_sum_index][local_sum_index];
                        needed_counter[local_sum_index] += 1;
                        if(needed_counter[local_sum_index] + local_sum_index == lse.unknowns_no - 1) managed_sums_count += 1;
                    }
                }
                has_been_checked[sum_index] = true;
            }
        }
    } while(managed_sums_count != number_sums);
}

double * parallel_system_solver(linear_system_of_equations lse, int number_of_threads){
    double * solution = new double[lse.unknowns_no];
    bool * is_finished = new bool[lse.unknowns_no];
    int * needed_counter = new int[lse.unknowns_no];
    for(int i = 0; i < lse.unknowns_no - 1; i++){
        is_finished[i] = false;
    }
    solution[lse.unknowns_no - 1] = lse.free_terms[lse.unknowns_no - 1] / lse.coefficients[lse.unknowns_no - 1][lse.unknowns_no - 1];
    is_finished[lse.unknowns_no - 1] = true;
    needed_counter[lse.unknowns_no - 1] = 0;
    std::vector<std::thread> threads; 
    for(int i = lse.unknowns_no - 1; i > -1; i--){
        threads.push_back(std::thread(manager_thread, std::ref(solution), number_of_threads, std::ref(needed_counter)));
        threads.back().join();
    }

    return solution;
}