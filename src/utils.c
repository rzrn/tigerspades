/*
    Copyright © 2017–2020 ByteBit

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

#include <assert.h>

#include <bs/common.h>
#include <bs/utils.h>

static int base64_map(char c) {
    if (c >= '0' && c <= '9')
        return c + 4;
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return 0;
}

// works in place
size_t base64_decode(char * data, size_t size) {
    assert(data != NULL && size > 0);

    int buff = 0; size_t buffbits = 0;
    size_t offsrc = 0, offdst = 0;

    for (size_t i = 0; i < size; i++) {
        if (buffbits + 6 < 8 * sizeof(buff) && data[offsrc] != '=') {
            buff <<= 6;
            buff |= base64_map(data[offsrc++]);

            buffbits += 6;
        }

        for (size_t j = 0; j < buffbits / 8; j++) {
            data[offdst++] = buff >> (buffbits - 8);
            buffbits -= 8;
        }
    }

    return offdst;
}

int int_cmp(void * first_key, void * second_key, size_t key_size) {
    UNUSED(key_size);

    assert(first_key && second_key);

    return (*(uint32_t *) first_key) != (*(uint32_t *) second_key);
}

size_t int_hash(void * raw_key, size_t key_size) {
    UNUSED(key_size);

    assert(raw_key);

    uint32_t x = *(uint32_t *) raw_key;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

void ht_iterate_remove(HashTable * ht, void * user, bool (*callback)(void * key, void * value, void * user)) {
    assert(ht && callback);

    bool removed = false;

    for (size_t chain = 0; chain < ht->capacity; chain++) {
        HTNode * node = ht->nodes[chain];
        HTNode * prev = NULL;

        while (node) {
            if (callback(node->key, node->value, user)) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    ht->nodes[chain] = node->next;
                }

                HTNode * del = node;
                node = node->next;

                _ht_destroy_node(del);

                ht->size--;
                removed = true;
            } else {
                prev = node;
                node = node->next;
            }
        }
    }

    if (removed && _ht_should_shrink(ht))
        _ht_adjust_capacity(ht);
}

bool ht_iterate(HashTable * ht, void * user, bool (*callback)(void * key, void * value, void * user)) {
    assert(ht && callback);

    for (size_t chain = 0; chain < ht->capacity; chain++) {
        HTNode * node = ht->nodes[chain];

        while (node) {
            if (!callback(node->key, node->value, user))
                return true;
            node = node->next;
        }
    }

    return false;
}
