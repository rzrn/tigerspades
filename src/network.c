/*
    Copyright © 2017–2023 ByteBit
    Copyright © 2018 vuolen
    Copyright © 2018 yvt
    Copyright © 2018 Aleksy
    Copyright © 2021–2022 Haxk20
    Copyright © 2022 Julius C. Enriquez
    Copyright © 2025 Ashy
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

#include <string.h>
#include <ctype.h>
#include <math.h>

#include <libdeflate.h>
#include <enet/enet.h>

#include <bs/texture.h>
#include <bs/common.h>
#include <bs/sound.h>
#include <bs/weapon.h>
#include <bs/grenade.h>
#include <bs/camera.h>
#include <bs/cameracontroller.h>
#include <bs/file.h>
#include <bs/hud.h>
#include <bs/map.h>
#include <bs/player.h>
#include <bs/network.h>
#include <bs/particle.h>
#include <bs/texture.h>
#include <bs/chunk.h>
#include <bs/config.h>
#include <bs/unicode.h>

void (*packets[256])(uint8_t * data, size_t len) = {NULL};

bool network_connected    = false;
bool network_map_transfer = false;
bool network_logged_in    = false;
bool network_map_cached   = false;

int network_received_packets = 0;

Vector3f network_pos_last, network_orient_last;

float network_pos_update           = 0.0F;
float network_orient_update        = 0.0F;
unsigned char network_keys_last    = 0;
unsigned char network_buttons_last = 0;
unsigned char network_tool_last    = 255;

uint8_t * compressed_chunk_data;

size_t compressed_chunk_data_size;
size_t compressed_chunk_data_offset = 0;
size_t compressed_chunk_data_estimate = 0;

NetworkStat network_stats[40];
float network_stats_last = 0.0F;

ENetHost * client = NULL;
ENetPeer * peer = NULL;

char network_custom_reason[CHAT_MESSAGE_SIZE];

static float connection_timestamp = -INFINITY;
static ProtocolVersion connection_version;

const char * network_reason_disconnect(ErrorCode code) {
    if (*network_custom_reason)
        return network_custom_reason;

    switch (code) {
        case ERROR_BANNED:               return "Banned";
        case ERROR_TOO_MANY_CONNECTIONS: return "Connection limit";
        case ERROR_WRONG_PROTOCOL:       return "Wrong protocol";
        case ERROR_FULL:                 return "Server full";
        case ERROR_SHUTDOWN:             return "Server shutdown";
        case ERROR_KICKED:               return "Kicked";
        case ERROR_INVALID_NICKNAME:     return "Invalid name";
        default:                         return "Unknown";
    }
}

static inline void beep(void) { sound_create(SOUND_LOCAL, sound(SOUND_CHAT), 0.0F, 0.0F, 0.0F); }

static inline void printJoinMsg(int team, char * name) {
    if (network_map_transfer || camera.mode == CAMERAMODE_SELECTION)
        return;

    char * t;
    switch (team) {
        case TEAM1: t = gamestate.team1.name; break;
        case TEAM2: t = gamestate.team2.name; break;
        default:
        case TEAM_SPECTATOR: t = "Spectator"; break;
    }

    char buff[64]; sprintf(buff, "%s joined the %s team", name, t);
    game_chat_add(Red, buff, sizeof(buff), UTF8);

    if (settings.connect_beep) beep();
}

static inline void printLeaveMsg(char * name) {
    if (network_map_transfer || camera.mode == CAMERAMODE_SELECTION)
        return;

    char buff[32]; sprintf(buff, "%s disconnected", name);
    game_chat_add(Red, buff, sizeof(buff), UTF8);

    if (settings.disconnect_beep) beep();
}

bool isdestructible(int x, int y, int z) {
    UNUSED(x); UNUSED(z);

    return (network_connected ? y > 1 : y > 0) && y < 64;
}

static uint8_t network_buffer[512];

static void network_send(int id, size_t len) {
    if (peer != NULL) {
        network_stats[0].outgoing += len + 1;
        network_buffer[0] = id;

        enet_peer_send(peer, 0, enet_packet_create(network_buffer, len + 1, ENET_PACKET_FLAG_RELIABLE));
    }
}

void network_join_game(unsigned char team, unsigned char weapon) {
    char namebuff[17]; encodeMagic(namebuff, sizeof(namebuff), settings.name, sizeof(settings.name));

    PacketExistingPlayer contained;
    contained.player_id = local_player.id;
    contained.team      = team;
    contained.weapon    = weapon;
    contained.tool      = TOOL_DEFAULT;
    contained.kills     = 0;
    contained.color     = players[local_player.id].block;
    contained.name      = namebuff;

    sendPacketExistingPlayer(&contained, strsize(namebuff, sizeof(namebuff)));
}

#define PACKET_INCOMPLETE 0
#define PACKET_EXTRA      0
#define PACKET_SERVERSIDE 0
#define begin(T) void send##T(T * contained, size_t len) \
                 { write##T(network_buffer + 1, contained); \
                   network_send(id##T, size##T + len); }
#include <ace/packets.h>

#define ERRLEN(T, len) do { log_error(#T " of invalid size (%ld) was received.", len); return; } while (false)
#define READPACKET(T, contained, src, len) T contained; if (size##T <= len) read##T(src, &contained); else ERRLEN(T, len);

static void getPacketPositionData(uint8_t * data, size_t len) {
    READPACKET(PacketPositionData, p, data, len);
    players[local_player.id].pos = ntohv3f(p.pos);
}

static void getPacketOrientationData(uint8_t * data, size_t len) {
    READPACKET(PacketOrientationData, p, data, len);
    players[local_player.id].orientation = ntohov3f(p.orient);
}

static void getPacketInputData(uint8_t * data, size_t len) {
    READPACKET(PacketInputData, p, data, len);

    if (IDVALID(p.player_id)) {
        if (p.player_id != local_player.id)
            players[p.player_id].input.keys = p.keys;

        players[p.player_id].physics.jump = HASBIT(p.keys, INPUT_JUMP) > 0;
    }
}

static void getPacketWeaponInput(uint8_t * data, size_t len) {
    READPACKET(PacketWeaponInput, p, data, len);

    if (IDVALID(p.player_id) && p.player_id != local_player.id) {
        players[p.player_id].input.buttons = p.input;

        float time = window_time();

        if (HASBIT(p.input, BUTTON_PRIMARY))
            players[p.player_id].start.lmb = time;
        if (HASBIT(p.input, BUTTON_SECONDARY))
            players[p.player_id].start.rmb = time;
    }
}

void handlePacketGrenade(PacketGrenade * p) {
    grenade_add(&(Grenade) {
        .team        = players[p->player_id].team,
        .fuse_length = p->fuse_length,
        .pos         = ntohv3f(p->pos),
        .velocity    = ntohov3f(p->vel),
    });
}

static void getPacketGrenade(uint8_t * data, size_t len) {
    READPACKET(PacketGrenade, p, data, len);
    handlePacketGrenade(&p);
}

static void getPacketSetTool(uint8_t * data, size_t len) {
    READPACKET(PacketSetTool, p, data, len);

    if (IDVALID(p.player_id) && p.tool < 4)
        players[p.player_id].tool = TOOL(p.tool);
}

static void getPacketSetColor(uint8_t * data, size_t len) {
    READPACKET(PacketSetColor, p, data, len);

    if (IDVALID(p.player_id)) {
        players[p.player_id].block = p.color;

        if (p.player_id == local_player.id)
            local_player.color[X] = local_player.color[Y] = -1;
    }
}

static void getPacketExistingPlayer(uint8_t * data, size_t len) {
    READPACKET(PacketExistingPlayer, p, data, len);

    if (IDVALID(p.player_id)) {
        decodeMagic(players[p.player_id].name, sizeof(players[p.player_id].name), p.name, len - sizePacketExistingPlayer);

        if (!players[p.player_id].connected) printJoinMsg(p.team, players[p.player_id].name);

        player_reset(&players[p.player_id]);
        players[p.player_id].connected     = true;
        players[p.player_id].alive         = true;
        players[p.player_id].team          = TEAM(p.team);
        players[p.player_id].weapon        = WEAPON(p.weapon);
        players[p.player_id].tool          = TOOL(p.tool);
        players[p.player_id].score         = p.kills;
        players[p.player_id].block         = p.color;
        players[p.player_id].ammo          = weapon_ammo(p.weapon);
        players[p.player_id].ammo_reserved = weapon_ammo_reserved(p.weapon);
    }
}

void handlePacketBlockAction(PacketBlockAction * p) {
    int x = p->pos.x, y = p->pos.y, z = p->pos.z;

    switch (p->action_type) {
        case ACTION_DESTROY: {
            if (63 - z > 0) {
                RGBA4i col = map_get(x, 63 - z, y);

                map_set(x, 63 - z, y, NULL);
                map_update_physics(x, 63 - z, y);

                particle_create(col, x + 0.5F, 63 - z + 0.5F, y + 0.5F, 2.5F, 1.0F, 8, 0.1F, 0.25F);
            }

            break;
        }

        case ACTION_GRENADE: {
            for (int j = (63 - z) - 1; j <= (63 - z) + 1; j++)
            for (int k = y - 1; k <= y + 1; k++)
            for (int i = x - 1; i <= x + 1; i++) {
                if (j > 1) {
                    map_set(i, j, k, NULL);
                    map_update_physics(i, j, k);
                }
            }

            break;
        }

        case ACTION_SPADE: {
            if ((63 - z - 1) > 1) {
                map_set(x, 63 - z - 1, y, NULL);
                map_update_physics(x, 63 - z - 1, y);
            }

            if ((63 - z + 0) > 1) {
                RGBA4i col = map_get(x, 63 - z, y);

                map_set(x, 63 - z + 0, y, NULL);
                map_update_physics(x, 63 - z + 0, y);

                particle_create(col, x + 0.5F, 63 - z + 0.5F, y + 0.5F, 2.5F, 1.0F, 8, 0.1F, 0.25F);
            }

            if ((63 - z + 1) > 1) {
                map_set(x, 63 - z + 1, y, NULL);
                map_update_physics(x, 63 - z + 1, y);
            }

            break;
        }

        case ACTION_BUILD: {
            if (IDVALID(p->player_id)) {
                float xc = x + 0.5F, yc = 63.0F - z + 0.5F, zc = y + 0.5F;

                if (map_isair(x, 63 - z, y))
                    sound_create(SOUND_WORLD, sound(SOUND_BUILD), xc, yc, zc);

                RGBA4i color = RGB3iAs4i(players[p->player_id].block);
                map_set(x, 63 - z, y, &color);
            }

            break;
        }
    }
}

void handlePacketBlockLine(PacketBlockLine * p) {
    if (!IDVALID(p->player_id)) return;

    int sx = p->start.x, sy = p->start.y, sz = p->start.z;
    int ex = p->end.x,   ey = p->end.y,   ez = p->end.z;

    RGBA4i color = {
        players[p->player_id].block.r,
        players[p->player_id].block.g,
        players[p->player_id].block.b,
        255
    };

    if (sx == ex && sy == ey && sz == ez) {
        map_set(sx, 63 - sz, sy, &color);
    } else for (LineRasterizer i = cube_line(sx, sy, sz, ex, ey, ez); !i.exhausted; rasterizer_next(&i)) {
        if (BLOCKLINE_MAX_LENGTH <= i.index) break; // current piqueserver’s implementation limit, may be extended in the future as an extension

        int x = i.x, y = 63 - i.z, z = i.y;
        if (map_isair(x, y, z)) map_set(x, y, z, &color);
    }

    sound_create(
        SOUND_WORLD, sound(SOUND_BUILD),
        (sx + ex) * 0.5F + 0.5F,
        (63 - sz + 63 - ez) * 0.5F + 0.5F,
        (sy + ey) * 0.5F + 0.5F
    );
}

void doPacketBlockAction(PacketBlockAction * p) {
    if (p->action_type == ACTION_BUILD)
        local_player.blocks = max(local_player.blocks - 1, 0);

    if (network_connected)
        sendPacketBlockAction(p, 0);
    else
        handlePacketBlockAction(p);
}

void doPacketBlockLine(PacketBlockLine * p, int amount) {
    local_player.blocks -= amount;

    if (network_connected)
        sendPacketBlockLine(p, 0);
    else
        handlePacketBlockLine(p);
}

static void getPacketBlockLine(uint8_t * data, size_t len) {
    READPACKET(PacketBlockLine, p, data, len);
    handlePacketBlockLine(&p);
}

static void getPacketBlockAction(uint8_t * data, size_t len) {
    READPACKET(PacketBlockAction, p, data, len);
    handlePacketBlockAction(&p);
}

static inline void chat_show_notice(const char * msg, size_t size, Codepage codepage)
{ chat_show_popup(msg, size, codepage, 5.0F, White); }

static inline void chat_show_warning(const char * msg, size_t size, Codepage codepage)
{ sound_create(SOUND_LOCAL, sound(SOUND_BEEP1), 0.0F, 0.0F, 0.0F);
  chat_show_popup(msg, size, codepage, 5.0F, Yellow); }

static inline void chat_show_error(const char * msg, size_t size, Codepage codepage)
{ sound_create(SOUND_LOCAL, sound(SOUND_BEEP2), 0.0F, 0.0F, 0.0F);
  chat_show_popup(msg, size, codepage, 5.0F, Red); }

static inline void chat_show_status(const char * msg, size_t size, Codepage codepage)
{ chat_show_popup(msg, size, codepage, 5.0F, Red); }

static inline bool decodeOpenSpadesAlert(const char * msg, size_t size, Codepage codepage) {
    if (size <= 3) return false;

    if (msg[1] == '%' && msg[2] == ' ') switch (msg[0]) {
        case 'N': chat_show_notice(&msg[3], size - 3, codepage);  return true;
        case '%': chat_show_warning(&msg[3], size - 3, codepage); return true;
        case '!': chat_show_error(&msg[3], size - 3, codepage);   return true;
        case 'C': chat_show_status(&msg[3], size - 3, codepage);  return true;
    }

    return false;
}

static void getPacketChatMessage(uint8_t * data, size_t len) {
    if (network_map_transfer) return;

    READPACKET(PacketChatMessage, p, data, len);

    size_t size = len - sizePacketChatMessage;

    Codepage codepage = CP437; char * msg = p.message;
    if (msg[0] == '\xFF') { msg++; size--; codepage = UTF8; }

    char buff[256];
    switch (p.chat_type) {
        case CHAT_INFO:    chat_show_notice(msg, size, codepage);  return;
        case CHAT_WARNING: chat_show_warning(msg, size, codepage); return;
        case CHAT_ERROR:   chat_show_error(msg, size, codepage);   return;
        case CHAT_BIG:     chat_show_status(msg, size, codepage);  return;

        case CHAT_SYSTEM: {
            if (decodeOpenSpadesAlert(msg, size, codepage))
                return;

            if (p.player_id == 255) {
                strnzcpy(network_custom_reason, msg, sizeof(network_custom_reason));
                return; // don’t add message to chat
            }

            buff[0] = 0;
            break;
        }

        case CHAT_ALL: case CHAT_TEAM: {
            if (IDVALID(p.player_id) && players[p.player_id].connected) {
                if (settings.chat_beep && camera.mode != CAMERAMODE_SELECTION) beep();

                char prefix[32] = {0};

                switch (players[p.player_id].team) {
                    case TEAM1: sprintf(prefix, "%s (%s)", players[p.player_id].name, gamestate.team1.name); break;
                    case TEAM2: sprintf(prefix, "%s (%s)", players[p.player_id].name, gamestate.team2.name); break;
                    case TEAM_SPECTATOR: sprintf(prefix, "%s (Spectator)", players[p.player_id].name); break;
                }

                sprintf(buff, "%s: ", prefix);
            } else if (decodeOpenSpadesAlert(msg, size, codepage)) {
                return;
            } else {
                sprintf(buff, ": ");
            }

            break;
        }
    }

    {
        size_t offset = strlen(buff);
        convert(buff + offset, sizeof(buff) - offset, UTF8, msg, size, codepage);
    }

    RGBA4i color = {255, 255, 255, 255};
    switch (p.chat_type) {
        case CHAT_SYSTEM: color = Red; break;
        case CHAT_TEAM: {
            switch (players[p.player_id].connected ? players[p.player_id].team : players[local_player.id].team) {
                case TEAM1: color.r = gamestate.team1.color.r; color.g = gamestate.team1.color.g; color.b = gamestate.team1.color.b; break;
                case TEAM2: color.r = gamestate.team2.color.r; color.g = gamestate.team2.color.g; color.b = gamestate.team2.color.b; break;

                default: break;
            }
        }
    }

    game_chat_add(color, buff, sizeof(buff), UTF8);
}

static inline void addExtInfoEntry(uint8_t id, uint8_t version, size_t * dest) {
    PacketExtInfoEntry extension;
    extension.id      = id;
    extension.version = version;

    size_t offset = 1 + sizePacketExtInfo + *dest; // skip packet id byte & header
    *dest += writePacketExtInfoEntry(network_buffer + offset, &extension);
}

static const char * getExtensionName(uint8_t id) {
    switch (id) {
        case EXT_PLAYER_PROPERTIES: return "Player Properties";
        case EXT_TRACE_BULLETS:     return "Trace Bullets";
        case EXT_HIT_EFFECTS:       return "Hit Effects";
        case EXT_CUSTOM_STAT:       return "Custom Statistics";
        case EXT_DAMAGE_MARKERS:    return "Damage Markers";
        case EXT_256PLAYERS:        return "Player Limit";
        case EXT_MESSAGES:          return "Message Types";
        case EXT_KICKREASON:        return "Kick Reason";
        default:                    return "Unknown";
    }
}

static void getPacketExtInfo(uint8_t * data, size_t len) {
    READPACKET(PacketExtInfo, p, data, len);

    if (len >= sizePacketExtInfo + p.length * sizePacketExtInfoEntry) {
        if (p.length > 0) {
            PacketExtInfoEntry ext;

            log_info("Server supports the following extensions:");
            for (int k = 0; k < p.length; k++) {
                readPacketExtInfoEntry(data + sizePacketExtInfo + k * sizePacketExtInfoEntry, &ext);

                log_info(
                    "Extension 0x%02X (%s) of version %i %s",
                    ext.id, getExtensionName(ext.id), ext.version,
                    ext.id >= 192 ? "(which is packetless)" : ""
                );

                if (ext.id == EXT_HIT_EFFECTS)
                    local_hit_effects = false;
            }
        } else log_info("Server does not support extensions");

        size_t offset = 0, length = 0;

        addExtInfoEntry(EXT_PLAYER_PROPERTIES, 1, &offset); length++;
        addExtInfoEntry(EXT_256PLAYERS,        1, &offset); length++;
        addExtInfoEntry(EXT_MESSAGES,          1, &offset); length++;
        addExtInfoEntry(EXT_KICKREASON,        1, &offset); length++;
        addExtInfoEntry(EXT_TRACE_BULLETS,     1, &offset); length++;
        addExtInfoEntry(EXT_HIT_EFFECTS,       1, &offset); length++;
        addExtInfoEntry(EXT_CUSTOM_STAT,       1, &offset); length++;
        addExtInfoEntry(EXT_DAMAGE_MARKERS,    1, &offset); length++;

        PacketExtInfo reply; reply.length = length;
        sendPacketExtInfo(&reply, offset);
    }
}

static void getPacketWorldUpdate075(uint8_t * data, size_t len) {
    if (len % sizePacketWorldUpdate075 == 0) {
        size_t offset = 0;

        for (size_t k = 0; k < len / sizePacketWorldUpdate075; k++) { // supports up to 256 players
            PacketWorldUpdate075 p; offset += readPacketWorldUpdate075(data + offset, &p);

            if (players[k].connected && players[k].alive && k != local_player.id) {
                Vector3f r = ntohv3f(p.pos);

                if (normv3f(players[k].pos, r) > 0.01F)
                    players[k].pos = r;

                players[k].orientation = ntohov3f(p.orient);
            }
        }
    } else log_error("Invalid PacketWorldUpdate of length %ld (0.75)", len);
}

static void getPacketWorldUpdate076(uint8_t * data, size_t len) {
    if (len % sizePacketWorldUpdate076 == 0) {
        size_t offset = 0;

        for (size_t k = 0; k < len / sizePacketWorldUpdate076; k++) {
            PacketWorldUpdate076 p; offset += readPacketWorldUpdate076(data + offset, &p);

            if (players[p.player_id].connected && players[p.player_id].alive && p.player_id != local_player.id) {
                Vector3f r = ntohv3f(p.pos);

                if (normv3f(players[p.player_id].pos, r) > 0.01F)
                    players[p.player_id].pos = r;

                players[p.player_id].orientation = ntohov3f(p.orient);
            }
        }
    } else log_error("Invalid PacketWorldUpdate of length %ld (0.76)", len);
}

static void getPacketWorldUpdate(uint8_t * data, size_t len) {
    if (len > 0) switch (connection_version) {
        case VERSION_075: getPacketWorldUpdate075(data, len); break;
        case VERSION_076: getPacketWorldUpdate076(data, len); break;
    }
}

static void getPacketSetHP(uint8_t * data, size_t len) {
    READPACKET(PacketSetHP, p, data, len);

    local_player.health = p.hp;

    if (p.type == DAMAGE_SOURCE_GUN) {
        local_player.last_damage_timer = window_time();
        sound_create(SOUND_LOCAL, sound(SOUND_HITPLAYER), 0.0F, 0.0F, 0.0F);
    }

    local_player.last_damage = ntohv3f(p.pos);
}

static void getPacketShortPlayerData(uint8_t * data, size_t len) {
    UNUSED(data); UNUSED(len); // should never be received
    log_warn("Unexpected ShortPlayerDataPacket");
}

static void getPacketMoveObject(uint8_t * data, size_t len) {
    READPACKET(PacketMoveObject, p, data, len);

    if (gamestate.mode == GAMEMODE_CTF) {
        switch (p.object_id) {
            case TEAM1_BASE: gamestate.ctf.team1_base = ntohv3f(p.pos); break;
            case TEAM2_BASE: gamestate.ctf.team2_base = ntohv3f(p.pos); break;

            case TEAM1_FLAG: {
                gamestate.ctf.team2_has_intel = false;
                gamestate.ctf.team1_flag = ntohv3f(p.pos);
                break;
            }

            case TEAM2_FLAG: {
                gamestate.ctf.team1_has_intel = false;
                gamestate.ctf.team2_flag = ntohv3f(p.pos);
                break;
            }
        }
    }

    if (gamestate.mode == GAMEMODE_TC && p.object_id < gamestate.tc.territory_count) {
        gamestate.tc.territory[p.object_id].pos  = ntohv3f(p.pos);
        gamestate.tc.territory[p.object_id].team = TEAM(p.team);
    }
}

static void getPacketCreatePlayer(uint8_t * data, size_t len) {
    READPACKET(PacketCreatePlayer, p, data, len);

    if (IDVALID(p.player_id)) {
        decodeMagic(players[p.player_id].name, sizeof(players[p.player_id].name), p.name, len - sizePacketCreatePlayer);

        if (!players[p.player_id].connected) printJoinMsg(p.team, players[p.player_id].name);

        RGB3i rgb = players[p.player_id].block;

        player_reset(&players[p.player_id]);
        players[p.player_id].connected = true;
        players[p.player_id].alive     = true;
        players[p.player_id].team      = TEAM(p.team);
        players[p.player_id].tool      = TOOL_DEFAULT;
        players[p.player_id].weapon    = WEAPON(p.weapon);
        players[p.player_id].pos       = ntohv3f(p.pos);

        Vector3f o = {p.team == TEAM1 ? 1.0F : -1.0F, 0.0F, 0.0F};

        players[p.player_id].orientation        = o;
        players[p.player_id].orientation_smooth = o;

        players[p.player_id].block = Gray;

        players[p.player_id].ammo          = weapon_ammo(p.weapon);
        players[p.player_id].ammo_reserved = weapon_ammo_reserved(p.weapon);

        if (p.player_id == local_player.id) {
            if (p.team == TEAM_SPECTATOR)
                camera.r = ntohv3f(p.pos);

            local_player.health    = 100;
            local_player.blocks    = 50;
            local_player.grenades  = 3;
            local_player.last_tool = TOOL_DEFAULT;

            camera.mode  = p.team == TEAM_SPECTATOR ? CAMERAMODE_SPECTATOR : CAMERAMODE_FPS;
            camera.rot.h = p.team == TEAM1 ? 0.5F * PI : 1.5F * PI;
            camera.rot.v = 0.5F * PI;

            camera.muzzle = camera.crosshair = camera.rot;

            if (settings.persistent_block_color) {
                players[local_player.id].block = rgb;
                updateBlockColor();
            } else local_player.color[X] = local_player.color[Y] = -1;

            network_logged_in = true;

            weapon_set(false);
        }
    }
}

static inline uint8_t readu8le(uint8_t * buff)
{ size_t dummy = 0; return getu8le(buff, &dummy); }

static inline Vector3f readv3f(uint8_t * buff)
{ size_t dummy = 0; return getv3f(buff, &dummy); }

static void getPacketStateData(uint8_t * data, size_t len) {
    if (len < sizePacketStateData) ERRLEN(PacketStateData, len);

    PacketStateData p; size_t offset = readPacketStateData(data, &p);

    decodeMagic(gamestate.team1.name, sizeof(gamestate.team1.name), (char *) p.team1_name.data, p.team1_name.size);
    gamestate.team1.color = p.team1;

    decodeMagic(gamestate.team2.name, sizeof(gamestate.team2.name), (char *) p.team2_name.data, p.team2_name.size);
    gamestate.team2.color = p.team2;

    gamestate.mode = p.gamemode;

    if (p.gamemode == GAMEMODE_CTF) {
        if (len < sizePacketStateData + sizeCTFStateData) ERRLEN(PacketStateData, len);

        CTFStateData ctf; offset += readCTFStateData(data + offset, &ctf);
        gamestate.ctf.team1_score     = ctf.team1_score;
        gamestate.ctf.team2_score     = ctf.team2_score;
        gamestate.ctf.capture_limit   = ctf.capture_limit;
        gamestate.ctf.team1_has_intel = HASBIT(ctf.intels, TEAM1_HAS_INTEL);
        gamestate.ctf.team2_has_intel = HASBIT(ctf.intels, TEAM2_HAS_INTEL);

        if (HASBIT(ctf.intels, TEAM2_HAS_INTEL))
            gamestate.ctf.team1_carrier = readu8le(ctf.team1_intel_location.data);
        else
            gamestate.ctf.team1_flag = ntohv3f(readv3f(ctf.team1_intel_location.data));

        if (HASBIT(ctf.intels, TEAM1_HAS_INTEL))
            gamestate.ctf.team2_carrier = readu8le(ctf.team2_intel_location.data);
        else
            gamestate.ctf.team2_flag = ntohv3f(readv3f(ctf.team2_intel_location.data));

        gamestate.ctf.team1_base = ntohv3f(ctf.team1_base);
        gamestate.ctf.team2_base = ntohv3f(ctf.team2_base);
    } else if (p.gamemode == GAMEMODE_TC) {
        if (len < sizePacketStateData + sizeTCStateData) ERRLEN(PacketStateData, len);

        TCStateData tc; offset += readTCStateData(data + offset, &tc);
        gamestate.tc.territory_count = tc.territory_count;

        for (size_t i = 0; i < tc.territory_count; i++) {
            TCTerritory territory; offset += readTCTerritory(data + offset, &territory);

            gamestate.tc.territory[i].pos  = ntohv3f(territory.pos);
            gamestate.tc.territory[i].team = TEAM(territory.team);
        }
    } else log_error("Unknown gamemode (%d)!", p.gamemode);

    if (settings.play_intro) sound_create(SOUND_LOCAL, sound(SOUND_INTRO), 0.0F, 0.0F, 0.0F);

    fog_color[0] = ((float) p.fog.r) / 255.0F;
    fog_color[1] = ((float) p.fog.g) / 255.0F;
    fog_color[2] = ((float) p.fog.b) / 255.0F;

    local_player.id       = p.player_id;
    local_player.health   = 100;
    local_player.blocks   = 50;
    local_player.grenades = 3;
    weapon_set(false);

    players[local_player.id].block = Gray;

    if (0 <= default_team && 0 <= default_gun) {
        network_join_game(default_team, default_gun);
        screen_current = SCREEN_NONE;
    } else if (default_team == TEAM_SPECTATOR) {
        network_join_game(default_team, WEAPON_RIFLE);
        screen_current = SCREEN_NONE;
    } else if (0 <= default_team) {
        screen_current = SCREEN_GUN_SELECT;
    } else {
        screen_current = SCREEN_TEAM_SELECT;
    }

    if (settings.show_welcome_screen) {
        camera.mode = CAMERAMODE_SELECTION;
    } else {
        players[local_player.id].team = TEAM_SPECTATOR;

        screen_current = SCREEN_NONE;
        camera.mode = CAMERAMODE_SPECTATOR;

        camera.r         = (Vector3f) {256.0F, 79.0F, 256.0F};
        camera.v         = (Vector3f) {0.0F, 0.0F, 0.0F};
        camera.rot       = (Euler2f) {M_PI, 3.14F};
        camera.crosshair = (Euler2f) {M_PI, 3.14F};
        camera.muzzle    = (Euler2f) {M_PI, 3.14F};
    }

    network_map_transfer = false;
    chat_popup_duration  = 0;

    hud_change(&hud_ingame);

    if (!network_map_cached && compressed_chunk_data != NULL) {
        log_info("Map data was %i bytes", compressed_chunk_data_offset);

        int avail_size = 1024 * 1024;

        void * decompressed = malloc(avail_size);
        CHECK_ALLOCATION_ERROR(decompressed)

        size_t decompressed_size;
        struct libdeflate_decompressor * d = libdeflate_alloc_decompressor();

        for (;;) {
            int r = libdeflate_zlib_decompress(
                d, compressed_chunk_data, compressed_chunk_data_offset,
                decompressed, avail_size, &decompressed_size
            );

            // switch not fancy enough here, breaking out of the loop is not aesthetic
            if (r == LIBDEFLATE_INSUFFICIENT_SPACE) {
                avail_size += 1024 * 1024;
                decompressed = realloc(decompressed, avail_size);
                CHECK_ALLOCATION_ERROR(decompressed)
            }

            if (r == LIBDEFLATE_SUCCESS) {
                map_vxl_load(decompressed, decompressed_size);

#ifndef USE_TOUCH
                if (settings.map_cache) {
                    char filename[128];
                    sprintf(filename, "cache/%08X.vxl", libdeflate_crc32(0, decompressed, decompressed_size));
                    log_info("Map cached: %s", filename);
                    FILE * f = fopen(filename, "wb");
                    fwrite(decompressed, 1, decompressed_size, f);
                    fclose(f);
                }
#endif
                chunk_rebuild_all();
                break;
            }

            if (r == LIBDEFLATE_BAD_DATA || r == LIBDEFLATE_SHORT_OUTPUT)
                break;
        }

        free(decompressed);

        libdeflate_free_decompressor(d);

        free(compressed_chunk_data);
        compressed_chunk_data = NULL;
    }
}

void player_reset_toggleable_input(void) {
    if (config_key(WINDOW_KEY_SPRINT)->toggle)
        window_pressed_keys[WINDOW_KEY_SPRINT] = 0;
}

static void getPacketKillAction(uint8_t * data, size_t len) {
    READPACKET(PacketKillAction, p, data, len);

    if (IDVALID(p.player_id) && IDVALID(p.killer_id) && KILLTYPEVALID(p.kill_type)) {
        if (p.player_id == local_player.id) {
            player_reset_toggleable_input();

            local_player.death_time       = window_time();
            local_player.respawn_time     = p.respawn_time;
            local_player.respawn_cnt_last = 255;
            sound_create(SOUND_LOCAL, sound(SOUND_DEATH), 0.0F, 0.0F, 0.0F);

            if (p.player_id != p.killer_id) {
                local_player.last_damage_timer = local_player.death_time;
                local_player.last_damage       = players[p.killer_id].pos;

                cameracontroller_death_init(local_player.id, players[p.killer_id].pos);
            } else {
                cameracontroller_death_init(local_player.id, (Vector3f) {0.0F, 0.0F, 0.0F});
            }
        }

        players[p.player_id].alive         = false;
        players[p.player_id].input.keys    = 0;
        players[p.player_id].input.buttons = 0;

        if (players[p.player_id].team != players[p.killer_id].team)
            players[p.killer_id].score++;

        static char * gun_name[] = {"Rifle", "SMG", "Shotgun"};

        char buff[256];
        switch (p.kill_type) {
            case KILLTYPE_WEAPON:
                sprintf(buff, "%s killed %s (%s)", players[p.killer_id].name, players[p.player_id].name,
                        gun_name[players[p.killer_id].weapon]);
                break;
            case KILLTYPE_HEADSHOT:
                sprintf(buff, "%s killed %s (Headshot)", players[p.killer_id].name, players[p.player_id].name);
                break;
            case KILLTYPE_MELEE:
                sprintf(buff, "%s killed %s (Spade)", players[p.killer_id].name, players[p.player_id].name);
                break;
            case KILLTYPE_GRENADE:
                sprintf(buff, "%s killed %s (Grenade)", players[p.killer_id].name, players[p.player_id].name);
                break;
            case KILLTYPE_FALL: sprintf(buff, "%s fell too far", players[p.player_id].name); break;
            case KILLTYPE_CLASSCHANGE: sprintf(buff, "%s changed weapons", players[p.player_id].name); break;
            case KILLTYPE_TEAMCHANGE: {
                sprintf(buff, "%s changed teams", players[p.player_id].name);
                if (settings.team_change_beep && camera.mode != CAMERAMODE_SELECTION) beep();

                break;
            }
        }

        if (p.killer_id == local_player.id || p.player_id == local_player.id) {
            local_player.last_kill_timer = window_time();
            game_killfeed_add(Red, buff, sizeof(buff));

            if (settings.kill_indicator) {
                if (p.kill_type == KILLTYPE_WEAPON || p.kill_type == KILLTYPE_HEADSHOT)
                    sound_create(SOUND_LOCAL, sound(SOUND_SPADE_WHACK), 0.0F, 0.0F, 0.0F);
            }
        } else switch (players[p.killer_id].team) {
            case TEAM1: game_killfeed_add(RGB3iAs4i(gamestate.team1.color), buff, sizeof(buff)); break;
            case TEAM2: game_killfeed_add(RGB3iAs4i(gamestate.team2.color), buff, sizeof(buff)); break;

            case TEAM_SPECTATOR: break;
        }
    }
}

static void getPacketMapStart(uint8_t * data, size_t len) {
    free(compressed_chunk_data);

    // ffs someone fix the wrong map size of 1.5 MiB
    compressed_chunk_data_size = 1024 * 1024;
    compressed_chunk_data      = malloc(compressed_chunk_data_size);
    CHECK_ALLOCATION_ERROR(compressed_chunk_data)

    compressed_chunk_data_offset = 0;

    network_logged_in    = false;
    network_map_transfer = true;
    network_map_cached   = false;

    player_init();
    trajectories_reset();

    hud_change(&hud_mapload);

    switch (connection_version) {
        case VERSION_075: {
            READPACKET(PacketMapStart075, p, data, len);
            compressed_chunk_data_estimate = p.map_size;

            break;
        }

        case VERSION_076: {
            READPACKET(PacketMapStart076, p, data, len);
            compressed_chunk_data_estimate = p.map_size;

            data[len - 1] = 0;

            char filename[128]; sprintf(filename, "cache/%08X.vxl", p.crc32);

            if (file_exists(filename)) {
                network_map_cached = true;
                void * mapd = file_load(filename);
                map_vxl_load(mapd, file_size(filename));
                free(mapd);

                chunk_rebuild_all();
            }

            log_info("Map name: %.*s %s", len - sizePacketMapStart076, p.map_name, network_map_cached ? "(cached)" : "");
            log_info("Map crc32: 0x%08X", p.crc32);

            PacketMapCached reply; reply.cached = network_map_cached;
            sendPacketMapCached(&reply, 0);

            break;
        }
    }
}

static void getPacketMapChunk(uint8_t * data, size_t len) {
    if (compressed_chunk_data == NULL)
        return;

    // increase allocated memory if it is not enough to store the next chunk
    if (compressed_chunk_data_offset + len > compressed_chunk_data_size) {
        compressed_chunk_data_size += 1024 * 1024;
        compressed_chunk_data = realloc(compressed_chunk_data, compressed_chunk_data_size);
        CHECK_ALLOCATION_ERROR(compressed_chunk_data)
    }

    // accept any chunk length for “superior” performance, as pointed out by github/NotAFile
    memcpy(compressed_chunk_data + compressed_chunk_data_offset, data, len);
    compressed_chunk_data_offset += len;
}

static void getPacketPlayerLeft(uint8_t * data, size_t len) {
    READPACKET(PacketPlayerLeft, p, data, len);

    if (IDVALID(p.player_id)) {
        players[p.player_id].connected = false;
        players[p.player_id].alive     = false;
        players[p.player_id].score     = 0;

        printLeaveMsg(players[p.player_id].name);
    }
}

static void getPacketTerritoryCapture(uint8_t * data, size_t len) {
    READPACKET(PacketTerritoryCapture, p, data, len);

    if (gamestate.mode == GAMEMODE_TC && p.tent < gamestate.tc.territory_count) {
        gamestate.tc.territory[p.tent].team = TEAM(p.team);
        sound_create(SOUND_LOCAL, sound(p.winning ? SOUND_HORN : SOUND_PICKUP), 0.0F, 0.0F, 0.0F);

        char * team_name = NULL;

        switch (p.team) {
            case TEAM1: team_name = gamestate.team1.name; break;
            case TEAM2: team_name = gamestate.team2.name; break;
        }

        if (team_name != NULL) {
            char capture_str[128];

            char x = sector1f(gamestate.tc.territory[p.tent].pos.x);
            char y = sector2f(gamestate.tc.territory[p.tent].pos.z);

            sprintf(capture_str, "%s have captured %c%c", team_name, x, y);
            game_chat_add(Red, capture_str, sizeof(capture_str), UTF8);

            if (p.winning) {
                sprintf(capture_str, "%s Team Wins!", team_name);
                chat_show_popup(capture_str, sizeof(capture_str), UTF8, 5.0F, Red);
            }
        }
    }
}

static void getPacketProgressBar(uint8_t * data, size_t len) {
    READPACKET(PacketProgressBar, p, data, len);

    if (gamestate.mode == GAMEMODE_TC && p.tent < gamestate.tc.territory_count) {
        gamestate.tc.progress       = clamp(0.0F, 1.0F, p.progress);
        gamestate.tc.rate           = p.rate;
        gamestate.tc.tent           = p.tent;
        gamestate.tc.team_capturing = TEAM(p.team_capturing);
        gamestate.tc.last_update    = window_time();
    }
}

static void getPacketIntelCapture(uint8_t * data, size_t len) {
    READPACKET(PacketIntelCapture, p, data, len);

    if (gamestate.mode == GAMEMODE_CTF && IDVALID(p.player_id)) {
        char capture_str[128];
        switch (players[p.player_id].team) {
            case TEAM1:
                gamestate.ctf.team1_score++;
                sprintf(capture_str, "%s has captured the %s Intel", players[p.player_id].name, gamestate.team2.name);
                break;
            case TEAM2:
                gamestate.ctf.team2_score++;
                sprintf(capture_str, "%s has captured the %s Intel", players[p.player_id].name, gamestate.team1.name);
                break;

            case TEAM_SPECTATOR:
                break;
        }

        sound_create(SOUND_LOCAL, sound(p.winning ? SOUND_HORN : SOUND_PICKUP), 0.0F, 0.0F, 0.0F);
        players[p.player_id].score += 10;
        game_chat_add(Red, capture_str, sizeof(capture_str), UTF8);

        if (p.winning) {
            char * name = NULL;

            switch (players[p.player_id].team) {
                case TEAM1: name = gamestate.team1.name; break;
                case TEAM2: name = gamestate.team2.name; break;

                case TEAM_SPECTATOR: break;
            }

            sprintf(capture_str, "%s Team Wins!", name);
            chat_show_popup(capture_str, sizeof(capture_str), UTF8, 5.0F, Red);

            gamestate.ctf.team1_score = 0;
            gamestate.ctf.team2_score = 0;
        }
    }
}

static void getPacketIntelPickup(uint8_t * data, size_t len) {
    READPACKET(PacketIntelPickup, p, data, len);

    if (gamestate.mode == GAMEMODE_CTF && IDVALID(p.player_id)) {
        char pickup_str[128];
        switch (players[p.player_id].team) {
            case TEAM1: {
                gamestate.ctf.team1_has_intel = true;
                gamestate.ctf.team2_carrier = p.player_id;
                sprintf(pickup_str, "%s has the %s Intel", players[p.player_id].name, gamestate.team2.name);
                break;
            }

            case TEAM2: {
                gamestate.ctf.team2_has_intel = true;
                gamestate.ctf.team1_carrier = p.player_id;
                sprintf(pickup_str, "%s has the %s Intel", players[p.player_id].name, gamestate.team1.name);
                break;
            }

            case TEAM_SPECTATOR:
                break;
        }

        game_chat_add(Red, pickup_str, sizeof(pickup_str), UTF8);
        sound_create(SOUND_LOCAL, sound(SOUND_PICKUP), 0.0F, 0.0F, 0.0F);
    }
}

static void getPacketIntelDrop(uint8_t * data, size_t len) {
    READPACKET(PacketIntelDrop, p, data, len);

    if (gamestate.mode == GAMEMODE_CTF && IDVALID(p.player_id)) {
        char drop_str[128];
        switch (players[p.player_id].team) {
            case TEAM1:
                gamestate.ctf.team1_has_intel = false;
                gamestate.ctf.team2_flag = ntohv3f(p.pos);
                sprintf(drop_str, "%s has dropped the %s Intel", players[p.player_id].name, gamestate.team2.name);
                break;
            case TEAM2:
                gamestate.ctf.team2_has_intel = false;
                gamestate.ctf.team1_flag = ntohv3f(p.pos);
                sprintf(drop_str, "%s has dropped the %s Intel", players[p.player_id].name, gamestate.team1.name);
                break;
            case TEAM_SPECTATOR:
                break;
        }

        game_chat_add(Red, drop_str, sizeof(drop_str), UTF8);
    }
}

void restock(void) {
    local_player.health   = 100;
    local_player.blocks   = 50;
    local_player.grenades = 3;
    weapon_set(true);

    sound_create(SOUND_LOCAL, sound(SOUND_SWITCH), 0.0F, 0.0F, 0.0F);
}

static void getPacketRestock(uint8_t * data, size_t len) {
    UNUSED(data); UNUSED(len); restock();
}

static void getPacketFogColor(uint8_t * data, size_t len) {
    READPACKET(PacketFogColor, p, data, len);
    fog_color[0] = ((float) p.color.r) / 255.0F;
    fog_color[1] = ((float) p.color.g) / 255.0F;
    fog_color[2] = ((float) p.color.b) / 255.0F;
}

static void getPacketWeaponReload(uint8_t * data, size_t len) {
    READPACKET(PacketWeaponReload, p, data, len);

    if (p.player_id == local_player.id) {
        local_player.ammo          = p.ammo;
        local_player.ammo_reserved = p.reserved;
    } else {
        sound_create_sticky(weapon_sound_reload(players[p.player_id].weapon), players + p.player_id, p.player_id);

        // don’t use values from packet which somehow are never correct
        players[p.player_id].ammo          = weapon_ammo(players[p.player_id].weapon);
        players[p.player_id].ammo_reserved = weapon_ammo_reserved(players[p.player_id].weapon);
    }
}

static void getPacketChangeWeapon(uint8_t * data, size_t len) {
    READPACKET(PacketChangeWeapon, p, data, len);

    if (IDVALID(p.player_id)) {
        if (p.player_id == local_player.id) {
            log_warn("Unexpected ChangeWeaponPacket");
            return;
        }

        players[p.player_id].weapon = WEAPON(p.weapon);
    }
}

static void getPacketHandshakeInit(uint8_t * data, size_t len) {
    READPACKET(PacketHandshakeInit, p, data, len);

    PacketHandshakeReturn reply; reply.challenge = p.challenge;
    sendPacketHandshakeReturn(&reply, 0);
}

#if HACKS_ENABLED
    static char operatingsystem[] = OS " " ARCH " (w/ hacks)";
#else
    static char operatingsystem[] = OS " " ARCH;
#endif

static void getPacketVersionGet(uint8_t * data, size_t len) {
    UNUSED(data); UNUSED(len);

    PacketVersionSend reply;
    reply.client          = 'B';
    reply.major           = BSMAJOR;
    reply.minor           = BSMINOR;
    reply.revision        = BSPATCH;
    reply.operatingsystem = operatingsystem;

    /*
      OpenSpades documentation states that ‘Operating System Name contains a human-readable ASCII
      encoded string, and does NOT end with a null character’, so we skip the final NULL simply
      by subtracting 1 from the `sizeof(...)`.

      https://github.com/rzrn/tigerspades/pull/2
    */
    if (settings.report_client_version || HACKS_ENABLED)
        sendPacketVersionSend(&reply, sizeof(operatingsystem) - 1);
}

