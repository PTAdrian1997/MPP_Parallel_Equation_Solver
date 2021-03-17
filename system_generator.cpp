#include <stdlib.h>
#include <time.h>
#include "system_generator.h"
#include <math.h>
#include <fstream>

/**
 * @brief Generate a random system of equations of n equations in n unknowns.
 * 
 * @param n The number of unknowns and equations in the system
 * @return linear_system_of_equations The random linear system of equations generated
 */
linear_system_of_equations generate_system(int n){

    srand(time(NULL));

    linear_system_of_equations result;

    result.coefficients = new double*[n];
    for(int row = 0; row < n; row++){

        result.coefficients[row] = new double[n];

        for(int col = 0; col < n; col++){
            if(col >= row)result.coefficients[row][col] = rand() * 0.1;
            else result.coefficients[row][col] = 0.0;
        }
    }

    result.free_terms = new double[n];
    for(int row = 0; row < n; row++)result.free_terms[row] = rand() * 0.1;

    result.unknowns_no = n;

    return result;
}

/**
 * @brief Write the coefficient matrix
 * 
 * @param coeff a 2-dimensional array, representing the coefficient matrix of a linear system
 * @param coeff_filename the name of the file to be generated
 * @param number_of_unknowns the number of unknowns in the equations
 */
void write_coefficient_matrix(double ** coeff, char * coeff_filename, int number_of_unknowns){

    std::ofstream coeff_file;
    coeff_file.open(coeff_filename);
    // write the number of unknowns on the first line:
    coeff_file << number_of_unknowns << "\n";
    // on each row, write the coeffiecients of the corresponding equation:
    for(int row_id = 0; row_id < number_of_unknowns; row_id++){
        for(int col_id = 0; col_id < number_of_unknowns; col_id++) coeff_file << coeff[row_id][col_id] << " ";
        coeff_file << "\n";
    }
    coeff_file.close();
}

/**
 * @brief 
 * 
 * @param free_terms 
 * @param free_terms_filename 
 * @param number_of_unknowns 
 */
void write_free_terms_array(double * free_terms, char * free_terms_filename, int number_of_unknowns){
    std::ofstream free_terms_file;
    free_terms_file.open(free_terms_filename);
    /*write the number of equations on the 1st line:*/
    free_terms_file << number_of_unknowns << "\n";
    /*write each free term on the next line, separated by space:*/
    for(int element_id = 0; element_id < number_of_unknowns; element_id ++)free_terms_file << free_terms[element_id] << " ";
    free_terms_file.close();
}

/**
 * @brief 
 * 
 * @param lse 
 * @param coeff_filename 
 * @param free_terms_filename 
 */
void write_system_of_equations(linear_system_of_equations lse, char * coeff_filename, char * free_terms_filename){
    /*first, write the file that contains the coefficients matrix:*/
    write_coefficient_matrix(lse.coefficients, coeff_filename, lse.unknowns_no);
    write_free_terms_array(lse.free_terms, free_terms_filename, lse.unknowns_no);
}