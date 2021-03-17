#include <iostream>
#define MAX_SIZE_N 10000
#include <thread>

using namespace std;

void thread_test_function(int thread_index, bool * location){
    location[thread_index] = true;
    int true_count = 0;
    do{
        true_count = 0;
        for(int i = 0; i < 2; i++){
            if(location[i])true_count += 1;
        }
    }while(true_count != 2);
    cout << "thread index: " << thread_index << ", partial problem solved\n";
}

int main(){

    bool * location = new bool[2];
    thread threads[2];
    for(int i = 0; i < 2; i++){
        location[i] = false;
        threads[i] = thread(thread_test_function, i, location);
        threads[i].join();
    }

    return 0;
}