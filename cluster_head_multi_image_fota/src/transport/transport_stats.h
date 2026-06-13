#ifndef TRANSPORT_STATS_H
#define TRANSPORT_STATS_H
#include <stdlib.h>
#include <zephyr/kernel.h>

// this module is for the public to fetch the latest transport (MQTT) statistics
// the actual counting/accumulation is done by mqtt callback module via transport_priv.h api
typedef struct
{
    int con;  // total connections
    int dcon; // total disconnections

    // session specific counters
    int con_s;    // seconds of current connection
    int dcon_s;   // seconds of last disconnect period
    int con_fail; // how many times it took to successfully connect to broker last time
    int sub_ack;  // how many sub acks received during current connection
    int sub_fail; // how many subscription failures occurred during current connection
    int pub_ack;  // how many pub acks received during current connection
    int pub_fail; // how many pub failures occured during current connection
    int rx;       // how may messages received during current connection
    int err;      // The received payload was larger than the payload buffer.
} transport_stats_t;

int transport_stats_get(transport_stats_t *tstats);

#endif // TRANSPORT_STATS_H
