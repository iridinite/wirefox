#pragma once
#include "WirefoxTime.h"

namespace wirefox {
    
    namespace detail {
        
        class AwaitableEvent {
        public:
            AwaitableEvent() = default;

            void    Wait();
            void    WaitFor(Timespan duration);

            void    Signal();

        private:
            cfg::LockableMutex m_mutex;
            std::condition_variable m_cv;
        };

    }

}
