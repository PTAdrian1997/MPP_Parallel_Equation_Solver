#include <iostream>
#define MAX_SIZE_N 10000
#include "system_reader.h"
#include "pure_sequential_system_solver.h"
#include "system_generator.h"
#include "parallel_equation_solver.h"

#include <cmath>

using namespace std;

char * coefficient_filename = "a_input.txt";
char * free_temrs_filename = "b_input.txt";
char * debug_filename = "debug.txt";

bool check_result(double * solution, linear_system_of_equations lse){
    double err = 0.001;
    bool result = true;
    for(int eq_id = 0; eq_id < lse.unknowns_no && result; eq_id++){
        double s = 0;
        for(int col_id = eq_id; col_id < lse.unknowns_no; col_id++) s += solution[col_id] * lse.coefficients[eq_id][col_id];
        cout << "s = " << s << ", lse.free_terms[" << eq_id << "] = " << lse.free_terms[eq_id] << "\n";
        if(abs(s - lse.free_terms[eq_id]) > err){
            result = false;
        }
    }
    return result;
}

void generate_system_main(){
    cout << "generate the linear system: \n";
    int n = MAX_SIZE_N;
    write_system_of_equations(generate_system(n), coefficient_filename, free_temrs_filename);
}

void solve_system(){
    
    cout << "read the system: \n";
    linear_system_of_equations lse = read_linear_system(coefficient_filename, free_temrs_filename, MAX_SIZE_N);
    cout << "solve the system: \n";
    int number_of_threads = 4;
    double * solution = parallel_system_solver(lse, number_of_threads);
    cout << "check solution: \n";
    cout << "solution is correct: " << check_result(solution, lse) << "\n";
}

int main(){
    
    solve_system();

    return 0;
}
