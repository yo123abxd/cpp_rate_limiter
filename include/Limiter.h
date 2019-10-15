#include <math.h>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <sys/time.h>
#include <typeinfo>
#include <unistd.h>

//123
#include <iostream>
//123

//if no notice, all the time unit is us
namespace token_bucket {
typedef uint64_t time_usec;

static const uint64_t sec_to_usec_pow = 1000000;
static bool is_equal_d(double d1, double d2) {
    static const double delta = 1e-9;
    return fabs(d1 - d2) < delta;
}

template<class T>
class AutoRelease {
public:
    AutoRelease(T *mu) : _m_mu(mu){
        _m_mu->lock();
    }
    ~AutoRelease() {
        _m_mu->unlock();
    }
private:
    T *_m_mu;
};

// A Reservation holds information about events that are permitted by a Limiter to happen after a delay.
class Reservation {
public:
    Reservation(bool ok, double tokens, time_usec time_to_act) : 
            m_ok(ok), m_tokens(tokens), m_time_to_act_us(time_to_act) {}
    //is reserve success
    bool m_ok;
    //the tokens reserved
    double m_tokens;
    //permitted time to act
    time_usec m_time_to_act_us;
};

class Limiter {
public:
    static const double min_rate;
    Limiter(double rate, double burst);
    std::shared_ptr<Reservation> reserveN(double n, time_usec max_time_to_wait);
    std::shared_ptr<Reservation> reserve(time_usec max_time_to_wait);
    bool waitN(double n, time_usec max_time_to_wait);
    bool wait(time_usec max_time_to_wait);
    bool set_rate(double rate);
    bool set_burst(double burst);

    double rate_us() {
        return _m_rate_us;
    }
    double burst() {
        return _m_burst;
    }
    time_usec last_event_time() {
        return _m_last_event_time_us;
    }
private:
    void update(time_usec now);
    double duration_to_tokens(time_usec duration) {
        return (duration * _m_rate_us);
    }

    time_usec tokens_to_duration(double tokens) {
        return tokens / _m_rate_us;
    }
    time_usec curr_time_us();

    
    // rate per us 
    double _m_rate_us;

    // every deta_T in [start, end], there is always 
    // deta_T * _m_rate_us + burst >= n
    // n is the number of tokens taken in deta_T
    double _m_burst;

    // remaining tokens at _m_last_update_time_us
    double _m_tokens;

    // last_update_time is the last time the limiter's tokens field was updated
    time_usec _m_last_update_time_us;
    // last_event_time is the latest time of a rate-limited event (past or future)
    time_usec _m_last_event_time_us;
    std::mutex mu;
};
const double Limiter::min_rate = 1e-9;

Limiter::Limiter(double rate, double burst) {
    time_usec curr = curr_time_us();
    _m_last_update_time_us = curr;
    _m_last_event_time_us = curr;
    _m_burst = burst < 0.0 ? 0.0 : burst;
    _m_rate_us = rate < 0.0 || is_equal_d(0.0, rate) ? 1e-9 : rate;
}

time_usec Limiter::curr_time_us() {
    timeval curr_time;
    gettimeofday(&curr_time, nullptr);
    return static_cast<time_usec>(curr_time.tv_sec) * sec_to_usec_pow + 
            static_cast<time_usec>(curr_time.tv_usec);
}

bool Limiter::set_rate(double rate) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(curr_time_us());
    if (rate > 0 && !is_equal_d(0.0, rate)) {
        _m_rate_us = rate;
        return true;
    }
    return false;
}
bool Limiter::set_burst(double burst) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(curr_time_us());
    if (burst > 0) {
        _m_burst = burst;
        return true;
    }
    return false;
}

std::shared_ptr<Reservation> Limiter::reserveN(double n, time_usec max_time_to_wait) {
    AutoRelease<std::mutex> autoRelease(&mu);
    time_usec now = curr_time_us();
    std::shared_ptr<Reservation> reservation_p = 
            std::make_shared<Reservation>(false, n, 0);
    if (now < _m_last_update_time_us) {
        _m_last_update_time_us = now;
    }
    update(now);

    if (n <= _m_burst && duration_to_tokens(max_time_to_wait) - n >= -_m_tokens) {
        //ok
        _m_tokens -= n;
        reservation_p->m_ok = true;
        reservation_p->m_tokens = n;
        reservation_p->m_time_to_act_us = _m_tokens >= 0 ? now : tokens_to_duration(-_m_tokens) + now;
        _m_last_event_time_us = reservation_p->m_time_to_act_us;
    }
    return reservation_p;
}

std::shared_ptr<Reservation> Limiter::reserve(time_usec max_time_to_wait) {
    return reserveN(1, max_time_to_wait);
}

bool Limiter::waitN(double n, time_usec max_time_to_wait) {
    std::shared_ptr<Reservation> reservation_p = reserveN(n, max_time_to_wait);
    time_usec curr_us = curr_time_us();
    if (reservation_p->m_ok && reservation_p->m_time_to_act_us >= curr_us) {
        usleep(reservation_p->m_time_to_act_us - curr_us);
    }
    return reservation_p->m_ok;
}

bool Limiter::wait(time_usec max_time_to_wait) {
    std::shared_ptr<Reservation> reservation_p = reserveN(1, max_time_to_wait);
    time_usec curr_us = curr_time_us();
    if (reservation_p->m_ok && reservation_p->m_time_to_act_us >= curr_us) {
        usleep(reservation_p->m_time_to_act_us - curr_us);
    }
    return reservation_p->m_ok;
}



void Limiter::update(time_usec now) {
    if (now < _m_last_update_time_us) {
        _m_last_update_time_us = now;
        return;
    }
    double curr_tokens = duration_to_tokens(now - _m_last_update_time_us) + _m_tokens;
    if (curr_tokens > _m_burst) {
        curr_tokens = _m_burst;
    }
    _m_tokens = curr_tokens;
    _m_last_update_time_us = now;
}

} //token_bucet
