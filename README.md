# token_bucket_cpp
A C++ header-only limiter implemented by token bucket

## Quick start

-It's very easy to setup. Just include `Limiter.h` in your code.
-`Limiter.h` are based on c++11, and it's thread safe
-demos are using pthread lib, when you compile it, don't forget to link pthread lib by using flag `-lpthread`

## Example

```c++
/*
 * These codes below explain how to generate 3.0 tokens per second
 * and cosume 1.0 tokens every time execute your codes.
 */
#include "../include/Limiter.h"
#include <thread>

using namespace std;

token_bucket::Limiter* lim_p;


int main() {
    lim_p = new token_bucket::Limiter(3.0, 5.0);
    for(int loop_time = 0; loop_time < 5; ) {
        //waitN(cosume tokens num, max wait time/s )
        bool ok = lim_p->wait(token_bucket::Time(20, token_bucket::Time::TIME_UNIT_S));
        if (ok) {
            /*
             * write your codes here
             */
            ++loop_time;
        } else {
            usleep(10000);
            // if exceed max wait time
        }
    }
    delete lim_p;
    return 0;
}
```