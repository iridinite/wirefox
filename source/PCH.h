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

#ifdef _MSC_VER
// suppress code analysis warnings in library code
#include <codeanalysis/warnings.h>
#pragma warning(push)
#pragma warning(disable: ALL_CODE_ANALYSIS_WARNINGS)
#endif

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

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "WirefoxConfig.h"

namespace wirefox {}
using namespace wirefox;
