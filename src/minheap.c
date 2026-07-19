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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdint.h>

#include <bs/minheap.h>

static void nodes_swap(Minheap * h, int a, int b) {
    MinheapBlock tmp;
    tmp = h->nodes[a];
    h->nodes[a] = h->nodes[b];
    h->nodes[b] = tmp;
}

void minheap_create(Minheap * h) {
    h->index = 0;
    h->length = 256;
    h->nodes = malloc(sizeof(MinheapBlock) * h->length);
}

void minheap_clear(Minheap * h) {
    h->index = 0;
    h->length = 256;
    h->nodes = realloc(h->nodes, sizeof(MinheapBlock) * h->length);
}

void minheap_destroy(Minheap * h) {
    free(h->nodes);
}

int minheap_isempty(Minheap * h) {
    return h->index <= 0;
}

MinheapBlock minheap_extract(Minheap * h) {
    MinheapBlock min = h->nodes[0];

    h->nodes[0] = h->nodes[--h->index];

    // now heapify at root node
    int k = 0;
    while (1) {
        int smallest = k;
        if (k * 2 + 1 < h->index
           && pos_keyy(h->nodes[k * 2 + 1].pos)
               < pos_keyy(h->nodes[smallest].pos)) // does left child exist and is less than parent?
            smallest = k * 2 + 1;
        if (k * 2 + 2 < h->index
           && pos_keyy(h->nodes[k * 2 + 2].pos)
               < pos_keyy(h->nodes[smallest].pos)) // does right child exist and is less than parent?
            smallest = k * 2 + 2;
        if (smallest == k) // parent is smallest, finished!
            break;
        nodes_swap(h, k, smallest);
        k = smallest;
    }

    return min;
}

MinheapBlock * minheap_put(Minheap * h, MinheapBlock * b) {
    if (h->index >= h->length) { // grow buffer
        h->length *= 2;
        h->nodes = realloc(h->nodes, sizeof(MinheapBlock) * h->length);
    }

    h->nodes[h->index++] = *b; // place new node at end of heap

    int k = h->index - 1;
    while (k > 0) {
        if (pos_keyy(h->nodes[k].pos) < pos_keyy(h->nodes[(k - 1) / 2].pos)) { // is child less than parent?
            nodes_swap(h, k, (k - 1) / 2);
            k = (k - 1) / 2; // continue at parent
        } else {
            break;
        }
    }

    return h->nodes + k;
}
