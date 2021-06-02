
#include <omp.h>
  
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <chrono>
#include <sys/time.h>

#define NUM_THREADS 40

struct linear_system_of_equations {
    double **coefficients;
    double *free_terms;
    int unknowns_no;
};

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


void solution2(linear_system_of_equations lse){
    
    double * unknowns = new double[lse.unknowns_no];
    double * sum = new double[lse.unknowns_no];
    
    int solution_index = 0;
    int thread_index = 0;
    int solved_index = 0;

    int start_index = 0;

    /* Initialize the sum array: */
    for(solved_index = 0; solved_index < lse.unknowns_no; solved_index ++)sum[solved_index] = lse.free_terms[solved_index];

    /* Solve the system: */
    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
    for(solved_index = lse.unknowns_no - 1; solved_index > -1; solved_index --){
        #pragma omp parallel default(none) private(thread_index, solution_index, start_index) shared(solved_index, sum, unknowns, lse) num_threads(NUM_THREADS)
        {
            thread_index = omp_get_thread_num();
            if(solved_index % NUM_THREADS == thread_index){
                if(lse.coefficients[solved_index][solved_index] != 0)
                    unknowns[solved_index] = sum[solved_index] / lse.coefficients[solved_index][solved_index];
                else unknowns[solved_index] = 0;
                // printf("thread_index= %d: solved index = %d\n", thread_index, solved_index);
            }

            #pragma omp barrier

            start_index = solved_index - 1;
            while(start_index > -1 && start_index % NUM_THREADS != thread_index)start_index --;
            
            for(solution_index = start_index ; solution_index > -1; solution_index = solution_index - NUM_THREADS){
                if(solution_index % NUM_THREADS == thread_index)
                    sum[solution_index] -= lse.coefficients[solution_index][solved_index] * unknowns[solved_index];
            }

            #pragma omp barrier
        }
    }

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto execution_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    auto execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto execution_time_s = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    auto execution_time_min = std::chrono::duration_cast<std::chrono::minutes>(end - begin).count();

    printf("execution_time_ns = %d\n", execution_time_ns);
    printf("execution_time_ms = %d\n", execution_time_ms);
    printf("execution_time_s = %d\n", execution_time_s);
    printf("execution_time_min = %d\n", execution_time_min);

}

int main()
{
  
    std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();

    char * matrix_coeff_filename = "a_input_10000.txt";
    char * free_terms_filename = "free_terms_10000.txt";
    char * unknown_num_filename = "unknown_no_10000.txt";

    printf("read the linear system: %s, %s, %s\n", unknown_num_filename, matrix_coeff_filename, free_terms_filename);
    linear_system_of_equations lse = read_linear_system(matrix_coeff_filename, free_terms_filename, unknown_num_filename);
    
    printf("n = %d\n", lse.unknowns_no);

    solution2(lse);

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

    auto total_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
    auto total_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    auto total_time_s = std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
    auto total_time_min = std::chrono::duration_cast<std::chrono::minutes>(end - begin).count();

    printf("total_time_ns = %d\n", total_time_ns);
    printf("total_time_ms = %d\n", total_time_ms);
    printf("total_time_s = %d\n", total_time_s);
    printf("total_time_min = %d\n", total_time_min);

    return 0;

}