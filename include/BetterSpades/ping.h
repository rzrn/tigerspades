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

#ifndef PING_H
#define PING_H

#include <pthread.h> // !!!

#include <enet/enet.h>
#include <parson.h>

#include <BetterSpades/hud.h>

typedef struct {
    int         current, max;
    char        name[32];
    char        map[21];
    char        gamemode[8];
    int         ping;
    char        identifier[32];
    char        country[4];
    GameVersion version;
} ServerEntry;

typedef struct News {
    Texture * image;
    char caption[65];
    char url[129];
    float tile_size;
    RGB3i color;
    struct News * next;
} News;

extern int server_count, player_count;
extern pthread_mutex_t serverlist_lock;
extern ServerEntry ** serverlist;
extern News * newslist;

typedef int (*ServerlistComparator)(const ServerEntry *, const ServerEntry *);

void ping_init();
void ping_deinit();
void ping_refresh();
const char * ping_status();

int serverlist_sort_players(const ServerEntry *, const ServerEntry *);
int serverlist_sort_name(const ServerEntry *, const ServerEntry *);
int serverlist_sort_map(const ServerEntry *, const ServerEntry *);
int serverlist_sort_mode(const ServerEntry *, const ServerEntry *);
int serverlist_sort_ping(const ServerEntry *, const ServerEntry *);

extern ServerlistComparator serverlist_comparator;
extern bool serverlist_descending;

void serverlist_sort();

#endif
