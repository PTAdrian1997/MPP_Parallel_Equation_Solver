#include "system_reader.h"
#include <fstream>

/**
 * @brief 
 * 
 * @param coeff_filename 
 * @return int** 
 */
double** read_coeff_matrix(char * coeff_filename){
    std::ifstream coeff_file(coeff_filename);
    // read the number of unknowns:
    int n;
    coeff_file >> n;
    double ** result = new double*[n];
    for(int row_id = 0; row_id < n; row_id ++){
        // read the equations:
        result[row_id] = new double[n];
        for(int i = 0;i < row_id;i++)result[row_id][i] = 0.0;
        for(int col_id = row_id; col_id < n; col_id++){
            coeff_file >> result[row_id][col_id];
            result[row_id][col_id] = result[row_id][col_id]  * 0.01;
            if(result[row_id][col_id] == 0)result[row_id][col_id] = 1.0;
        }
    }
    coeff_file.close();
    return result;
}

double * read_free_terms(char * free_term_filename){
    std::ifstream free_tearm_file(free_term_filename);
    // read the number of equations:
    int n = 0;
    free_tearm_file >> n;
    // read the values:
    double * result = new double[n];
    for(int eq_id = 0; eq_id < n; eq_id ++){
        free_tearm_file >> result[eq_id];
        if(result[eq_id] == 0)result[eq_id] = 1000.0;
    }
    free_tearm_file.close();
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
linear_system_of_equations read_linear_system(char * coeff_filename, char * free_terms_filename, int no_unknowns){
    linear_system_of_equations result;
    result.coefficients = read_coeff_matrix(coeff_filename);
    result.free_terms = read_free_terms(free_terms_filename);
    result.unknowns_no = no_unknowns;
    return result;
}