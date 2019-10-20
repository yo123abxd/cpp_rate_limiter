/*
 * These codes below explain how to generate 3.0 tokens per second 
 * and cosume 1.0 tokens every time execute your codes.
 */
#include "../include/Limiter.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <time.h>

using namespace std;

token_bucket::Limiter* lim_p;

void foo (int th_num) {
    for(int loop_time = 0; loop_time < 5; ) {
        //waitN(cosume tokens num, max wait time/s )
        bool ok = lim_p->wait(token_bucket::Time(20, token_bucket::Time::TIME_UNIT_S));
        if (ok) {
            time_t t = time(nullptr);
            /*
             * write your codes here
             */
            cout<<"thread_num = "<< th_num << ", loop_time = " << loop_time <<" time : "<< t << endl;
            ++loop_time;
        } else {
            //if exceed max wait time
            usleep(10000);
        }
    }
}

int main() {
    //first parameter->generate n tokens per s, second parameter->burst tokens
    //means generate 3.0 tokens per s
    //when rate <= 0, it will be set to MIN_RATE which equals DBL_MIN
    lim_p = new token_bucket::Limiter(3.0, 5.0);
    vector<thread*> vec(5, nullptr);
    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num] = new thread(foo, th_num);
    }

    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num]->join();
        delete vec[th_num];
    }
    delete lim_p;
    return 0;
}
