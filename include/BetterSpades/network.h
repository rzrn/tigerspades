/*
    Copyright (c) 2017-2020 ByteBit

    This file is part of BetterSpades.

    BetterSpades is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BetterSpades is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NETWORK_H
#define NETWORK_H

#include <AceOfSpades/protocol.h>
#include <BetterSpades/common.h>

#define MASKON(X)   (1 << X)
#define MASKOFF(X) ~(1 << X)

#define SETBIT(dest, bit, value) { dest &= MASKOFF(bit); dest |= (value << bit); }
#define HASBIT(dest, bit) ((dest) & MASKON(bit))

typedef struct { char ip[32]; int port; GameVersion version; } Address;

const char * network_reason_disconnect(ErrorCode code);

unsigned int network_ping(void);

void network_updateColor(void);
void network_disconnect(void);
int network_identifier_split(char * addr, Address *);
int network_connect(Address *);
int network_connect_string(char * addr, GameVersion);
int network_update(void);
void network_init(void);

void network_join_game(unsigned char team, unsigned char weapon);

extern void (*packets[256])(uint8_t * data, int len);

extern bool network_connected, network_map_transfer, network_logged_in;

extern int network_received_packets;

extern float network_pos_update;
extern float network_orient_update;
extern unsigned char network_keys_last;
extern unsigned char network_buttons_last;
extern unsigned char network_tool_last;

extern uint8_t * compressed_chunk_data;
extern int compressed_chunk_data_size;
extern int compressed_chunk_data_offset;
extern int compressed_chunk_data_estimate;

typedef struct {
    int outgoing;
    int ingoing;
    int avg_ping;
} NetworkStat;

extern NetworkStat network_stats[40];

extern float network_stats_last;

static inline Vector3f ntohv3f(const Vector3f v)
{ return (Vector3f) {.x = v.x, .y = 63.0F - v.z, .z = v.y}; }

static inline Vector3f htonv3f(const Vector3f v)
{ return (Vector3f) {.x = v.x, .y = v.z, .z = 63.0F - v.y}; }

static inline Vector3i ntohv3i(const Vector3i v)
{ return (Vector3i) {.x = v.x, .y = 63 - v.z, .z = v.y}; }

static inline Vector3i htonv3i(const Vector3i v)
{ return (Vector3i) {.x = v.x, .y = v.z, .z = 63 - v.y}; }

static inline Vector3f ntohov3f(const Vector3f v)
{ return (Vector3f) {.x = v.x, .y = -v.z, .z = v.y}; }

static inline Vector3f htonov3f(const Vector3f v)
{ return (Vector3f) {.x = v.x, .y = v.z, .z = -v.y}; }

void restock();

void handlePacketBlockAction(PacketBlockAction *);
void handlePacketBlockLine(PacketBlockLine *);
void handlePacketGrenade(PacketGrenade *);

void doPacketBlockAction(PacketBlockAction *);
void doPacketBlockLine(PacketBlockLine *, int);

bool isdestructible(int, int, int);

static inline char sector1f(float x) { return 'A' + (int) (x / 64.0F); }
static inline char sector2f(float y) { return '1' + (int) (y / 64.0F); }

#define PACKET_INCOMPLETE 0
#define PACKET_EXTRA      0
#define PACKET_SERVERSIDE 0
#define begin(T) extern void send##T(T * contained, size_t len);
#include <AceOfSpades/packets.h>

#endif
