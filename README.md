# token_bucket_cpp
A C++ header-only & Cross-Platform limiter implemented with token bucket

## Quick start

- This library is implemented by c++11 stl, so it's Cross-Platform.And it's thread safe
- It's very easy to setup. Just include `Limiter.h` in your code.

## Example

```c++
/*
 * These codes below explain how to generate 3.0 tokens per second
 * and cosume 1.0 tokens every time execute your codes.
 * you can only take 5.0 tokens each time, or waitN will return false.
 */
#include <chrono>
#include <thread>

#include "../include/Limiter.h"

token_bucket::Limiter* lim_p;

int main() {
    lim_p = new token_bucket::Limiter(3.0, 5.0);
    for(int loop_time = 0; loop_time < 5; ) {
        //wait(cosume tokens num, max wait time/s )
        bool ok = lim_p->wait(1.0, std::chrono::seconds(20));
        if (ok) {
            /*
             * write your codes here
             */
            ++loop_time;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            // if exceed max wait time
        }
    }
    delete lim_p;
    return 0;
}
```

- for more examples, please read the codes in dir `demo/`
