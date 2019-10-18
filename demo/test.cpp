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
<<<<<<< HEAD
        bool ok = lim_p->wait(token_bucket::Time(20, token_bucket::Time::TIME_UNIT_S));
=======
        bool ok = lim_p->wait(1.0e9);
>>>>>>> dce70dfc40768f211d53cb3991cafd8ae44d2584
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
<<<<<<< HEAD
    //error when rate is too small
    lim_p = new token_bucket::Limiter(3.0e-19, 5.0);
=======
    lim_p = new token_bucket::Limiter(3.0e-6, 5.0);
>>>>>>> dce70dfc40768f211d53cb3991cafd8ae44d2584
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
