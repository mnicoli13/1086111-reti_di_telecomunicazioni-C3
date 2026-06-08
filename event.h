/*******************************************************
                     EVENT.H
   Two event classes. arrival schedules the next arrival
   and dispatches the new packet to one of the N stations
   according to the routing policy. service handles the
   completion of a packet on a specific server.
*******************************************************/

#ifndef _EVENT_H
#define _EVENT_H

#include "global.h"
#include "buffer.h"

class event {
public:
    event*  next;   // next event in the calendar
    double  time;   // event time

    event();
    event(double Time);
    event(event* Next, double Time);
    ~event(){}

    virtual void body() {}
};

inline event::event(){
    next = NULL;
    time = -1;
}

inline event::event(event* Next, double Time){
    next = Next;
    time = Time;
}

inline event::event(double Time){
    time = Time;
}

class arrival: public event {
public:
    int source_id;
    virtual void body();
    arrival(double Time);
};

class service: public event {
    int srv_idx;          // index of the server completing the service
public:
    virtual void body();
    service(double Time, int idx): event(Time) { srv_idx = idx; }
};

inline arrival::arrival(double Time): event(Time) {}

#endif
