#include "PCH.h"
#include "AwaitableEvent.h"

using namespace wirefox::detail;

void AwaitableEvent::Wait() {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    m_cv.wait(lock);
}

void AwaitableEvent::WaitFor(Timespan duration) {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(Time::ToMilliseconds(duration)));
}

void AwaitableEvent::Signal() {
    m_cv.notify_all();
}
