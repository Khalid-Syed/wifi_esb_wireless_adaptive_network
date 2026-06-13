#ifndef TRANSPORT_QUEUE_H
#define TRANSPORT_QUEUE_H
#include <stdlib.h>
#include <zephyr/kernel.h>

// transport msg object to use with transport_queue
typedef struct
{
    char *topic;
    char *payload;
    uint8_t qos;
} transport_msg_t;

#define TRANSPORT_MSGQ_ITEM_SIZE_POW_2 16 // must be sizeof(transport_msg_t) rounded up to next power of 2

/**
 * @brief add a transport_msg_t to the transport queue
 * 
 * @param msg 
 * @return int 
 */
int transport_queue_add(transport_msg_t msg);
/**
 * @brief get the number of transport_msg_t in the transport queue
 *
 * @param msg
 * @return int
 */
int transport_queue_pending(void);
/**
 * @brief get a pointer to the next transport_msg_t in the transport queue
 *
 * @param msg
 * @return int
 */
int transport_queue_peek(transport_msg_t *msg);
/**
 * @brief remove the next transport_msg_t from the transport queue
 * 
 * @return int 
 */
int transport_queue_remove(void);
/**
 * @brief clear the transport queue
 * 
 * @return int 
 */
int transport_queue_clear(void);

#endif // TRANSPORT_QUEUE_H
