#include <float.h>
#include <math.h>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <sys/time.h>
#include <typeinfo>
#include <unistd.h>

namespace token_bucket {

static const uint64_t POW_S_TO_MS = 1000;
static const uint64_t POW_MS_TO_US = 1000;
static const uint64_t POW_S_TO_US = 1000000;

class Time {
public:
    enum unit {
        TIME_UNIT_S,
        TIME_UNIT_MS,
        TIME_UNIT_US
    };
    explicit Time(uint64_t time, unit u = TIME_UNIT_S) {
        set_time(time, u);
    }
    void set_time(uint64_t time, unit u) {
        switch (u) {
            case TIME_UNIT_S:
                _m_time_us = time * POW_S_TO_US;
                break;
            case TIME_UNIT_MS:
                _m_time_us = time * POW_S_TO_MS;
                break;
            case TIME_UNIT_US:
                _m_time_us = time;
                break;
            default:
                _m_time_us = 0;
                break;
        }
    }
    uint64_t get_time_us() {
        return _m_time_us;
    }

    uint64_t get_time_ms() {
        return _m_time_us / POW_S_TO_MS;
    }

    uint64_t get_time_s() {
        return _m_time_us / POW_S_TO_US;
    }

    bool operator==(const Time& t) {
        return _m_time_us == t._m_time_us;
    }

    bool operator<(const Time& t) {
        return _m_time_us < t._m_time_us;
    }
    
    bool operator>(const Time& t) {
        return _m_time_us > t._m_time_us;
    }

    bool operator>=(const Time& t) {
        return _m_time_us >= t._m_time_us;
    }

    bool operator<=(const Time& t) {
        return _m_time_us <= t._m_time_us;
    }

    Time operator-(const Time& t) {
        return Time(_m_time_us - t._m_time_us, TIME_UNIT_US);
    }

    Time operator+(const Time& t) {
        return Time(_m_time_us + t._m_time_us, TIME_UNIT_US);
    }
private:
    uint64_t _m_time_us;
};

static bool is_equal_d(double d1, double d2) {
    static const double delta = 1e-10;
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
    Reservation(bool ok, double tokens, Time time_to_act) : 
            m_ok(ok), m_tokens(tokens), m_time_to_act(time_to_act) {}
    //is reserve success
    bool m_ok;
    //the tokens reserved
    double m_tokens;
    //permitted time to act
    Time m_time_to_act;
};

class Limiter {
public:
    static const double MIN_RATE;
    Limiter(double rate, double burst);
    std::shared_ptr<Reservation> reserveN(double n, Time max_time_to_wait);
    std::shared_ptr<Reservation> reserve(Time max_time_to_wait);
    bool waitN(double n, Time max_time_to_wait);
    bool wait(Time max_time_to_wait);
    bool set_rate(double rate);
    bool set_burst(double burst);

    double rate() {
        return _m_rate;
    }
    double burst() {
        return _m_burst;
    }
    Time last_event_time() {
        return _m_last_event_time;
    }
private:
    void update(Time now);
    double duration_to_tokens(Time duration) {
        return duration.get_time_us() * _m_rate / POW_S_TO_US;
    }

    Time tokens_to_duration(double tokens) {
        return Time(tokens / _m_rate * POW_S_TO_US, Time::TIME_UNIT_US);
    }
    Time curr_time();

    
    // rate per s 
    double _m_rate;

    // every deta_T in [start, end], there is always 
    // deta_T * _m_rate + burst >= n
    // n is the number of tokens taken in deta_T
    double _m_burst;

    // remaining tokens at _m_last_update_time
    double _m_tokens;

    // last_update_time is the last time the limiter's tokens field was updated
    Time _m_last_update_time;
    // last_event_time is the latest time of a rate-limited event (past or future)
    Time _m_last_event_time;
    std::mutex mu;
};
const double Limiter::MIN_RATE = DBL_MIN;

Limiter::Limiter(double rate, double burst) : 
        _m_tokens(0),
        _m_last_update_time(0, Time::TIME_UNIT_S),
        _m_last_event_time(0, Time::TIME_UNIT_S) {
    Time curr = curr_time();
    _m_last_update_time = curr;
    _m_last_event_time = curr;
    _m_burst = burst < 0.0 ? 0.0 : burst;
    _m_rate = rate < 0.0 || is_equal_d(0.0, rate) ? MIN_RATE : rate;
}

Time Limiter::curr_time() {
    timeval curr_time_val;
    gettimeofday(&curr_time_val, nullptr);
    return Time(static_cast<uint64_t>(curr_time_val.tv_sec) * POW_S_TO_US + 
            static_cast<uint64_t>(curr_time_val.tv_usec), Time::TIME_UNIT_US);
}

bool Limiter::set_rate(double rate) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(curr_time());
    if (rate > 0 && !is_equal_d(0.0, rate)) {
        _m_rate = rate;
        return true;
    }
    return false;
}
bool Limiter::set_burst(double burst) {
    AutoRelease<std::mutex> autoRelease(&mu);
    update(curr_time());
    if (burst > 0) {
        _m_burst = burst;
        return true;
    }
    return false;
}

std::shared_ptr<Reservation> Limiter::reserveN(double n, Time max_time_to_wait) {
    AutoRelease<std::mutex> autoRelease(&mu);
    Time now = curr_time();
    std::shared_ptr<Reservation> reservation_p = 
            std::make_shared<Reservation>(false, n, Time(0, Time::TIME_UNIT_S));
    if (now < _m_last_update_time) {
        _m_last_update_time = now;
    }
    update(now);

    if (n <= _m_burst && duration_to_tokens(max_time_to_wait) - n >= -_m_tokens) {
        //ok
        _m_tokens -= n;
        reservation_p->m_ok = true;
        reservation_p->m_tokens = n;
        reservation_p->m_time_to_act = _m_tokens >= 0 ? now : tokens_to_duration(-_m_tokens) + now;
        _m_last_event_time = reservation_p->m_time_to_act;
    }
    return reservation_p;
}

std::shared_ptr<Reservation> Limiter::reserve(Time max_time_to_wait) {
    return reserveN(1.0, max_time_to_wait);
}

bool Limiter::waitN(double n, Time max_time_to_wait) {
    std::shared_ptr<Reservation> reservation_p = reserveN(n, max_time_to_wait);
    Time curr_us = curr_time();
    if (reservation_p->m_ok && reservation_p->m_time_to_act >= curr_us) {
        usleep((reservation_p->m_time_to_act - curr_us).get_time_us());
    }
    return reservation_p->m_ok;
}

bool Limiter::wait(Time max_time_to_wait) {
    std::shared_ptr<Reservation> reservation_p = reserveN(1.0, max_time_to_wait);
    Time curr_us = curr_time();
    if (reservation_p->m_ok && reservation_p->m_time_to_act >= curr_us) {
        usleep((reservation_p->m_time_to_act - curr_us).get_time_us());
    }
    return reservation_p->m_ok;
}



void Limiter::update(Time now) {
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
