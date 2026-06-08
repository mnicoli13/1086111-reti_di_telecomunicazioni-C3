/*******************************************************
                    MQUEUE.C
*******************************************************/

#include "global.h"
#include <stdio.h>
#include "mqueue.h"
#include "rand.h"
#include "buffer.h"
#include "event.h"
#include "calendar.h"
#include "easyio.h"

/* ---- globals shared with event.c ----
   Declared "extern" in event.c and defined here. */
calendar*  cal     = NULL;
int        N       = 0;             // number of servers
int        policy  = ROUTE_RANDOM;   // routing policy
double*    mu_inv  = NULL;           // service-time means 1/mu_i
double     inter   = 0.0;            // inter-arrival mean 1/lambda
buffer**   bufs    = NULL;           // N buffers
int        rr_next = 0;              // RR cursor

/* Run-length parameters (read from input). */
static double  Trslen;
static double  Runlen;
static int     NRUNmin;

/* helper: convert the policy code into a readable label */
static const char* policy_name(int p){
    switch(p){
        case ROUTE_RANDOM:   return "random";
        case ROUTE_RR:       return "round-robin";
        case ROUTE_SHORTEST: return "shortest-queue";
    }
    return "?";
}

mqueue::mqueue(int argc, char *argv[]): simulator(argc, argv) {
    cal       = new calendar();
    delay     = new Sstat();
    length    = NULL;
    imbalance = new Sstat();
}

mqueue::~mqueue() {
    if (length != NULL) {
        for (int i = 0; i < N; i++) delete length[i];
        delete[] length;
    }
    if (bufs != NULL) {
        for (int i = 0; i < N; i++) delete bufs[i];
        delete[] bufs;
    }
    if (mu_inv != NULL) delete[] mu_inv;
    delete delay;
    delete imbalance;
    delete cal;
}

void mqueue::input() {
    printf("MODEL PARAMETERS:\n\n");
    printf("\n Arrivals model:\n");
    printf("1 - Poisson:>\n");
    traffic_model = read_int("", 1, 1, 1);

    N = read_int("Number of servers N", 2, 1, MAX_N);

    double lambda = read_double("Arrival rate lambda (pkt/s)", 1.0, 1e-6, 1e6);
    inter = 1.0 / lambda;

    printf("\n Service model:\n");
    printf("1 - Exponential:>\n");
    service_model = read_int("", 1, 1, 1);

    mu_inv = new double[N];
    char prompt[80];
    for (int i = 0; i < N; i++) {
        snprintf(prompt, sizeof(prompt), "Service rate mu_%d (pkt/s)", i + 1);
        double mu = read_double(prompt, 1.0, 1e-6, 1e9);
        mu_inv[i] = 1.0 / mu;
    }

    printf("\n Routing policy:\n");
    printf("  1 - random\n");
    printf("  2 - round-robin\n");
    printf("  3 - shortest queue:>\n");
    policy = read_int("", 1, 1, 3);

    printf("SIMULATION PARAMETERS:\n\n");
    Trslen   = read_double("Simulation transient len (s)", 200, 0.01, 100000);
    Runlen   = read_double("Simulation RUN len (s)",       500, 0.01, 100000);
    NRUNmin  = read_int   ("Simulation number of RUNs",      10,    2,    100);
}

void mqueue::init() {
    input();

    // allocate buffers and per-server length statistics
    bufs   = new buffer*[N];
    length = new Sstat*[N];
    for (int i = 0; i < N; i++) {
        bufs[i]   = new buffer();
        length[i] = new Sstat();
    }
    rr_next = 0;

    // schedule the first arrival at t = 0
    cal->put(new arrival(0.0));
}

void mqueue::run() {
    double clock = 0.0;
    event* ev;

    // --- transient phase ---
    while (clock < Trslen) {
        ev = cal->get();
        ev->body();
        clock = ev->time;
        delete ev;
    }
    clear_stats();
    clear_counters(clock);

    // --- measurement runs ---
    int current_run_number = 1;
    while (current_run_number <= NRUNmin) {
        double run_end = current_run_number * Runlen + Trslen;
        while (clock < run_end) {
            ev = cal->get();
            ev->body();
            clock = ev->time;
            delete ev;
        }
        // flush the length integrals up to the end of the run
        for (int i = 0; i < N; i++) bufs[i]->update_area(run_end);

        update_stats(Runlen);
        print_trace(current_run_number);
        clear_counters(run_end);
        current_run_number++;
    }
}

