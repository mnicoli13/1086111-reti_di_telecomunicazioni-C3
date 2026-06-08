/***************************************************************************
                                BUFFER.C
***************************************************************************/

#include "buffer.h"

buffer::buffer() {
    head        = NULL;
    last        = NULL;
    size        = 0;
    status      = 0;
    in_service  = NULL;
    tot_delay   = 0.0;
    tot_packs   = 0.0;
    area        = 0.0;
    last_update = 0.0;
}

void buffer::insert(packet* pack) {
    if (head == NULL) {
        head = pack;
        last = pack;
        last->next = head;
    } else {
        last->next = pack;
        last = pack;
        last->next = head;
    }
}

packet* buffer::get() {
    packet* pack;
    if (head == NULL) return NULL;
    if (last == head) {
        pack = head;
        last = NULL;
        head = NULL;
    } else {
        pack = head;
        head = head->next;
        last->next = head;
    }
    return pack;
}

void buffer::update_area(double t) {
    area += (double)station() * (t - last_update);
    last_update = t;
}

void buffer::reset_counters(double t) {
    update_area(t);
    area        = 0.0;
    last_update = t;
    tot_delay   = 0.0;
    tot_packs   = 0.0;
}
