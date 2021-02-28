/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.4 */

#ifndef PB_PROTOCOL_ALARM_PB_H_INCLUDED
#define PB_PROTOCOL_ALARM_PB_H_INCLUDED
#include <proto/pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _protocol_State {
    bool started;
    bool armed;
    bool modem_sleeping;
    bool reconnecting;
    bool doors_open;
} protocol_State;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define protocol_State_init_default              {0, 0, 0, 0, 0}
#define protocol_State_init_zero                 {0, 0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define protocol_State_started_tag               1
#define protocol_State_armed_tag                 2
#define protocol_State_modem_sleeping_tag        3
#define protocol_State_reconnecting_tag          4
#define protocol_State_doors_open_tag            5

/* Struct field encoding specification for nanopb */
#define protocol_State_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, BOOL,     started,           1) \
X(a, STATIC,   REQUIRED, BOOL,     armed,             2) \
X(a, STATIC,   REQUIRED, BOOL,     modem_sleeping,    3) \
X(a, STATIC,   REQUIRED, BOOL,     reconnecting,      4) \
X(a, STATIC,   REQUIRED, BOOL,     doors_open,        5)
#define protocol_State_CALLBACK NULL
#define protocol_State_DEFAULT NULL

extern const pb_msgdesc_t protocol_State_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define protocol_State_fields &protocol_State_msg

/* Maximum encoded size of messages (where known) */
#define protocol_State_size                      10

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
