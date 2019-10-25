/*
 * These codes below explain how to generate 3.0 tokens per second 
 * and cosume 2.0 tokens every time execute your codes.
 */
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "../include/Limiter.h"

using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

token_bucket::Limiter* lim_p;

void foo (int th_num) {
    for(int loop_time = 0; loop_time < 5; ) {
        //waitN(cosume tokens num, max wait time/s )
        bool ok = lim_p->wait(2.0, seconds(20));
        if (ok) {
            /*
             * write your codes here
             */
            std::cout << "thread_num = " << th_num 
                    << ", loop_time = " << loop_time 
                    << " time : " 
                    << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() 
                    << std::endl;
            ++loop_time;
        } else {
            //if exceed max wait time
            std::this_thread::sleep_for(microseconds(10));
        }
    }
}

int main() {
    //first parameter->generate n tokens per s, second parameter->burst tokens
    //means generate 3.0 tokens per s
    //when rate <= 0, it will be set to MIN_RATE which equals DBL_MIN
    lim_p = new token_bucket::Limiter(3.0, 5.0);
    std::vector<std::thread*> vec(5, nullptr);
    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num] = new std::thread(foo, th_num);
    }

    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num]->join();
        delete vec[th_num];
    }
    delete lim_p;
    return 0;
}
