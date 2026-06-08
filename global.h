#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

/* Seeds used by the rand.h macros: kept separated so the streams
   used for inter-arrivals, services and routing are independent. */
#define SEED_ARR  1
#define SEED_SRV  2
#define SEED_RT   3
/* Kept as an alias for code in rand.c that hard-codes SEED. */
#define SEED      SEED_ARR

#define END 100

/* Upper bound on the number of servers (only used for input validation
   and to size the stat arrays; the simulator itself is fully dynamic). */
#define MAX_N 64

/* Routing policies for the multi-server system. */
#define ROUTE_RANDOM    1
#define ROUTE_RR        2
#define ROUTE_SHORTEST  3

#endif
