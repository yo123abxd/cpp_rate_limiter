#pragma once

#include <stdint.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

namespace token_bucket {

using std::chrono::duration;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::chrono::nanoseconds;

static bool is_equal_d(double d1, double d2) {
    static const double delta = 1e-20;
    return (d1 < d2 ? d2 - d1 : d1 - d2) < delta;
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
class Limiter;
class Reservation {
public:
    friend Limiter;
    typedef time_point<steady_clock, nanoseconds> time_point_ns;
    Reservation(bool ok, double tokens, time_point_ns time_to_act) : 
            _m_ok(ok), _m_tokens(tokens), _m_time_to_act(time_to_act) {}

    bool get_ok() {
        return _m_ok;
    }

    double get_tokens() {
        return _m_tokens;
    }

    nanoseconds duration_to_act() {
        return _m_time_to_act -steady_clock::now();
    }
private:
    //is reserve success
    bool _m_ok;
    //the tokens reserved
    double _m_tokens;
    //permitted time to act
    time_point_ns _m_time_to_act;
};

class Limiter {
public:
    typedef time_point<steady_clock, nanoseconds> time_point_ns;
    Limiter(double rate, double burst);
    std::shared_ptr<Reservation> reserve(double n, const nanoseconds& max_time_to_wait);

    bool wait(double n, const nanoseconds& max_time_to_wait);

    bool set_rate(double rate);
    bool set_burst(double burst);

    double tokens() {
        return _m_tokens;
    }

    double rate() {
        return _m_rate;
    }

    double burst() {
        return _m_burst;
    }
private:
    static const uint64_t POW_S_TO_NS = 1000000000;

    void update(time_point_ns now);
    double duration_to_tokens(nanoseconds duration) {
        return duration.count() * _m_rate / POW_S_TO_NS;
    }

    nanoseconds tokens_to_duration(double tokens) {
        if (_m_rate < 0.0 || is_equal_d(_m_rate, 0.0)) {
            return nanoseconds(0);
        }
        return nanoseconds(static_cast<uint64_t>(tokens / _m_rate * POW_S_TO_NS));
    }
    
    // rate per s 
    double _m_rate;

    // every deta_T in [start, end], there is always 
    // deta_T * _m_rate + burst >= n
    // n is the number of tokens taken in deta_T
    double _m_burst;

    // remaining tokens at _m_last_update_time
    double _m_tokens;

    // last_update_time is the last time the limiter's tokens field was updated
    time_point_ns _m_last_update_time;
    // last_event_time is the latest time of a rate-limited event (past or future)
    std::mutex mu;
};

Limiter::Limiter(double rate, double burst) : 
        _m_burst(burst), _m_tokens(0), _m_last_update_time(steady_clock::now()), mu() {
    _m_rate = rate < 0.0 ? 0.0 : rate;
}


//reset rate
bool Limiter::set_rate(double rate) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(steady_clock::now());
    if (rate >= 0) {
        nanoseconds dura = tokens_to_duration(_m_tokens);
        _m_rate = rate;
        _m_tokens = duration_to_tokens(dura);
        _m_rate = _m_rate > _m_burst ? _m_burst : _m_rate;
        return true;
    }
    return false;
}

//reset burst
bool Limiter::set_burst(double burst) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(steady_clock::now());
    _m_burst = burst;
    return true;
}

std::shared_ptr<Reservation> Limiter::reserve(double n, const nanoseconds& max_time_to_wait){
    AutoRelease<std::mutex> autoRelease(&mu);
    time_point_ns now = steady_clock::now();
    std::shared_ptr<Reservation> reservation_p = std::make_shared<Reservation>(false, n, now);
    if (now < _m_last_update_time) {
        _m_last_update_time = now;
    }
    update(now);

    if (n >= 0 && n <= _m_burst && duration_to_tokens(max_time_to_wait) - n >= -_m_tokens) {
        //ok
        _m_tokens -= n;
        reservation_p->_m_ok = true;
        reservation_p->_m_tokens = n;
        reservation_p->_m_time_to_act = _m_tokens >= 0 ? now : tokens_to_duration(-_m_tokens) + now;
    }
    return reservation_p;
}

bool Limiter::wait(double n, const nanoseconds& max_time_to_wait) {
    std::shared_ptr<Reservation> reservation_p = reserve(n, max_time_to_wait);
    time_point_ns curr = steady_clock::now();
    if (reservation_p->_m_ok && reservation_p->_m_time_to_act >= curr) {
        std::this_thread::sleep_until(reservation_p->_m_time_to_act);
    }
    return reservation_p->_m_ok;
}

void Limiter::update(time_point_ns now) {
    if (now < _m_last_update_time) {
        _m_last_update_time = now;
        return;
    }
    double curr_tokens = duration_to_tokens(now - _m_last_update_time) + _m_tokens;
    if (curr_tokens > _m_burst) {
        curr_tokens = _m_burst;
    }
    _m_tokens = curr_tokens;
    _m_last_update_time = now;
}

} //token_bucet