static void getPacketPlayerProperties(uint8_t * data, size_t len) {
    READPACKET(PacketPlayerProperties, p, data, len);

    if (len >= sizePacketPlayerProperties && p.subID == 0) {
        players[p.player_id].ammo          = p.ammo_clip;
        players[p.player_id].ammo_reserved = p.ammo_reserved;
        players[p.player_id].score         = p.score;

        if (p.player_id == local_player.id) {
            local_player.health        = p.health;
            local_player.blocks        = p.blocks;
            local_player.grenades      = p.grenades;
            local_player.ammo          = p.ammo_clip;
            local_player.ammo_reserved = p.ammo_reserved;
        }
    }
}

static void getPacketBulletTrace(uint8_t * data, size_t len) {
    READPACKET(PacketBulletTrace, p, data, len);

    if (projectiles.head == NULL || projectiles.size == 0 || projectiles.length == 0) return;

    size_t index = p.index % projectiles.size;

    Trajectory * t = (Trajectory *) (projectiles.head + index * WIDTH(projectiles));

    if (p.origin) { t->index = p.index; t->begin = t->end = 0; }
    if (t->index != p.index) return;

    t->data[t->end].pos   = ntohv3f(p.pos);
    t->data[t->end].value = p.value;

    NEXT(t->end, projectiles);

    if (t->begin == t->end) NEXT(t->begin, projectiles);
}

