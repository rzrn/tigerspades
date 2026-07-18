/*
    Copyright © 2024–2026 rzrn

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Client <-> Server
#if PACKET_CLIENTSIDE || PACKET_SERVERSIDE
    begin(PacketPositionData)
        v3f(pos)
    end()

    begin(PacketOrientationData)
        v3f(orient)
    end()

    begin(PacketInputData)
        u8(player_id)
        u8(keys)
    end()

    begin(PacketWeaponInput)
        u8(player_id)
        u8(input)
    end()

    begin(PacketGrenade)
        u8(player_id)
        f32(fuse_length)
        v3f(pos)
        v3f(vel)
    end()

    begin(PacketSetTool)
        u8(player_id)
        u8(tool)
    end()

    begin(PacketSetColor)
        u8(player_id)
        bgr(color)
    end()

    begin(PacketExistingPlayer)
        u8(player_id)
        u8(team)
        u8(weapon)
        u8(tool)
        u32(kills)
        bgr(color)
        string(name)
    end()

    begin(PacketBlockAction)
        u8(player_id)
        u8(action_type)
        v3i(pos)
    end()

    begin(PacketBlockLine)
        u8(player_id)
        v3i(start)
        v3i(end)
    end()

    begin(PacketChatMessage)
        u8(player_id)
        u8(chat_type)
        string(message)
    end()

    begin(PacketWeaponReload)
        u8(player_id)
        u8(ammo)
        u8(reserved)
    end()

    begin(PacketChangeWeapon)
        u8(player_id)
        u8(weapon)
    end()

    #if PACKET_INCOMPLETE
        begin(PacketExtInfoEntry)
            u8(id)
            u8(version)
        end()
    #endif

    begin(PacketExtInfo)
        u8(length)
    end()
#endif

// Client <- Server
#if PACKET_SERVERSIDE
    begin(PacketWorldUpdate075)
        v3f(pos)
        v3f(orient)
    end()

    begin(PacketWorldUpdate076)
        u8(player_id)
        v3f(pos)
        v3f(orient)
    end()

    begin(PacketSetHP)
        u8(hp)
        u8(type)
        v3f(pos)
    end()

    begin(PacketShortPlayerData)
        u8(player_id)
        u8(team)
        u8(weapon)
    end()

    begin(PacketMoveObject)
        u8(object_id)
        u8(team)
        v3f(pos)
    end()

    begin(PacketCreatePlayer)
        u8(player_id)
        u8(weapon)
        u8(team)
        v3f(pos)
        string(name)
    end()

    begin(PacketStateData)
        u8(player_id)
        bgr(fog)
        bgr(team1)
        bgr(team2)
        blob(team1_name, 10)
        blob(team2_name, 10)
        u8(gamemode)
    end()

    #if PACKET_INCOMPLETE
        begin(CTFStateData)
            u8(team1_score)
            u8(team2_score)
            u8(capture_limit)
            u8(intels)
            blob(team1_intel_location, 12)
            blob(team2_intel_location, 12)
            v3f(team1_base)
            v3f(team2_base)
        end()

        begin(TCTerritory)
            v3f(pos)
            u8(team)
        end()

        begin(TCStateData)
            u8(territory_count)
        end()
    #endif

    begin(PacketKillAction)
        u8(player_id)
        u8(killer_id)
        u8(kill_type)
        u8(respawn_time)
    end()

    begin(PacketMapStart075)
        u32(map_size)
    end()

    begin(PacketMapStart076)
        u32(map_size)
        u32(crc32)
        string(map_name)
    end()

    begin(PacketPlayerLeft)
        u8(player_id)
    end()

    begin(PacketTerritoryCapture)
        u8(tent)
        u8(winning)
        u8(team)
    end()

    begin(PacketProgressBar)
        u8(tent)
        u8(team_capturing)
        c8(rate)
        f32(progress)
    end()

    begin(PacketIntelCapture)
        u8(player_id)
        u8(winning)
    end()

    begin(PacketIntelPickup)
        u8(player_id)
    end()

    begin(PacketIntelDrop)
        u8(player_id)
        v3f(pos)
    end()

    begin(PacketRestock)
        u8(player_id)
    end()

    begin(PacketFogColor)
        u8(alpha)
        bgr(color)
    end()

    begin(PacketHandshakeInit)
        u32(challenge)
    end()
#endif

// Client -> Server
#if PACKET_CLIENTSIDE
    begin(PacketHit)
        u8(player_id)
        u8(hit_type)
    end()

    begin(PacketChangeTeam)
        u8(player_id)
        u8(team)
    end()

    begin(PacketMapCached)
        u8(cached)
    end()

    begin(PacketHandshakeReturn)
        u32(challenge)
    end()

    begin(PacketVersionSend)
        c8(client)
        u8(major)
        u8(minor)
        u8(revision)
        string(operatingsystem)
    end()
#endif

// Extra packets
#if PACKET_EXTRA
    begin(PacketPlayerProperties)
        u8(subID)
        u8(player_id)
        u8(health)
        u8(blocks)
        u8(grenades)
        u8(ammo_clip)
        u8(ammo_reserved)
        u32(score)
    end()

    begin(PacketBulletTrace)
        u8(index)
        v3f(pos)
        f32(value)
        u8(origin)
    end()

    begin(PacketHitEffect)
        v3f(pos)
        v3i(block)
        u8(target)
    end()

    begin(PacketStat)
        u8(subID)
    end()

    begin(PacketDamageMarker)
        u8(player_id)
        u8(hit_amount)
    end()

    #if PACKET_INCOMPLETE
        begin(PacketStatNew)
            u8(index)
            u8(length)
        end()

        begin(PacketStatNewEntry)
            bgr(color)
            blob(label, 64)
        end()

        begin(PacketStatData)
            u8(index)
        end()

        begin(PacketStatDataEntry)
            f32(value)
        end()

        begin(PacketStatDel)
            u8(index)
        end()
    #endif
#endif
