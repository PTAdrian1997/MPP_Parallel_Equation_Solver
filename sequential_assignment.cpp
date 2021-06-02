#include <iostream>
#define MAX_SIZE_N 10000

#include <cmath>

#include <stdlib.h>
#include <stdio.h>
#include <fstream>

#include <chrono>

struct linear_system_of_equations {
    double **coefficients;
    double *free_terms;
    int unknowns_no;
};

using namespace std;

char * coefficient_filename = "a_input.txt";
char * free_temrs_filename = "b_input.txt";

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

/**
 * @brief 
 * 
 * @param coeff_filename 
 * @return int** 
 */
double** read_coeff_matrix(char * coeff_filename, int unknowns_no){
    std::ifstream coeff_file(coeff_filename);
    // read the number of unknowns:
    double ** result = new double*[unknowns_no];
    for(int row_id = 0; row_id < unknowns_no; row_id ++){
        // read the equations:
        result[row_id] = new double[unknowns_no];
        for(int i = 0;i < row_id;i++)result[row_id][i] = 0.0;
        for(int col_id = row_id; col_id < unknowns_no; col_id++){
            coeff_file >> result[row_id][col_id];
            result[row_id][col_id] = result[row_id][col_id]  * 0.01;
            if(result[row_id][col_id] == 0)result[row_id][col_id] = 1.0;
        }
    }
    coeff_file.close();
    return result;
}

/**
 * @brief 
 * 
 * @param free_term_filename 
 * @param unknowns_no 
 * @return double* 
 */
double * read_free_terms(char * free_term_filename, int unknowns_no){
    std::ifstream free_tearm_file(free_term_filename);
    // read the values:
    double * result = new double[unknowns_no];
    for(int eq_id = 0; eq_id < unknowns_no; eq_id ++){
        free_tearm_file >> result[eq_id];
        if(result[eq_id] == 0)result[eq_id] = 1000.0;
    }
    free_tearm_file.close();
    return result;
}

/**
 * @brief 
 * 
 * @param filename 
 * @return int 
 */
int read_unknown_no(char * filename){
    int result = 0;
    std::ifstream unknowns_no_file(filename);
    unknowns_no_file >> result;
    unknowns_no_file.close();
    return result;
}

/**
 * @brief 
 * 
 * @param coeff_filename 
 * @param free_terms_filename 
 * @param no_unknowns 
 * @return linear_system_of_equations 
 */
linear_system_of_equations read_linear_system(char * coeff_filename, char * free_terms_filename, char * unknown_no_filename){
    linear_system_of_equations result;
    result.unknowns_no = read_unknown_no(unknown_no_filename);
    result.coefficients = read_coeff_matrix(coeff_filename, result.unknowns_no);
    result.free_terms = read_free_terms(free_terms_filename, result.unknowns_no);
    return result;
}


void solve_system(){
    
    char * matrix_coeff_filename = "a_input.txt";
    char * free_terms_filename = "free_terms.txt";
    char * unknown_num_filename = "unknown_no.txt";

    cout << "read the system: \n";
    linear_system_of_equations lse = read_linear_system(coefficient_filename, free_temrs_filename, unknown_num_filename);
    cout << "solve the system: \n";

    // double * solution = parallel_system_solver(lse, number_of_threads);
    double * solution = sequential_system_solver(lse, "debug.txt");
}

int main(){

    
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    solve_system();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "\n";

    return 0;
}
