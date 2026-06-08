/*******************************************************
                  MQUEUE.H
   Multi-server simulator with N independent FIFO queues
   and configurable routing policy. Replaces the single
   server "queue" class of the original template.
*******************************************************/

#ifndef _MQUEUE_H
#define _MQUEUE_H

#include "simulator.h"
#include "calendar.h"
#include "event.h"
#include "buffer.h"
#include "packet.h"
#include "stat.h"

class mqueue: public simulator {

    virtual void input(void);

    int        traffic_model;
    int        service_model;

    // Per-run statistics
    Sstat*     delay;       // average sojourn time (system-wide)
    Sstat**    length;      // average station occupancy, one per server
    Sstat*     imbalance;   // max length - min length across servers

public:
    mqueue(int argc, char *argv[]);
    virtual ~mqueue(void);
    virtual void init(void);
    virtual void run(void);

private:
    virtual void clear_counters(double clk);
    virtual void clear_stats(void);
    virtual void update_stats(double run_len);
    virtual void print_trace(int Run);
    virtual void results(void);
    virtual int  isconfsatisf(double perc);
};

#endif