static void getPacketHitEffect(uint8_t * data, size_t len) {
    READPACKET(PacketHitEffect, p, data, len);

    Vector3f r = ntohv3f(p.pos);
    Vector3i b = ntohv3i(p.block);

    if (p.target != HITEFFECT_BLOCK) {
        WAV * wav = sound(p.target == HITEFFECT_HEADSHOT ? SOUND_SPADE_WHACK : SOUND_HITPLAYER);
        sound_create(SOUND_WORLD, wav, r.x, r.y, r.z);
    } else if (b.y > 0) map_damage(b.x, b.y, b.z, 15);

    if (p.target == HITEFFECT_BLOCK)
        particle_create(map_get(b.x, b.y, b.z), r.x, r.y, r.z, 2.5F, 1.0F, 4, 0.1F, 0.25F);
    else
        particle_create(Red, r.x, r.y, r.z, 3.5F, 1.0F, 8, 0.1F, 0.4F);
}

static inline float * dom(Stat * s, size_t i, size_t j)
{ return &s->data[i + s->ncols * j][0]; }

static inline float * cod(Stat * s, size_t i, size_t j)
{ return &s->data[i + s->ncols * j][1]; }

static void stat_alloc(Stat * s, uint8_t index, size_t nrows, size_t ncols) {
    s->index  = index;
    s->nrows  = nrows;
    s->ncols  = ncols;
    s->legend = malloc(nrows * sizeof(LegendEntry));
    s->data   = malloc(nrows * ncols * sizeof(float2));
    s->next   = NULL;

    for (size_t i = 0; i < ncols; i++) {
        float t = (float) i / (float) (ncols - 1);

        for (size_t j = 0; j < nrows; j++) {
            *dom(s, i, j) = t;
            *cod(s, i, j) = -FLT_MAX;
        }
    }
}

