#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <mpi.h>
#include <chrono>

#define NUM_OMP_THREADS 4

using namespace std;

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


void solution1(){
    linear_system_of_equations lse;
    char * matrix_coeff_filename = "a_input_10.txt";
    char * free_terms_filename = "free_terms_10.txt";
    char * unknown_num_filename = "unknown_no_10.txt";
    MPI_Init(NULL, NULL);
    double absolute_begin = MPI_Wtime();

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* The thread with rank 0 will read the linear system of equations and will share the result with the other threads: */
    if(world_rank == 0){
        lse.unknowns_no = read_unknown_no(unknown_num_filename);
    }
    MPI_Bcast(&lse.unknowns_no, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(world_rank == 0){
        lse.coefficients = read_coeff_matrix(matrix_coeff_filename, lse.unknowns_no);
        lse.free_terms = read_free_terms(free_terms_filename, lse.unknowns_no);
    }
    else{
        lse.coefficients = new double*[lse.unknowns_no];
        lse.free_terms = new double[lse.unknowns_no];
    }
    MPI_Bcast(lse.free_terms, lse.unknowns_no, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    for(int row_index = 0; row_index < lse.unknowns_no; row_index ++){
        if(world_rank != 0){
            lse.coefficients[row_index] = new double[lse.unknowns_no];
        }
        MPI_Bcast(lse.coefficients[row_index], lse.unknowns_no, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    int solved_index = 0;
    int solution_index = 0;
    int thread_index = 0;
    int start_index = 0;

    double * solution = new double[lse.unknowns_no];
    double * sum = new double[lse.unknowns_no];

    /* Solve the system: */
    for(solved_index = lse.unknowns_no - 1; solved_index > -1; solved_index --){
        #pragma omp parallel default(none) private(thread_index, solution_index, start_index) shared(solved_index, lse, solution, sum) num_threads(NUM_OMP_THREADS)
        {
            thread_index = omp_get_thread_num();
            if(world_rank == 0){

            }
            MPI_Barrier();
        }
    }

    MPI_Finalize();
}

int main(){

    solution1();

    return 0;
}