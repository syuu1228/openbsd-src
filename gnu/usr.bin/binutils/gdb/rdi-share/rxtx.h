/* 
 * Copyright (C) 1995 Advanced RISC Machines Limited. All rights reserved.
 * 
 * This software may be freely used, copied, modified, and distributed
 * provided that the above copyright notice is preserved in all copies of the
 * software.
 */

/*-*-C-*-
 *
 * $Revision: 1.2 $
 *     $Date: 1998/01/08 11:12:29 $
 *
 *
 *   Project: ANGEL
 *
 *     Title:  Definitions required for the rx and tx engines
 */

#ifndef angel_rxtx_h
#define angel_rxtx_h


/*
 * we need a definition for bool, which is "system" dependent
 */
#ifdef TARGET
# include "angel.h"
#else
# include "host.h"
#endif

#include "devclnt.h"

/* return status codes for the rx engine */
typedef enum re_status {
  RS_WAIT_PKT,
  RS_IN_PKT,
  RS_BAD_PKT,
  RS_GOOD_PKT
} re_status;

/* return status codes for the tx engine */
typedef enum te_status {
  TS_IDLE,
  TS_IN_PKT,
  TS_DONE_PKT
} te_status;


/*
 * required serial definitions, they should all be <32, refer to the
 * re_config struct comments for more details
 */
#define serial_STX    (0x1c) /* data packet start */
#define serial_ETX    (0x1d) /* packet end */
#define serial_ESC    (0x1b) /* standard escape character */
#define serial_XON    (0x11) /* software flow control - enable transmission */
#define serial_XOFF   (0x13) /* software flow control - disable transmission */

/*
 * All other characters are transmitted clean. If any of the above
 * characters need to be transmitted as part of the serial data stream
 * then the character will be preceded by the "serial_ESC" character,
 * and then the required character transmitted (OR-ed with the
 * "serial_ESCAPE" value, to ensure that the serial stream never has
 * any of the exceptional characters generated by data transfers).
 */

#define serial_ESCAPE   (0x40)  /* OR-ed with escaped characters */

/* bad packet error codes */
typedef enum re_error {
  RE_OKAY,
  RE_U_STX,
  RE_U_ETX,
  RE_LEN,
  RE_CRC,
  RE_NETX,
  RE_INTERNAL
} re_error;

/* a decoded packet */
struct data_packet {
  unsigned short  buf_len;      /* should be set by caller */
  DevChanID       type;         /* valid when status is RS_GOOD_PKT */
  unsigned short  len;          /* --"--                            */
  unsigned int    crc;          /* crc for the unescaped pkt */
  unsigned char   *data;        /* should be set by caller */
};

/*
 * Purpose: typedef for flow control function
 *
 *  Params:
 *          Input:  fc_char  the flow control character in question
 *          In/Out: cb_data  callback data as set in the fc_data
 *                             field of re_config, typically device id
 *
 * This callback would tpyically respond to received XON and XOFF
 * characters by controlling the transmit side of the device.
 */
typedef void (*fc_cb_func)(char fc_char, void *cb_data);


/*
 * Purpose: typedef for the function to alloc the data buffer 
 *
 *  Params:
 *          In/Out: packet    the data packet: len and type will be set on
 *                              entry, and buf_len and data should
 *                              be set by this routine if successful.
 *                  cb_data   callback data as set in the ba_data
 *                              field of re_config, typically device id
 *
 *         Returns: TRUE      buffer allocated okay
 *                  FALSE     couldn't allocate buffer of required size
 *                              for given type
 *
 * This callback should attempt to acquire a buffer for the data portion
 * of the packet which is currently being received, based on the len and 
 * type fields supplied in packet.
 *
 * angel_DD_RxEng_BufferAlloc() is supplied for use as this callback,
 * and will be sufficient for most devices.
 */
typedef bool (*BufferAlloc_CB_Fn)(struct data_packet *packet, void *cb_data);


/*
 * The static info needed by the engine, may vary per device.
 *
 * fc_set and esc_set are bitmaps, e.g. bit 3 == charcode 3 == ASCII ETX.
 * Thus any of the first 32 charcodes can be set for flow control or to
 * be escaped.
 * 
 * Note that esc_set should include all of fc_set, and should have bits
 * set for stx, etx and esc, as a minimum.
 *
 * If character codes > 31 need to be used then fc_set and esc_set
 * and their handling can be extended to use arrays and bit manipulation
 * macros, potentially up to the full 256 possible chars.
 *
 * Note too that this could/should be shared with the tx engine.
 */

