/*
 * Wirefox Networking API
 * (C) Mika Molenkamp, 2019.
 *
 * Licensed under the BSD 3-Clause License, see the LICENSE file in the project
 * root folder for more information.
 */

#pragma once

/**
 * \file
 * \brief The purpose of this file is to include the typedef'd classes referenced in WirefoxConfig.h.
 * 
 * Because any concrete implementations need to include the actual correct header (ie. HandshakerThreeWay, not Handshaker),
 * the include sections in several source files might get more cluttered than strictly necessary, and more importantly, it
 * introduces several copies that need to be updated if I choose to ever change the typedefs. They also cannot be in
 * WirefoxConfig.h itself, because client apps might include that header, and they shouldn't see any detail:: stuff.
 * 
 * So instead I put these includes in this header, so other headers can include this one, while not directly relying on the
 * concrete implementation headers.
 */

// DefaultSocket
#include "SocketUDP.h"

// DefaultHandshaker
#include "HandshakerThreeWay.h"

// DefaultCongestionControl
#include "CongestionControlWindow.h"

// DefaultEncryption
#ifdef WIREFOX_ENABLE_ENCRYPTION
#include "EncryptionLayerSodium.h"
#else
#include "EncryptionLayerNull.h"
#endif
