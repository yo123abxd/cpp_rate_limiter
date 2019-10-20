/*
 * These codes below explain how to generate 3.0 tokens per second 
 * and cosume 2.0 tokens every time execute your codes with member
 * function reserve.
 */
#include "../include/Limiter.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <time.h>
#include <memory>
#include <sys/time.h>

using namespace std;

token_bucket::Limiter* lim_p;

void foo (int th_num) {
    for(int loop_time = 0; loop_time < 5; ) {
        //waitN(cosume tokens num, max wait time/s )
        shared_ptr<token_bucket::Reservation> reservation_p = 
                lim_p->reserveN(2.0, 
                token_bucket::Time(20, token_bucket::Time::TIME_UNIT_S));
        if (reservation_p->m_ok) {
            timeval curr_time;
            gettimeofday(&curr_time, nullptr);
            uint64_t usleep_time = 0;
            if ((usleep_time = reservation_p->m_time_to_act.get_time_us()
                    - (curr_time.tv_sec * 1000000 + curr_time.tv_usec)) > 0.0) {
                usleep(usleep_time);
            }
            /*
             * write your codes here
             */
            cout<<"thread_num = "<< th_num << ", loop_time = " << loop_time <<" time : "<< curr_time.tv_sec << " cosume tokens:" << reservation_p->m_tokens << endl;
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
