/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

/**
 * \internal \file PCH.h
 * \brief Precompiled header, do not include in client applications.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NO_MIN_MAX
#define NO_MIN_MAX
#endif

#pragma warning (push, 0)

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cassert>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <list>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <random>
#include <functional>
#include <sstream>

#include <asio.hpp>

#pragma warning (pop)

namespace wirefox {}
using namespace wirefox;
