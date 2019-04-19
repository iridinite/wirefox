/**
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <cassert>

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <random>

#include <wirefox/WirefoxConfig.h>

#include <wirefox/Enumerations.h>
#include <wirefox/PeerAbstract.h>
#include <wirefox/BinaryStream.h>
#include <wirefox/Packet.h>
#include <wirefox/WirefoxTime.h>
#include <wirefox/Serializable.h>
