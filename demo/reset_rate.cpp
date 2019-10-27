/*
 * These codes below explain how to reset "rate" when the instance of Limiter 
 * you already constructed success
 */
#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <chrono>

#include "../include/Limiter.h"

using std::chrono::system_clock;
using std::chrono::seconds;
using std::chrono::milliseconds;
using std::chrono::duration_cast;

token_bucket::Limiter* lim_p;

void printer () {
    for(int loop_time = 0; loop_time < 5; ) {
        //reserve(cosume tokens num, max wait time/s )
        std::shared_ptr<token_bucket::Reservation> reservation_p = 
                lim_p->reserve(1.0, seconds(10));
        if (reservation_p->get_ok()) {
            std::this_thread::sleep_for(reservation_p->duration_to_act());

            std::cout << "loop_time = " << loop_time <<" time_ms : " 
                    << duration_cast<seconds>(system_clock::now().time_since_epoch()).count() 
                    << " cosume tokens:" << reservation_p->get_tokens() << std::endl;
            ++loop_time;
        } else {
            //if exceed max wait time
            std::this_thread::sleep_for(milliseconds(500));
            std::cout<<"fail sleep 500ms\n";
        }
    }
}

void burst_setter() {
    std::this_thread::sleep_for(milliseconds(3500));
    std::cout << "reset rate to 5.0\n";
    lim_p->set_rate(5.0);
}

int main() {
    //first parameter->generate n tokens per s, second parameter->burst tokens
    //means generate 1.0 tokens per s
    //when rate <= 0, it will not generator tokens anymore
    lim_p = new token_bucket::Limiter(1.0, 5.0);

    std::thread th_setter(burst_setter);

    std::vector<std::thread*> vec(2, nullptr);
    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num] = new std::thread(printer);
    }

    th_setter.join();
    for (int th_num = 0; th_num < static_cast<int>(vec.size()); th_num++) {
        vec[th_num]->join();
        delete vec[th_num];
    }
    delete lim_p;
    return 0;
}
