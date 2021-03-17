
#ifndef LINEAR_SYSTEM_ALGEBRA_H
#define LINEAR_SYSTEM_ALGEBRA_H

struct linear_system_of_equations {
    double **coefficients;
    double *free_terms;
    int unknowns_no;
};

#endif