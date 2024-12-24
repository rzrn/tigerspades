#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <ace/types.h>

typedef enum {
    VERSION_075 = 3,
    VERSION_076 = 4
} ProtocolVersion;

typedef enum {
    // Client <-> Server
    idPacketPositionData     = 0,
    idPacketOrientationData  = 1,
    idPacketInputData        = 3,
    idPacketWeaponInput      = 4,
    idPacketGrenade          = 6,
    idPacketSetTool          = 7,
    idPacketSetColor         = 8,
    idPacketExistingPlayer   = 9,
    idPacketBlockAction      = 13,
    idPacketBlockLine        = 14,
    idPacketChatMessage      = 17,
    idPacketWeaponReload     = 28,
    idPacketChangeWeapon     = 30,
    idPacketExtInfo          = 60,

    // Client <- Server
    idPacketWorldUpdate      = 2,
    idPacketWorldUpdate075   = idPacketWorldUpdate,
    idPacketWorldUpdate076   = idPacketWorldUpdate,
    idPacketSetHP            = 5,
    idPacketShortPlayerData  = 10,
    idPacketMoveObject       = 11,
    idPacketCreatePlayer     = 12,
    idPacketStateData        = 15,
    idPacketKillAction       = 16,
    idPacketMapStart         = 18,
    idPacketMapStart075      = idPacketMapStart,
    idPacketMapStart076      = idPacketMapStart,
    idPacketMapChunk         = 19,
    idPacketPlayerLeft       = 20,
    idPacketTerritoryCapture = 21,
    idPacketProgressBar      = 22,
    idPacketIntelCapture     = 23,
    idPacketIntelPickup      = 24,
    idPacketIntelDrop        = 25,
    idPacketRestock          = 26,
    idPacketFogColor         = 27,
    idPacketHandshakeInit    = 31,
    idPacketVersionGet       = 33,

    // Client -> Server
    idPacketHit              = 5,
    idPacketChangeTeam       = 29,
    idPacketMapCached        = 31,
    idPacketHandshakeReturn  = 32,
    idPacketVersionSend      = 34,
} PacketId;

typedef enum {
    ERROR_UNDEFINED            = 0,
    ERROR_BANNED               = 1,
    ERROR_TOO_MANY_CONNECTIONS = 2,
    ERROR_WRONG_PROTOCOL       = 3,
    ERROR_FULL                 = 4,
    ERROR_SHUTDOWN             = 5,
    ERROR_KICKED               = 10,
    ERROR_INVALID_NICKNAME     = 20,
} ErrorCode;

#define PLAYERS_MAX 256 // just because 32 players are not enough

#if PLAYERS_MAX < 256
    #define IDVALID(id) ((id) < PLAYERS_MAX)
#else
    #define IDVALID(id) true
#endif

typedef enum {
    GAMEMODE_CTF = 0,
    GAMEMODE_TC  = 1
} GameMode;

enum {
    TEAM1          = 0,
    TEAM2          = 1,
    TEAM_SPECTATOR = 255
};

#define TEAM(t) (((t) == TEAM1 || (t) == TEAM2) ? (t) : TEAM_SPECTATOR)

enum {
    INPUT_UP     = 0,
    INPUT_DOWN   = 1,
    INPUT_LEFT   = 2,
    INPUT_RIGHT  = 3,
    INPUT_JUMP   = 4,
    INPUT_CROUCH = 5,
    INPUT_SNEAK  = 6,
    INPUT_SPRINT = 7
};

enum {
    BUTTON_PRIMARY   = 0,
    BUTTON_SECONDARY = 1
};

typedef enum {
    TEAM1_FLAG = 0,
    TEAM2_FLAG = 1,
    TEAM1_BASE = 2,
    TEAM2_BASE = 3
} Object;

typedef enum {
    DAMAGE_SOURCE_FALL = 0,
    DAMAGE_SOURCE_GUN  = 1
} DamageSource;