static void stat_free(Stat * s) {
    free(s->legend);
    free(s->data);
}

static Stat * stat_try_emplace(Stat ** s, size_t index) {
    Stat * prev = NULL, * curr;

    for (curr = *s; curr != NULL; prev = curr, curr = curr->next)
        if (curr->index == index)
            break;

    if (curr == NULL) {
        curr = malloc(sizeof(Stat));

        if (prev == NULL)
            *s = curr;
        else
            prev->next = curr;
    } else {
        Stat * next = curr->next;

        stat_free(curr);
        curr->next = next;
    }

    return curr;
}

static Stat * stat_find(Stat * s, size_t index) {
    for (Stat * curr = s; curr != NULL; curr = curr->next)
        if (curr->index == index)
            return curr;

    return NULL;
}

static Stat * stat_remove(Stat ** s, size_t index) {
    Stat * prev = NULL, * curr;

    for (curr = *s; curr != NULL; prev = curr, curr = curr->next)
        if (curr->index == index)
            break;

    if (curr == NULL) return NULL;

    if (prev == NULL)
        *s = curr->next;
    else
        prev->next = curr->next;

    return curr;
}

static void getPacketStatNew(uint8_t * data, size_t len) {
    PacketStatNew p; size_t offset = readPacketStatNew(data, &p);
    if (len % sizePacketStatNewEntry != 0) ERRLEN(PacketStatNewEntry, len);

    size_t nrows = len / sizePacketStatNewEntry;

    Stat * s = stat_try_emplace(&hud_custom_stat, p.index);
    stat_alloc(s, p.index, nrows, p.length);

    for (size_t j = 0; j < s->nrows; j++) {
        PacketStatNewEntry e; offset += readPacketStatNewEntry(data + offset, &e);

        LegendEntry * p = &s->legend[j];
        decodeMagic(p->label, sizeof(p->label), (char *) e.label.data, e.label.size);

        p->color = RGB3iAs3f(e.color);
    }
}

