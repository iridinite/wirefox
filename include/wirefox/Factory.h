/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

namespace wirefox {

    namespace detail {

        /**
         * \brief Represents a utility that instantiates other classes.
         */
        template<class T>
        class Factory {
        public:
            Factory() = delete;
            ~Factory() = delete;

            /**
             * \brief Instantiates the wrapped type.
             * 
             * \tparam      Args    The type to create a new instance of.
             * \param[in]   arg     Variable list of arguments to pass to the constructor of T.
             * \returns     An owning pointer to a new instance of T.
             */
            template<typename... Args>
            static std::unique_ptr<T> Create(Args&&... arg);
        };

        template<class T>
        template<typename... Args>
        std::unique_ptr<T> Factory<T>::Create(Args&&... arg) {
            return std::make_unique<T>(std::forward<Args>(arg)...);
        }

    }

}
