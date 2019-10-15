#include "../include/Limiter.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <time.h>

using namespace std;

token_bucket::Limiter* lim_p;

void foo (int i) {
    for(int j = 0; j < 5; j++) {
        //wait(max wait time us )
        bool ok = lim_p->wait(1.0e9);
        if (!ok) {
            //if exceed max wait time
        }
        time_t t = time(nullptr);
        cout<<"thread_num = "<< i << ", j = " << j <<" time : "<< t <<endl;
    }
}

int main() {
    //first parameter->generate n tokens per us, second parameter->burst tokens
    //means generate 3.0e-6 tokens per us
    lim_p = new token_bucket::Limiter(3.0e-6, 5.0);
    vector<thread*> vec(5, nullptr);
    for (int i = 0; i < static_cast<int>(vec.size()); i++) {
        vec[i] = new thread(foo, i);
    }

    for (int i = 0; i < static_cast<int>(vec.size()); i++) {
        vec[i]->join();
        delete vec[i];
    }
    delete lim_p;
    return 0;
}