struct re_config {
  unsigned char     stx;                  /* the STX char for this device */
  unsigned char     etx;                  /* the ETX --"--                */
  unsigned char     esc;                  /* the ESC --"--                */
  unsigned int      fc_set;               /* bitmap of flow control chars */
  unsigned int      esc_set;              /* bitmap of special chars      */
  fc_cb_func        fc_callback;          /* flow control callback func   */
  void              *fc_data;             /* data to pass to fc_callback  */
  BufferAlloc_CB_Fn ba_callback;          /* buffer alloc callback        */
  void              *ba_data;             /* data to pass to ba_calback   */
};

/* the dynamic info needed by the rx engine */
struct re_state {
  unsigned char          rx_state; /* 3 bits pkt state, 1 prepro state */
  unsigned short         field_c;  /* chars left in current field */
  unsigned short         index;    /* index into buffer */
  unsigned int           crc;      /* crc accumulator */
  re_error               error;    /* valid only if status is RS_BAD_PKT */
  const struct re_config *config;  /* pointer to static config */
};

/* dynamic state info needed by the tx engine */
struct te_state {
  unsigned short         field_c;  /* position in current field */
  unsigned char          tx_state; /* encodes n,e, and f (2+1+2=5 bits) */
  unsigned char          encoded;  /* escape-encoded char for transmission */
  const struct re_config *config;  /* pointer to static config */
  unsigned int           crc;      /* space for CRC (before escaping) */
};

/*
 * Function: Angel_RxEngineInit
 *  Purpose: Initialise state (during device init) for engine.
 *
 *   Params:
 *              Input: config   static config info
 *             In/Out: state    internal state
 */

void Angel_RxEngineInit(const struct re_config *config,
                        struct re_state *state);

/*
 * Function: Angel_RxEngine
 *  Purpose: Rx Engine for character-based devices
 *
 *   Params:
 *              Input: new_ch       the latest character
 *
 *             In/Out: packet       details of packet
 *                                   packet.buf_len and packet.data must
 *                                   be set on entry!
 *                     state        internal state, intially set by
 *                                   angel_RxEngineInit()
 *
 *            Returns: re_status (see above)
 *
 */

re_status Angel_RxEngine(unsigned char new_ch, struct data_packet *packet,
                         struct re_state *state);

/*
 * This can be used as the buffer allocation callback for the rx engine,
 * and will make use of angel_DD_GetBuffer() [in devdrv.h]. 
 *
 * Saves duplicating this callback function in every device driver that
 * uses the rx engine.
 *
 * Note that this REQUIRES that the device id is installed as ba_data
 * in the rx engine config structure for the driver.
 */
bool angel_DD_RxEng_BufferAlloc( struct data_packet *packet, void *cb_data );

/*
 * Function: Angel_TxEngineInit
 *  Purpose: Set up tx engine at start of new packet, calculate CRC etc.
 *           (This should perform the actions described under
 *             "Initialisation" above)
 *
 *   Params:
 *              Input: config   static config info
 *                     packet   the packet to transmit
 *             In/Out: state    internal state
 */

void Angel_TxEngineInit(const struct re_config   *config,
                        const struct data_packet *packet, 
                        struct te_state    *state);

/*
 * Function: Angel_TxEngine
 *  Purpose: Tx Engine for character-based devices
 *
 *   Params:
 *              Input: packet       details of packet
 *                                   packet.len, packet.data and
 *                                   packet.type must
 *                                   be set on entry!
 *             In/Out: state        internal state, intially set by
 *                                   angel_TxEngineStart()
 *             Output: tx_ch        the character to be transmitted 
 *                                   (NOT SET if return code is TS_IDLE)
 *
 *            Returns: te_status (see above)
 */

te_status Angel_TxEngine(const struct data_packet  *packet,
                         struct te_state *state,
                         unsigned char *tx_ch);



#endif /* !defined(angel_rxtx_h) */

/*  EOF rxtx.h */
