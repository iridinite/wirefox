/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

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