static void getPacketStatData(uint8_t * data, size_t len) {
    PacketStatData p; size_t offset = readPacketStatData(data, &p);
    if (len % sizePacketStatDataEntry != 0) ERRLEN(PacketStatDataEntry, len);

    size_t nrows = len / sizePacketStatDataEntry;

    Stat * s = stat_find(hud_custom_stat, p.index);

    if (s == NULL) {
        log_error("Invalid statistics display ID (%d) was received.", p.index);
        return;
    }

    if (s->nrows != nrows) ERRLEN(PacketStatDataEntry, len);

    for (size_t i = 0; i + 1 < s->ncols; i++)
        for (size_t j = 0; j < s->nrows; j++)
            *cod(s, i, j) = *cod(s, i + 1, j);

    for (size_t j = 0; j < s->nrows; j++) {
        PacketStatDataEntry e; offset += readPacketStatDataEntry(data + offset, &e);
        *cod(s, s->ncols - 1, j) = clamp(-FLT_MAX, FLT_MAX, e.value);
    }
}

static void getPacketStatDel(uint8_t * data, size_t len) {
    UNUSED(len);

    PacketStatDel p; readPacketStatDel(data, &p);

    Stat * s = stat_remove(&hud_custom_stat, p.index);

    if (s == NULL) {
        log_error("Invalid statistics display ID (%d) was received.", p.index);
    } else {
        stat_free(s); free(s);
    }
}