typedef enum {
    HITTYPE_TORSO = 0,
    HITTYPE_HEAD  = 1,
    HITTYPE_ARMS  = 2,
    HITTYPE_LEGS  = 3,
    HITTYPE_SPADE = 4
} HitType;

typedef enum {
    KILLTYPE_WEAPON      = 0,
    KILLTYPE_HEADSHOT    = 1,
    KILLTYPE_MELEE       = 2,
    KILLTYPE_GRENADE     = 3,
    KILLTYPE_FALL        = 4,
    KILLTYPE_TEAMCHANGE  = 5,
    KILLTYPE_CLASSCHANGE = 6,
    KILLTYPE_MIN         = KILLTYPE_WEAPON,
    KILLTYPE_MAX         = KILLTYPE_CLASSCHANGE
} KillType;

#define KILLTYPEVALID(x) ((x) <= KILLTYPE_MAX)

typedef enum {
    WEAPON_RIFLE   = 0,
    WEAPON_SMG     = 1,
    WEAPON_SHOTGUN = 2,
    WEAPON_MIN     = WEAPON_RIFLE,
    WEAPON_MAX     = WEAPON_SHOTGUN,
    WEAPON_DEFAULT = WEAPON_RIFLE
} Weapon;

#define WEAPON(w) ((w) > WEAPON_MAX ? WEAPON_DEFAULT : (w))

typedef enum {
    TOOL_SPADE   = 0,
    TOOL_BLOCK   = 1,
    TOOL_GUN     = 2,
    TOOL_GRENADE = 3,
    TOOL_MIN     = TOOL_SPADE,
    TOOL_MAX     = TOOL_GRENADE,
    TOOL_DEFAULT = TOOL_GUN
} Tool;

#define TOOL(t) ((t) > TOOL_MAX ? TOOL_DEFAULT : (t))

typedef enum {
    ACTION_BUILD   = 0,
    ACTION_DESTROY = 1,
    ACTION_SPADE   = 2,
    ACTION_GRENADE = 3
} ActionType;

typedef enum {
    CHAT_ALL     = 0,
    CHAT_TEAM    = 1,
    CHAT_SYSTEM  = 2,
    CHAT_BIG     = 3,
    CHAT_INFO    = 4,
    CHAT_WARNING = 5,
    CHAT_ERROR   = 6
} MessageType;

enum {
    TEAM1_HAS_INTEL = 0,
    TEAM2_HAS_INTEL = 1
};

enum {
    PACKET_EXT_BASE = 0x40,
};

enum Extension {
    EXT_PLAYER_PROPERTIES = 0x00,
    EXT_TRACE_BULLETS     = 0x10,
    EXT_HIT_EFFECTS       = 0x11,
    EXT_256PLAYERS        = 0xC0,
    EXT_MESSAGES          = 0xC1,
    EXT_KICKREASON        = 0xC2,
};

enum HitEffectTarget {
    HITEFFECT_BLOCK    = 0,
    HITEFFECT_HEADSHOT = 1,
    HITEFFECT_PLAYER   = 2,
};

#define begin(T) typedef struct _##T T; struct _##T {
#define end() };

#define c8(dest)      char dest;
#define u8(dest)      uint8_t dest;
#define u16(dest)     uint16_t dest;
#define u32(dest)     uint32_t dest;
#define f32(dest)     float dest;
#define v3f(dest)     Vector3f dest;
#define v3i(dest)     Vector3i dest;
#define bgr(dest)     RGB3i dest;
#define string(dest)  char * dest;
#define blob(dest, n) Blob dest;

#include <ace/packets.h>

#define begin(T) extern size_t read##T(uint8_t * buff, T *);
#include <ace/packets.h>

#define begin(T) extern size_t write##T(uint8_t * buff, T *);
#include <ace/packets.h>

#define begin(T) extern const size_t size##T;
#include <ace/packets.h>

#endif
