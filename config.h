#ifndef CONFIG_H_INCLUDE
#define CONFIG_H_INCLUDE

#define SIMULATION  true
#define LOGGING     true
#define SERIAL_PLOT false

#define TQ_CLK   3
#define HARDSYNC 5
#define RESYNC   6
#define STATE_0  8
#define STATE_1  9

#if SIMULATION
    #define TQ 10000 // time quantum, in microseconds
#else
    #define TQ 10000 // time quantum, in microseconds
    
    #define INPUT_BIT 10
    #define WRITE_BIT 11
    #define BUS_IDLE  12
#endif

#define SJW 1
#define T1  8
#define T2 -7

#define DOMINANT    LOW
#define RECESSIVE   HIGH

#endif