static void getPacketStat(uint8_t * data, size_t len) {
    if (len < sizePacketStat) ERRLEN(PacketStat, len);
    PacketStat p; size_t offset = readPacketStat(data, &p);

    len -= sizePacketStat;

    switch (p.subID) {
        case subIdPacketStatNew: {
            if (len < sizePacketStatNew) ERRLEN(PacketStatNew, len);
            getPacketStatNew(data + offset, len - sizePacketStatNew);
            break;
        }

        case subIdPacketStatData: {
            if (len < sizePacketStatData) ERRLEN(PacketStatData, len);
            getPacketStatData(data + offset, len - sizePacketStatData);
            break;
        }

        case subIdPacketStatDel: {
            if (len < sizePacketStatDel) ERRLEN(PacketStatDel, len);
            getPacketStatDel(data + offset, len - sizePacketStatDel);
            break;
        }

        default: {
            log_error("Invalid subID of PackedStat (%d) was received.", p.subID);
            break;
        }
    }
}

static void getPacketDamageMarker(uint8_t * data, size_t len) {
    READPACKET(PacketDamageMarker, p, data, len);

    if (settings.damage_markers)
        game_damage_marker_add(p.player_id, p.hit_amount);
}

void updateBlockColor(void) {
    PacketSetColor contained;
    contained.player_id = local_player.id;
    contained.color     = players[local_player.id].block;

    sendPacketSetColor(&contained, 0);
}

