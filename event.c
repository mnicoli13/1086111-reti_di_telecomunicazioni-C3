/***********************************************************************
                                EVENT.C
***********************************************************************/

#include "event.h"
#include "buffer.h"
#include "calendar.h"
#include "rand.h"

// Globals defined in mqueue.c
extern calendar*  cal;
extern int        N;
extern int        policy;
extern double*    mu_inv;       // mu_inv[i] = 1 / mu_i  (mean service time of server i)
extern double     inter;        // 1 / lambda (mean inter-arrival time)
extern buffer**   bufs;         // array of N buffer pointers
extern int        rr_next;      // next server to use for round-robin

/* Choose the destination station for a new arrival according
   to the configured routing policy. Ties in "shortest queue"
   are broken by lowest index (deterministic). */
static int pick_server(){
    if (policy == ROUTE_RANDOM) {
        int idx;
        GEN_UNIF(SEED_RT, 0, N-1, idx);
        if (idx < 0)    idx = 0;
        if (idx >= N)   idx = N-1;
        return idx;
    }
    if (policy == ROUTE_RR) {
        int idx = rr_next;
        rr_next = (rr_next + 1) % N;
        return idx;
    }
    // ROUTE_SHORTEST: pick the station with fewest packets (queue + in service).
    // Ties are resolved by reservoir sampling so that each tied server is
    // chosen with equal probability; a deterministic tie-break would otherwise
    // bias the load toward lower-indexed servers.
    int best   = 0;
    int best_n = bufs[0]->station();
    int ties   = 1;
    for (int i = 1; i < N; i++) {
        int n = bufs[i]->station();
        if (n < best_n) {
            best_n = n; best = i; ties = 1;
        } else if (n == best_n) {
            ties++;
            int r;
            GEN_UNIF(SEED_RT, 0, ties - 1, r);
            if (r == 0) best = i;
        }
    }
    return best;
}

void arrival::body() {
    // 1. schedule next external arrival
    double dt;
    GEN_EXP(SEED_ARR, inter, dt);
    cal->put(new arrival(time + dt));

    // 2. route the packet to a station
    int idx     = pick_server();
    buffer* buf = bufs[idx];

    // 3. update station length integral up to "now"
    buf->update_area(time);

    // 4. enqueue or start service
    packet* pack = new packet(time);
    if (buf->status) {
        // server busy: append to the waiting line
        buf->insert(pack);
        buf->size += 1;
    } else {
        // server idle: start service immediately
        buf->in_service = pack;
        buf->status     = 1;
        double st;
        GEN_EXP(SEED_SRV, mu_inv[idx], st);
        cal->put(new service(time + st, idx));
    }
}

void service::body() {
    buffer* buf = bufs[srv_idx];

    // update station length integral up to "now"
    buf->update_area(time);

    // record the sojourn time of the packet just completed
    if (buf->in_service != NULL) {
        buf->tot_delay += time - buf->in_service->get_time();
        buf->tot_packs += 1.0;
        delete buf->in_service;
        buf->in_service = NULL;
    }

    // start service on the next waiting packet, if any
    packet* p = buf->get();
    if (p != NULL) {
        buf->size      -= 1;
        buf->in_service = p;
        double st;
        GEN_EXP(SEED_SRV, mu_inv[srv_idx], st);
        cal->put(new service(time + st, srv_idx));
    } else {
        buf->status = 0;
    }
}
