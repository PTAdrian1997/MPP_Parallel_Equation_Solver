#include <iostream>
#include <mpi.h>
#include <stdint.h>
#include <fstream>
#include <stdlib.h>

#include <algorithm>

#include <chrono>

#define NUM_THREADS 5
#define READ_CHUNK_SIZE 10

using namespace std;

struct linear_system_of_equations {
    double **coefficients;
    double *free_terms;
    int unknowns_no;
};

const int NEW_VALUE_FOR_SOLUTION_TAG = 0;
const int NUMBER_OF_UNKNOWNS_TAG = 1;

/**
 * @brief Read the coefficient matrix from the provided file
 * 
 * @param coeff_filename The name of the file that stores the coefficient matrix
 * @param unknowns_no The number of unknowns in the equation
 * @return int** the coefficient matrix
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
 * @brief Read the array of free terms from the provided file
 * 
 * @param free_term_filename The filename of the file that stores the free terms
 * @param unknowns_no the number of unknowns in the equation system
 * @return double* The array of free terms
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
 * @brief Read the number of unknowns from the provided file
 * 
 * @param filename The name of the file that stores the free terms of the equation system
 * @return int The number of unknowns from the equation system
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
linear_system_of_equations read_linear_system(char * coeff_filename, char * free_terms_filename, char * unknown_no_filename, int world_rank, int world_size){

    linear_system_of_equations result;

    /* The thread with rank 0 will read the linear system of equations and will share the result with the other threads: */
    if(world_rank == 0){
        result.unknowns_no = read_unknown_no(unknown_no_filename);
    }
    MPI_Bcast(&result.unknowns_no, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if(world_rank == 0){
        result.coefficients = read_coeff_matrix(coeff_filename, result.unknowns_no);
        result.free_terms = read_free_terms(free_terms_filename, result.unknowns_no);
    }
    else{
        result.coefficients = new double*[result.unknowns_no];
        result.free_terms = new double[result.unknowns_no];
    }
    MPI_Bcast(result.free_terms, result.unknowns_no, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    for(int row_index = 0; row_index < result.unknowns_no; row_index ++){
        if(world_rank != 0){
            result.coefficients[row_index] = new double[result.unknowns_no];
        }
        MPI_Bcast(result.coefficients[row_index], result.unknowns_no, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    return result;
}


void solution1(linear_system_of_equations lse, double absolute_begin){

    double begin = MPI_Wtime();

    int solved_index = 0;
    int sum_index = 0;
    int other_rank = 0;
    int expected_sender = 0;
    int start_index = 0;

    /* Copy the function input to some thread-local variable: */
    linear_system_of_equations tl_lse;
    tl_lse.unknowns_no = lse.unknowns_no;
    tl_lse.coefficients =  new double*[lse.unknowns_no];
    tl_lse.free_terms = new double[lse.unknowns_no];
    for(int row_index = 0; row_index < lse.unknowns_no; row_index++){
        tl_lse.coefficients[row_index] = new double[lse.unknowns_no];
        for(int col_index = 0; col_index < lse.unknowns_no; col_index ++){
            tl_lse.coefficients[row_index][col_index] = lse.coefficients[row_index][col_index];
        }
        tl_lse.free_terms[row_index] = lse.free_terms[row_index];
    }

    double * solution = new double[lse.unknowns_no];
    double * sum = new double[lse.unknowns_no];

    /* The structure that will hold the information required to update the local solution
     * arrays;
     */
    struct SolutionEnvelope{
        int solution_index;
        double solution_value;
    };
    MPI_Datatype MPI_SolutionEnvelope;
    MPI_Datatype custom_type[] = {MPI_INT, MPI_DOUBLE};
    int blocklen[] = {1, 1};
    MPI_Aint disp[] = {offsetof(SolutionEnvelope, solution_index), offsetof(SolutionEnvelope, solution_value)};
    MPI_Type_create_struct(2, blocklen, disp, custom_type, &MPI_SolutionEnvelope);
    MPI_Type_commit(&MPI_SolutionEnvelope);

    // Find out rank, size
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    struct SolutionEnvelope solution_envelope;

    /* Initialize the sum and solution array: */
    for(sum_index = 0; sum_index < tl_lse.unknowns_no; sum_index ++){
        sum[sum_index] = tl_lse.free_terms[sum_index];
        solution[sum_index] = -11111111111111.0000001;
    }

    for(solved_index = tl_lse.unknowns_no - 1; solved_index > -1; solved_index --){

        if(solved_index % world_size == world_rank){
            if(tl_lse.coefficients[solved_index][solved_index] == 0)solution[solved_index] = 0.0;
            else solution[solved_index] = sum[solved_index] / lse.coefficients[solved_index][solved_index];
            /* Let the other threads know about the new result: */
            solution_envelope.solution_index = solved_index;
            solution_envelope.solution_value = solution[solved_index];
            // for(other_rank = 0; other_rank < world_size; other_rank++)
            //     if(other_rank != world_rank)MPI_Send(&solution_envelope, 1, MPI_SolutionEnvelope, other_rank, NEW_VALUE_FOR_SOLUTION_TAG, MPI_COMM_WORLD);
        }
        MPI_Bcast(&solution_envelope, 1, MPI_SolutionEnvelope, solved_index % world_size, MPI_COMM_WORLD);
        if(world_rank != solved_index % world_size){
            solution[solution_envelope.solution_index] = solution_envelope.solution_value;
        }
        // else{
            // expected_sender = solved_index % world_size;
            // MPI_Recv(&solution_envelope, 1, MPI_SolutionEnvelope, expected_sender, NEW_VALUE_FOR_SOLUTION_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // solution[solution_envelope.solution_index] = solution_envelope.solution_value;
        // }

        start_index = solved_index - 1;
        while(start_index > -1 && start_index % world_size != world_rank)start_index --;
        for(int future_solution_index = start_index; future_solution_index > -1; future_solution_index -= world_size){
            sum[future_solution_index] -= solution[solved_index] * tl_lse.coefficients[future_solution_index][solved_index];
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    // if(world_rank == 0)for(int i = 0; i < lse.unknowns_no; i++)printf("%f\n", solution[i]);

    double end = MPI_Wtime();

    if(world_rank == 0){
        printf("execution_elapsed_time: %f\n", end - begin);
        printf("total elapsed time: %f\n", end - absolute_begin);
    }

}

int main(int argc,char* argv[]){
    
    MPI_Init(NULL, NULL);

    double absolute_begin = MPI_Wtime();

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    char * matrix_coeff_filename = "a_input_1000.txt";
    char * free_terms_filename = "free_terms_1000.txt";
    char * unknown_num_filename = "unknown_no_1000.txt";

    linear_system_of_equations lse = read_linear_system(matrix_coeff_filename, free_terms_filename, unknown_num_filename, world_rank, world_size);

    solution1(lse, absolute_begin);

    MPI_Finalize();

    return 0;
}