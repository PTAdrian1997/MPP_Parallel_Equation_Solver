#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <iostream>

using namespace std;

int main(){

    int ar1[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int ar2[10];

    copy(begin(ar1), end(ar1), begin(ar2));

    for(auto e : ar2)
        cout << e << ' ';
    cout << '\n';

    return 0;

}