/***************************************************************************
                                BUFFER.H
   FIFO buffer of one server. Tracks the time integral of the number of
   packets at the station (queue + in-service) so that the time-weighted
   mean queue length can be computed.
***************************************************************************/

#ifndef BUFFER_H
#define BUFFER_H

#include "packet.h"

class buffer {

    packet* head;
    packet* last;

public:
    int      size;          // packets currently waiting in line
    int      status;        // 1 if the server is busy, 0 otherwise
    packet*  in_service;    // packet currently being served (NULL if idle)

    // counters for the current run
    double   tot_delay;     // sum of sojourn times of completed packets
    double   tot_packs;     // number of completed packets
    double   area;          // integral of (size+status) over time
    double   last_update;   // last time the integral was updated

    buffer();
    ~buffer(){}

    void     insert(packet* pack);   // enqueue
    packet*  get();                  // dequeue (NULL if empty)
    packet*  full(){ return head; }  // !=NULL if queue non empty

    int      station() const { return size + status; }
    void     update_area(double t);  // accumulate area up to t
    void     reset_counters(double t);
};

#endif
