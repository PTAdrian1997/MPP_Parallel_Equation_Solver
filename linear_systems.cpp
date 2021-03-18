#include <iostream>
#define MAX_SIZE_N 10000
#include "system_reader.h"
#include "pure_sequential_system_solver.h"
#include "system_generator.h"
#include "parallel_equation_solver.h"
#include <chrono>

#include <cmath>

using namespace std;

char * coefficient_filename = "a_input.txt";
char * free_temrs_filename = "b_input.txt";
char * debug_filename = "debug.txt";



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
    // double * solution = parallel_system_solver(lse, number_of_threads);
    double * solution = sequential_system_solver(lse, "debug.txt");
}

int main(){

    typedef std::chrono::high_resolution_clock Time;
    typedef std::chrono::milliseconds ms;
    typedef std::chrono::duration<float> fsec;
    
    auto start = Time::now();
    solve_system();
    auto end = Time::now();

    fsec perf_time = end - start;
    cout << perf_time.count() << "\n";

    return 0;
}
