#include "pure_sequential_system_solver.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

/**
 * @brief 
 * 
 * @param system_of_equations 
 * @return int* 
 */
double * sequential_system_solver(linear_system_of_equations system_of_equations, char * perf_filename){

    int n = system_of_equations.unknowns_no;
    double * result = new double[n];

    std::ofstream perf_file(perf_filename, std::ios_base::app);

    perf_file << "sequential_system_solver \n====================\n";

    for(int sol_id = n - 1; sol_id > -1; sol_id--){
        result[sol_id] = system_of_equations.free_terms[sol_id] * 1.0 / system_of_equations.coefficients[sol_id][sol_id];
        for(int j = n - 1; j > sol_id; j--){
            perf_file << "result[" << sol_id << "]=" << result[sol_id] << ", j = " << j << "\n";
            result[sol_id] -= system_of_equations.coefficients[sol_id][j] * result[j] * 1.0 / system_of_equations.coefficients[sol_id][sol_id];
        }
    }
    perf_file.close();

    return result;
}