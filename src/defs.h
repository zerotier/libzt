/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

// --- lwIP
#define APPLICATION_POLL_FREQ           2
#define ZT_LWIP_TCP_TIMER_INTERVAL      50
#define STATUS_TMR_INTERVAL             500 // How often we check connection statuses (in ms)

// --- picoTCP 
#define MAX_PICO_FRAME_RX_BUF_SZ        ZT_MAX_MTU * 128

// --- jip

// --- General

// TCP Buffer sizes
#define DEFAULT_TCP_TX_BUF_SZ           1024 * 1024
#define DEFAULT_TCP_RX_BUF_SZ           1024 * 1024

// TCP RX/TX buffer soft boundaries
#define DEFAULT_TCP_TX_BUF_SOFTMAX      DEFAULT_TCP_TX_BUF_SZ * 0.80
#define DEFAULT_TCP_TX_BUF_SOFTMIN      DEFAULT_TCP_TX_BUF_SZ * 0.20
#define DEFAULT_TCP_RX_BUF_SOFTMAX      DEFAULT_TCP_RX_BUF_SZ * 0.80
#define DEFAULT_TCP_RX_BUF_SOFTMIN      DEFAULT_TCP_RX_BUF_SZ * 0.20

// UDP Buffer sizes (should be about the size of your MTU)
#define DEFAULT_UDP_TX_BUF_SZ           ZT_MAX_MTU
#define DEFAULT_UDP_RX_BUF_SZ           ZT_MAX_MTU * 128