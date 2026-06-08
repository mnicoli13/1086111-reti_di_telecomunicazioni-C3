/* -*- C++ -*- */
#include <stdio.h>
#include "global.h"
#include "mqueue.h"
#include "simulator.h"

int main(int argc, char *argv[]) {

    simulator *eval;

    printf("\n");
    printf("**********************************************************\n\n");
    printf("        MULTI-SERVER QUEUE SIMULATION (project C3)\n");
    printf("    N FIFO queues, Poisson arrivals, expon. services,\n");
    printf("       routing: random / round-robin / shortest.\n\n");
    printf("**********************************************************\n\n");

    eval = new mqueue(argc, argv);
    eval->init();
    eval->run();
    eval->results();
    delete eval;
    return 0;
}