void mqueue::results() {
    fprintf(fpout, "*********************************************\n");
    fprintf(fpout, "           SIMULATION RESULTS                \n");
    fprintf(fpout, "*********************************************\n\n");
    fprintf(fpout, "Input parameters:\n");
    fprintf(fpout, "  Number of servers N        %d\n",      N);
    fprintf(fpout, "  Arrival rate lambda        %.6f\n",    1.0/inter);
    for (int i = 0; i < N; i++)
        fprintf(fpout, "  Service rate mu_%-2d         %.6f\n", i+1, 1.0/mu_inv[i]);
    fprintf(fpout, "  Routing policy             %s\n",      policy_name(policy));
    fprintf(fpout, "  Transient length (s)       %.3f\n",    Trslen);
    fprintf(fpout, "  Run length (s)             %.3f\n",    Runlen);
    fprintf(fpout, "  Number of runs             %d\n\n",    NRUNmin);

    fprintf(fpout, "Results (mean +/- 95%% confidence half-width):\n\n");

    fprintf(fpout, "  Average sojourn time       %.6f  +/- %.2e   p:%5.2f%%\n",
            delay->mean(),
            delay->confidence(.95),
            delay->confpercerr(.95));

    fprintf(fpout, "\n  Mean station occupancy (queue + in-service):\n");
    for (int i = 0; i < N; i++) {
        fprintf(fpout, "    L_%-2d                     %.6f  +/- %.2e   p:%5.2f%%\n",
                i + 1,
                length[i]->mean(),
                length[i]->confidence(.95),
                length[i]->confpercerr(.95));
    }

    // Imbalance index as defined in the spec: difference between the largest
    // and the smallest of the per-server mean queue lengths.
    double Lmax = length[0]->mean(), Lmin = length[0]->mean();
    for (int i = 1; i < N; i++) {
        double m = length[i]->mean();
        if (m > Lmax) Lmax = m;
        if (m < Lmin) Lmin = m;
    }
    fprintf(fpout, "\n  Load imbalance index (max_i L_i - min_i L_i)  %.6f\n",
            Lmax - Lmin);
    // Per-run (max-min) averaged across runs: same quantity in steady state
    // but with a confidence interval, useful as a robustness check.
    fprintf(fpout, "  per-run (max-min) avg                         %.6f  +/- %.2e   p:%5.2f%%\n",
            imbalance->mean(),
            imbalance->confidence(.95),
            imbalance->confpercerr(.95));
}

void mqueue::print_trace(int n) {
    fprintf(fptrc, "*********************************************\n");
    fprintf(fptrc, "                 TRACE RUN %d                \n", n);
    fprintf(fptrc, "*********************************************\n\n");

    fprintf(fptrc, "Sojourn time so far          %.6f  +/- %.2e   p:%5.2f%%\n",
            delay->mean(),
            delay->confidence(.95),
            delay->confpercerr(.95));
    for (int i = 0; i < N; i++) {
        fprintf(fptrc, "  L_%-2d                     %.6f\n",
                i+1, length[i]->last_sample());
    }
    fprintf(fptrc, "  imbalance (max-min)      %.6f\n",
            imbalance->last_sample());
    fflush(fptrc);
}

void mqueue::clear_counters(double clk) {
    for (int i = 0; i < N; i++) bufs[i]->reset_counters(clk);
}

void mqueue::clear_stats() {
    delay->reset();
    imbalance->reset();
    for (int i = 0; i < N; i++) length[i]->reset();
}

void mqueue::update_stats(double run_len) {
    // System-wide average sojourn time for this run:
    // weighted average of the per-station means, using packet counts.
    double tot_d = 0.0, tot_p = 0.0;
    for (int i = 0; i < N; i++) {
        tot_d += bufs[i]->tot_delay;
        tot_p += bufs[i]->tot_packs;
    }
    if (tot_p > 0.0) *delay += tot_d / tot_p;

    // Per-station time-weighted mean occupancy.
    double Lmax = -1.0, Lmin = -1.0;
    for (int i = 0; i < N; i++) {
        double Li = bufs[i]->area / run_len;
        *(length[i]) += Li;
        if (i == 0 || Li > Lmax) Lmax = Li;
        if (i == 0 || Li < Lmin) Lmin = Li;
    }
    *imbalance += (Lmax - Lmin);
}

int mqueue::isconfsatisf(double perc) {
    return delay->isconfsatisfied(10, .95);
}
