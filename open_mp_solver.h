#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "linear_system_schema.h"

double * open_mp_parallel_solver(linear_system_of_equations lse, int number_of_threads);