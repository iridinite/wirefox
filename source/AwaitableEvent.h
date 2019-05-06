/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once
#include "WirefoxTime.h"

namespace wirefox {
    
    namespace detail {
        
        /**
         * \brief Represents a thread blocker that can be signalled across threads.
         * 
         * This can be used as a synchronization primitive, or as an interruptible timer mechanism. For example,
         * you can call WaitFor() on one thread, and either wait for that time to run out, or call Signal() from
         * another thread to interrupt the timer.
         */
        class AwaitableEvent {
        public:
            AwaitableEvent() = default;

            /**
             * \brief Awaits this event indefinitely.
             * 
             * The thread will sleep without time limit, until Signal() is called.
             */
            void    Wait();

            /**
             * \brief Awaits this event for an amount of time, then returns.
             * 
             * The thread will sleep, generally consuming no additional CPU time on most implementations.
             * If Signal() is called while this method is blocking, this method will return immediately.
             * 
             * \param[in]   duration    The maximum amount of time that can be spent waiting.
             */
            void    WaitFor(Timespan duration);

            /**
             * \brief Signals any waiting threads to resume.
             */
            void    Signal();

        private:
            cfg::LockableMutex m_mutex;
            std::condition_variable m_cv;
        };

    }

}