void updateInputData(void) {
    PacketInputData contained;
    contained.player_id = local_player.id;
    contained.keys      = players[local_player.id].input.keys;

    sendPacketInputData(&contained, 0);

    network_keys_last = players[local_player.id].input.keys;
}

unsigned int network_ping(void) { return peer != NULL ? peer->roundTripTime : 0; }

float network_packet_loss(void) { return peer != NULL ? peer->packetLoss / (float) ENET_PEER_PACKET_LOSS_SCALE : 0.0F; }

static inline int network_destroy(void) {
    network_connected    = false;
    network_map_transfer = false;
    network_logged_in    = false;

    if (peer != NULL) { enet_peer_reset(peer); peer = NULL; }
    if (client != NULL) { enet_host_destroy(client); client = NULL; }

    for (size_t k = 0; k < PLAYERS_MAX; k++)
        players[k].connected = false;

    players[local_player.id].team  = TEAM1;
    players[local_player.id].alive = true;

    screen_current = SCREEN_NONE;
    camera.mode = CAMERAMODE_FPS;

    return 0;
}

void network_disconnect(void) {
    if (peer != NULL) {
        network_map_transfer = false;
        network_logged_in    = false;

        enet_peer_disconnect(peer, 0);

        ENetEvent event;
        while (enet_host_service(client, &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: enet_packet_destroy(event.packet); break;
                case ENET_EVENT_TYPE_DISCONNECT: goto fin;
                default: break;
            }
        }

        fin: network_destroy();
    }
}

int network_connect_sub(char * ip, int port, ProtocolVersion version) {
    network_map_transfer = false;
    network_logged_in    = false;

    ENetAddress address;

    client = enet_host_create(NULL, 1, 1, 0, 0); // limit bandwidth here if you want to
    if (client == NULL) return network_destroy();

    enet_host_compress_with_range_coder(client);

    enet_address_set_host(&address, ip); address.port = port;

    peer = enet_host_connect(client, &address, 1, version);
    if (peer == NULL) return network_destroy();

    hud_serverlist_popup = NULL;
    network_custom_reason[0] = 0;

    memset(network_stats, 0, sizeof(NetworkStat) * 40);

    connection_version = version;
    connection_timestamp = window_time();

    return 1;
}

const char * get_version_name(GameVersion version) {
    switch (version) {
        case VER075: return "0.75";
        case VER076: return "0.76";
        default:     return "0.7X";
    }
}

static Address network_address = {.ip = "127.0.0.1", .port = 32887, .version = VER07X};

int network_connect(Address * addr) {
    if (addr == NULL)
        addr = &network_address; // Reuse previously saved address
    else
        network_address = *addr; // Save new address to reuse it later

    hud_change(&hud_mapload);

    log_info("Connecting to %s at port %i (protocol version %s)", addr->ip, addr->port, get_version_name(addr->version));
    if (peer != NULL) network_disconnect();

    return network_connect_sub(addr->ip, addr->port,
        addr->version == VER075 ? VERSION_075 :
        addr->version == VER076 ? VERSION_076 :
        addr->version == VER07X ? VERSION_075 :
                                  VERSION_075
    );
}

