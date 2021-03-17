#include "linear_system_schema.h"

#ifndef SYSTEM_READER_H
#define SYSTEM_READER_H

linear_system_of_equations read_linear_system(char * coeff_filename, char * free_terms_filename, int no_unknowns);

#endif