/*
    Copyright © 2017–2020 ByteBit
    Copyright © 2023–2026 rzrn

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

#define _XOPEN_SOURCE 600

#include <math.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#include <http.h>

#include <parson.h>
#include <enet/enet.h>
#include <lodepng/lodepng.h>

#include <hashtable.h>

#include <bs/config.h>
#include <bs/window.h>
#include <bs/ping.h>
#include <bs/common.h>
#include <bs/list.h>
#include <bs/hud.h>
#include <bs/channel.h>
#include <bs/utils.h>
#include <bs/network.h>

typedef struct {
    ServerEntry * entry;
    ENetAddress addr;
    float timestamp;
    int trycount;
} PingTask;

size_t server_count = 0, player_count = 0;
ServerEntry ** serverlist = NULL;

News * newslist = NULL;

pthread_t ping_thread; pthread_mutex_t serverlist_mutex;

void serverlist_lock(void)   { pthread_mutex_lock(&serverlist_mutex);   }
void serverlist_unlock(void) { pthread_mutex_unlock(&serverlist_mutex); }

static void newslist_clear(void) {
    while (newslist != NULL) {
        News * next = newslist->next;
        free(newslist); newslist = next;
    }
}

static void serverlist_clear(void) {
    pthread_mutex_lock(&serverlist_mutex);

    for (size_t i = 0; i < server_count; i++)
        free(serverlist[i]);

    player_count = server_count = 0;

    pthread_mutex_unlock(&serverlist_mutex);
}

GameVersion json_get_game_version(const JSON_Object * obj) {
    const char * game_version = json_object_get_string(obj, "game_version");

    if (game_version != NULL) {
        if (strcmp(game_version, "0.75") == 0)
            return VER075;

        if (strcmp(game_version, "0.76") == 0)
            return VER076;
    }

    return VER07X;
}

#define IPKEY(addr) (((uint64_t) addr.host << 16) | (addr.port));

int serverlist_sort_default(const ServerEntry * a, const ServerEntry * b) {
    if (strcmp(a->country, "LAN") == 0)
        return -1;

    if (strcmp(b->country, "LAN") == 0)
        return 1;

    if (a->current == b->current) {
        if (a->ping == b->ping)
            return strcmp(a->name, b->name);
        else {
            if (a->ping < 0) return +1;
            if (b->ping < 0) return -1;

            return a->ping - b->ping;
        }
    }

    return b->current - a->current;
}

int serverlist_sort_players(const ServerEntry * a, const ServerEntry * b) {
    return b->current - a->current;
}

int serverlist_sort_name(const ServerEntry * a, const ServerEntry * b) {
    return strcmp(a->name, b->name);
}

int serverlist_sort_map(const ServerEntry * a, const ServerEntry * b) {
    return strcmp(a->map, b->map);
}

int serverlist_sort_mode(const ServerEntry * a, const ServerEntry * b) {
    return strcmp(a->gamemode, b->gamemode);
}

int serverlist_sort_ping(const ServerEntry * a, const ServerEntry * b) {
    if (a->ping < 0) return +1;
    if (b->ping < 0) return -1;

    return a->ping - b->ping;
}

ServerlistComparator serverlist_comparator = serverlist_sort_default;
bool serverlist_descending = true;

static int serverlist_cmp(const void * a, const void * b) {
    const ServerEntry * A = *((const ServerEntry **) a);
    const ServerEntry * B = *((const ServerEntry **) b);

    return serverlist_descending ? serverlist_comparator(A, B) : -serverlist_comparator(A, B);
}

void serverlist_sort(void) {
    qsort(serverlist, server_count, sizeof(ServerEntry *), serverlist_cmp);
}

static void ping_lan(ENetSocket socket) {
    static const ENetBuffer hellolan = {.data = "HELLOLAN", .dataLength = 8};
    static ENetAddress addr = {.host = 0xFFFFFFFF}; // 255.255.255.255

    int begin = max(0, settings.min_lan_port), end = min(65535, settings.max_lan_port);

    for (addr.port = begin; addr.port <= end; addr.port++)
        enet_socket_send(socket, &addr, &hellolan, 1);
}

static volatile bool working = true, quit = false;

const char * _status = NULL;

const char * ping_status(void) {
    return _status;
}

void * ping_update(void * data) {
    UNUSED(data);

    pthread_detach(pthread_self());

    begin: working = true;

    // Step 1: clean up everything if needed.
    _status = "Initializing...";

    newslist_clear();
    serverlist_clear();

    // Step 2: do blocking `http_get` requests.
    _status = "Connecting to the master server...";

    http_t * request_serverlist = NULL, * request_news = NULL;

    if (!offline) {
        if (serverlist_url != NULL)
            request_serverlist = http_get(serverlist_url, NULL);

        if (newslist_url != NULL)
            request_news = http_get(newslist_url, NULL);
    }

    // Step 3: shout loudly into the local network.
    _status = request_serverlist == NULL ? "Fetching local servers..." : "Fetching servers...";

    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);

    ENetSocket lan = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    enet_socket_set_option(lan, ENET_SOCKOPT_NONBLOCK, 1);
    enet_socket_set_option(lan, ENET_SOCKOPT_BROADCAST, 1);

    float ping_start = request_serverlist == NULL ? -INFINITY : INFINITY;
    float lan_ping_start = window_time(); ping_lan(lan);

    // Step 4: catch stones flying towards us.
    static const ENetBuffer hello = {.data = "HELLO", .dataLength = 5};

    HashTable pings; ht_setup(&pings, sizeof(uint64_t), sizeof(PingTask), 64);

    while (working && (request_serverlist != NULL ||
                             request_news != NULL ||
               window_time() - ping_start <= 8.0F ||
           window_time() - lan_ping_start <= 5.0F)) {
        if (request_serverlist != NULL) switch (http_process(request_serverlist)) {
            case HTTP_STATUS_PENDING: break;

            case HTTP_STATUS_COMPLETED: {
                JSON_Value * js = json_parse_string(request_serverlist->response_data);
                JSON_Array * servers = json_value_get_array(js);

                pthread_mutex_lock(&serverlist_mutex);

                size_t begin = server_count; server_count += json_array_get_count(servers);
                serverlist = realloc(serverlist, server_count * sizeof(ServerEntry));
                CHECK_ALLOCATION_ERROR(serverlist)

                for (size_t k = begin; k < server_count; k++) {
                    JSON_Object * s = json_array_get_object(servers, k - begin);

                    serverlist[k] = malloc(sizeof(ServerEntry));

                    serverlist[k]->current = (int) json_object_get_number(s, "players_current");
                    serverlist[k]->max     = (int) json_object_get_number(s, "players_max");

                    if (settings.serverlist_send_ping)
                        serverlist[k]->ping = -1;
                    else
                        serverlist[k]->ping = (int) json_object_get_number(s, "latency");

                    strnzcpy(serverlist[k]->name,       json_object_get_string(s, "name"),       sizeof(serverlist[k]->name));
                    strnzcpy(serverlist[k]->map,        json_object_get_string(s, "map"),        sizeof(serverlist[k]->map));
                    strnzcpy(serverlist[k]->gamemode,   json_object_get_string(s, "game_mode"),  sizeof(serverlist[k]->gamemode));
                    strnzcpy(serverlist[k]->identifier, json_object_get_string(s, "identifier"), sizeof(serverlist[k]->identifier));
                    strnzcpy(serverlist[k]->country,    json_object_get_string(s, "country"),    sizeof(serverlist[k]->country));

                    serverlist[k]->version = json_get_game_version(s);

                    Address addr;

                    if (settings.serverlist_send_ping && network_identifier_split(serverlist[k]->identifier, &addr)) {
                        PingTask task = {
                            .entry     = serverlist[k],
                            .trycount  = 0,
                            .addr.port = addr.port,
                            .timestamp = window_time(),
                        };
                        enet_address_set_host(&task.addr, addr.ip);

                        uint64_t ID = IPKEY(task.addr);
                        ht_insert(&pings, &ID, &task);

                        enet_socket_send(sock, &task.addr, &hello, 1);
                    }

                    player_count += serverlist[k]->current;
                }

                serverlist_sort();
                pthread_mutex_unlock(&serverlist_mutex);

                http_release(request_serverlist);
                json_value_free(js);
                request_serverlist = NULL;

                ping_start = window_time();

                break;
            }

            case HTTP_STATUS_FAILED: {
                http_release(request_serverlist);
                request_serverlist = NULL;

                break;
            }
        }

        if (request_news != NULL) switch (http_process(request_news)) {
            case HTTP_STATUS_COMPLETED: {
                JSON_Value * js = json_parse_string(request_news->response_data);
                JSON_Array * news = json_value_get_array(js);
                int news_entries = json_array_get_count(news);

                News * current = NULL;

                for (int k = 0; k < news_entries; k++) {
                    if (current != NULL) {
                        current->next = calloc(1, sizeof(News));
                        current = current->next;
                    } else current = newslist = calloc(1, sizeof(News));

                    JSON_Object * s = json_array_get_object(news, k);
                    if (json_object_get_string(s, "caption"))
                        strncpy(current->caption, json_object_get_string(s, "caption"), sizeof(current->caption) - 1);

                    if (json_object_get_string(s, "url"))
                        strncpy(current->url, json_object_get_string(s, "url"), sizeof(current->url) - 1);

                    int color = json_object_get_number(s, "color");

                    current->tile_size = json_object_get_number(s, "tilesize");
                    current->color     = (RGB3i) {.r = color & 0xFF, .g = (color >> 8) & 0xFF, .b = (color >> 16) & 0xFF};
                    current->image     = NULL;

                    if (json_object_get_string(s, "image")) {
                        char * img = (char *) json_object_get_string(s, "image");
                        size_t imglen = strlen(img);

                        if (imglen > 0) {
                            int size = base64_decode(img, imglen);

                            uint32_t * buffer; unsigned int width, height;
                            lodepng_decode32((unsigned char **) &buffer, &width, &height, (uint8_t *) img, size);

                            current->image = texture_alloc();
                            texture_create_buffer(current->image, "image", width, height, buffer, true);
                            texture_filter(current->image, TEXTURE_FILTER_LINEAR);
                        }
                    }
                }

                json_value_free(js);
                http_release(request_news);
                request_news = NULL;
                break;
            }

            case HTTP_STATUS_FAILED: {
                http_release(request_news);
                request_news = NULL;
                break;
            }

            default: break;
        }

        char tmp[512]; ENetAddress from;
        ENetBuffer buf = {.data = tmp, .dataLength = sizeof(tmp)};

        {
            int recv = enet_socket_receive(sock, &from, &buf, 1);

            if (recv != 0) {
                uint64_t ID = IPKEY(from);
                PingTask * task = ht_lookup(&pings, &ID);

                if (task != NULL) {
                    if (recv > 0) { // received something!
                        if (!strncmp(buf.data, "HI", recv)) {

                            float dt = window_time() - task->timestamp;

                            pthread_mutex_lock(&serverlist_mutex);
                            task->entry->ping = ceil(dt * 1000.0F);
                            serverlist_sort();
                            pthread_mutex_unlock(&serverlist_mutex);

                            ht_erase(&pings, &ID);
                        } else if (task->trycount >= 3) {
                            ht_erase(&pings, &ID);
                        } else {
                            enet_socket_send(sock, &task->addr, &hello, 1);
                            task->timestamp = window_time();
                            task->trycount++;

                            log_warn("Ping timeout on %s, retrying (attempt %i)", task->entry->identifier, task->trycount);
                        }
                    } else ht_erase(&pings, &ID); // connection was closed
                }
            }
        }

        if (enet_socket_receive(lan, &from, &buf, 1) > 0) {
            float ping = window_time() - lan_ping_start;

            JSON_Value * js = json_parse_string(buf.data);
            if (js != NULL) {
                JSON_Object * root = json_value_get_object(js);

                ServerEntry * e = malloc(sizeof(ServerEntry));

                strcpy(e->country, "LAN");
                snprintf(e->identifier, sizeof(e->identifier) - 1, "aos://%u:%u", from.host, from.port);

                strnzcpy(e->name,     json_object_get_string(root, "name"),      sizeof(e->name));
                strnzcpy(e->gamemode, json_object_get_string(root, "game_mode"), sizeof(e->gamemode));
                strnzcpy(e->map,      json_object_get_string(root, "map"),       sizeof(e->map));

                e->current = json_object_get_number(root, "players_current");
                e->max     = json_object_get_number(root, "players_max");
                e->version = json_get_game_version(root);
                e->ping    = ceil(ping * 1000.0F);

                pthread_mutex_lock(&serverlist_mutex);
                serverlist = realloc(serverlist, (server_count + 1) * sizeof(ServerEntry *));
                serverlist[server_count] = e;

                player_count += e->current; server_count++;

                serverlist_sort();
                pthread_mutex_unlock(&serverlist_mutex);

                json_value_free(js);
            }
        }
    }

    // Step 5: wait until we get kicked.
    _status = "Nothing to see here";

    enet_socket_destroy(sock);
    enet_socket_destroy(lan);
    ht_destroy(&pings);

    while (working) usleep(10000);

    if (quit) return NULL;

    goto begin; // it hurts
}

void ping_init(void) {
    #ifdef SIGPIPE
        signal(SIGPIPE, SIG_IGN);
    #endif

    pthread_mutex_init(&serverlist_mutex, NULL);

    pthread_create(&ping_thread, NULL, ping_update, NULL);
}

void ping_refresh(void) {
    serverlist_comparator = serverlist_sort_default;
    serverlist_descending = true;

    working = false;
}

void ping_deinit(void) {
    quit = true; working = false;
    void * ret; pthread_join(ping_thread, &ret);
}
