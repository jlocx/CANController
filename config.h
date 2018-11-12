#ifndef CONFIG_H_INCLUDE
#define CONFIG_H_INCLUDE

#define SIMULATION

#define TQ_CLK   3
#define HARDSYNC 5
#define RESYNC   6
#define STATE_0  8
#define STATE_1  9

#define LED 13

#ifdef SIMULATION
    #define TQ 1000000 // time quantum, in microseconds
#else
    #define TQ 10000    // time quantum = 10 ms, in microseconds
#endif

#define SJW 1
#define T1  8
#define T2 -7

#define DOMINANT    LOW
#define RECESSIVE   HIGH

#endif