int network_identifier_split(const char * str, Address * addr) {
    while (*str && isspace(*str)) str++; // skip trailing whitespace

    if (strstr(str, "aos://") != str) return 0;
    str += 6; // skip that “aos://” prefix

    char * colon = strchr(str, ':');
    addr->port = colon ? strtoul(colon + 1, NULL, 10) : 32887;

    size_t len = strlen(str), iplen = colon ? colon - str : len;

    if (memchr(str, '.', iplen)) {
        strncpy(addr->ip, str, iplen);
        addr->ip[iplen] = 0;
    } else {
        unsigned int ip = strtoul(str, NULL, 10);
        sprintf(addr->ip, "%i.%i.%i.%i", ip & 255, (ip >> 8) & 255, (ip >> 16) & 255, (ip >> 24) & 255);
    }

    addr->version = strcmp(str + len - 5, ":0.75") == 0 ? VER075 :
                    strcmp(str + len - 5, ":0.76") == 0 ? VER076 :
                                                          VER07X;

    return 1;
}

int network_connect_string(const char * str, GameVersion version) {
    Address addr;

    if (!network_identifier_split(str, &addr))
        return 0;

    if (version != VER07X) addr.version = version;

    return network_connect(&addr);
}

#define CONNECTION_TIMEOUT 2.5F

int network_update(void) {
    if (peer != NULL) {
        if (!network_connected && CONNECTION_TIMEOUT <= window_time() - connection_timestamp) {
            network_destroy();

            log_error("Disconnected: connection timed out");

            hud_serverlist_popup = "No response";
            hud_change(&hud_serverlist);

            return 0;
        }

        if (window_time() - network_stats_last >= 1.0F) {
            for (int k = 39; k > 0; k--)
                network_stats[k] = network_stats[k - 1];

            network_stats[0].ingoing  = 0;
            network_stats[0].outgoing = 0;
            network_stats[0].avg_ping = network_ping();
            network_stats_last        = window_time();
        }

        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE: {
                    network_stats[0].ingoing += event.packet->dataLength;
                    int id = event.packet->data[0];

                    if (*packets[id]) {
                        log_debug("Packet id %i", id);
                        (*packets[id])(event.packet->data + 1, event.packet->dataLength - 1);
                    } else {
                        log_error("Invalid packet id %i, length: %i", id, (int) event.packet->dataLength - 1);
                    }

                    network_received_packets++;
                    enet_packet_destroy(event.packet);
                    break;
                }

                case ENET_EVENT_TYPE_CONNECT: {
                    network_received_packets = 0;
                    network_connected        = true;
                    local_hit_effects        = true;

                    break;
                }

                case ENET_EVENT_TYPE_DISCONNECT: {
                    network_destroy();

                    if (event.data == ERROR_WRONG_PROTOCOL && network_address.version == VER07X) {
                        if (network_connect_sub(network_address.ip, network_address.port, VERSION_076)) // retry
                            goto exit;
                    }

                    log_error("Disconnected: %s", network_reason_disconnect(event.data));

                    hud_change(&hud_serverlist);
                    hud_serverlist_popup = network_reason_disconnect(event.data);

                    exit: return 0;
                }

                default: break;
            }
        }

        if (network_logged_in && players[local_player.id].team != TEAM_SPECTATOR && players[local_player.id].alive) {
            if (players[local_player.id].input.keys != network_keys_last)
                updateInputData();

            if ((players[local_player.id].input.buttons != network_buttons_last) &&
               !HASBIT(players[local_player.id].input.keys, INPUT_SPRINT)) {
                PacketWeaponInput contained;
                contained.player_id = local_player.id;
                contained.input     = players[local_player.id].input.buttons;
                sendPacketWeaponInput(&contained, 0);

                network_buttons_last = players[local_player.id].input.buttons;
            }

            if (players[local_player.id].tool != network_tool_last) {
                PacketSetTool contained;
                contained.player_id = local_player.id;
                contained.tool      = players[local_player.id].tool;
                sendPacketSetTool(&contained, 0);

                network_tool_last = players[local_player.id].tool;
            }

            if (window_time() - network_pos_update > 1.0F
               && norm3f(network_pos_last.x, network_pos_last.y, network_pos_last.z,
                         players[local_player.id].pos.x,
                         players[local_player.id].pos.y,
                         players[local_player.id].pos.z) > 0.01F) {
                network_pos_update = window_time();
                network_pos_last   = players[local_player.id].pos;

                PacketPositionData contained;
                contained.pos = htonv3f(players[local_player.id].pos);
                sendPacketPositionData(&contained, 0);
            }

            if (window_time() - network_orient_update > (1.0F / 120.0F)
               && angle3f(network_orient_last.x, network_orient_last.y, network_orient_last.z,
                          players[local_player.id].orientation.x,
                          players[local_player.id].orientation.y,
                          players[local_player.id].orientation.z)
                   > 0.5F / 180.0F * PI) {
                network_orient_update = window_time();
                network_orient_last   = players[local_player.id].orientation;

                PacketOrientationData contained;
                contained.orient = htonov3f(players[local_player.id].orientation);
                sendPacketOrientationData(&contained, 0);
            }
        }
    }

    chunk_queue_blocks();

    return 1;
}

void network_init(void) {
    enet_initialize();

    packets[idPacketPositionData]                  = getPacketPositionData;
    packets[idPacketOrientationData]               = getPacketOrientationData;
    packets[idPacketInputData]                     = getPacketInputData;
    packets[idPacketWeaponInput]                   = getPacketWeaponInput;
    packets[idPacketGrenade]                       = getPacketGrenade;
    packets[idPacketSetTool]                       = getPacketSetTool;
    packets[idPacketSetColor]                      = getPacketSetColor;
    packets[idPacketExistingPlayer]                = getPacketExistingPlayer;
    packets[idPacketBlockAction]                   = getPacketBlockAction;
    packets[idPacketBlockLine]                     = getPacketBlockLine;
    packets[idPacketChatMessage]                   = getPacketChatMessage;
    packets[idPacketExtInfo]                       = getPacketExtInfo;

    packets[idPacketWorldUpdate]                   = getPacketWorldUpdate;
    packets[idPacketSetHP]                         = getPacketSetHP;
    packets[idPacketShortPlayerData]               = getPacketShortPlayerData;
    packets[idPacketMoveObject]                    = getPacketMoveObject;
    packets[idPacketCreatePlayer]                  = getPacketCreatePlayer;
    packets[idPacketStateData]                     = getPacketStateData;
    packets[idPacketKillAction]                    = getPacketKillAction;
    packets[idPacketMapStart]                      = getPacketMapStart;
    packets[idPacketMapChunk]                      = getPacketMapChunk;
    packets[idPacketPlayerLeft]                    = getPacketPlayerLeft;
    packets[idPacketTerritoryCapture]              = getPacketTerritoryCapture;
    packets[idPacketProgressBar]                   = getPacketProgressBar;
    packets[idPacketIntelCapture]                  = getPacketIntelCapture;
    packets[idPacketIntelPickup]                   = getPacketIntelPickup;
    packets[idPacketIntelDrop]                     = getPacketIntelDrop;
    packets[idPacketRestock]                       = getPacketRestock;
    packets[idPacketFogColor]                      = getPacketFogColor;
    packets[idPacketWeaponReload]                  = getPacketWeaponReload;
    packets[idPacketChangeWeapon]                  = getPacketChangeWeapon;
    packets[idPacketHandshakeInit]                 = getPacketHandshakeInit;
    packets[idPacketVersionGet]                    = getPacketVersionGet;

    packets[PACKET_EXT_BASE + EXT_PLAYER_PROPERTIES] = getPacketPlayerProperties;
    packets[PACKET_EXT_BASE + EXT_TRACE_BULLETS]     = getPacketBulletTrace;
    packets[PACKET_EXT_BASE + EXT_HIT_EFFECTS]       = getPacketHitEffect;
    packets[PACKET_EXT_BASE + EXT_CUSTOM_STAT]       = getPacketStat;
    packets[PACKET_EXT_BASE + EXT_DAMAGE_MARKERS]    = getPacketDamageMarker;
}
