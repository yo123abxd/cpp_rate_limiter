/*
 * These codes below explain how to reset "burst" when the instance of Limiter
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
                lim_p->reserve(loop_time + 1, seconds(20));
        if (reservation_p->get_ok()) {
            std::this_thread::sleep_for(reservation_p->duration_to_act());

            std::cout << "loop_time = " << loop_time <<" time_ms : " 
                    << duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() 
                    << " cosume tokens:" << reservation_p->get_tokens() << std::endl;
            ++loop_time;
        } else {
            //if exceed max wait time
            std::cout << "fail to take tokens, lim_p->burst() = " << lim_p->burst() << std::endl;
            std::this_thread::sleep_for(milliseconds(500));
        }
    }
}

void burst_setter() {
    std::this_thread::sleep_for(seconds(5));
    std::cout << "reset burst to 5.0\n";
    lim_p->set_burst(5.0);
}

int main() {
    //first parameter->generate n tokens per s, second parameter->burst tokens
    //means generate 5.0 tokens per s
    //when rate <= 0, it will not generator tokens anymore
    lim_p = new token_bucket::Limiter(5.0, 4.0);
    std::thread th_printer(printer);
    std::thread th_setter(burst_setter);

    th_printer.join();
    th_setter.join();
    delete lim_p;
    return 0;
}
