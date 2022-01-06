/************************************************************************
** File: cf_cfdp_types.h
**
** NASA Docket No. GSC-18,447-1, and identified as “CFS CFDP (CF)
** Application version 3.0.0”
** Copyright © 2019 United States Government as represented by the
** Administrator of the National Aeronautics and Space Administration.
** All Rights Reserved.
** Licensed under the Apache License, Version 2.0 (the "License"); you may
** not use this file except in compliance with the License. You may obtain
** a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
**
** Purpose:
**  The CF Application cfdp engine and packet parsing header file
**
**
**
*************************************************************************/

#ifndef CF_CFDP_TYPES_H
#define CF_CFDP_TYPES_H

#include "common_types.h"
#include "cf_cfdp_pdu.h"
#include "cf_platform_cfg.h"
#include "cf_clist.h"
#include "cf_chunk.h"
#include "cf_timer.h"
#include "cf_crc.h"
#include "cf_codec.h"

#define CF_NUM_TRANSACTIONS_PER_CHANNEL                                                \
    (CF_MAX_COMMANDED_PLAYBACK_FILES_PER_CHAN + CF_MAX_SIMULTANEOUS_RX +               \
     ((CF_MAX_POLLING_DIR_PER_CHAN + CF_MAX_COMMANDED_PLAYBACK_DIRECTORIES_PER_CHAN) * \
      CF_NUM_TRANSACTIONS_PER_PLAYBACK))
#define CF_NUM_TRANSACTIONS (CF_NUM_CHANNELS * CF_NUM_TRANSACTIONS_PER_CHANNEL)

#define CF_NUM_HISTORIES (CF_NUM_CHANNELS * CF_NUM_HISTORIES_PER_CHANNEL)

#define CF_NUM_CHUNKS_ALL_CHANNELS (CF_TOTAL_CHUNKS * CF_NUM_TRANSACTIONS_PER_CHANNEL)

typedef enum
{
    CF_TxnState_IDLE    = 0,
    CF_TxnState_R1      = 1,
    CF_TxnState_S1      = 2,
    CF_TxnState_R2      = 3,
    CF_TxnState_S2      = 4,
    CF_TxnState_DROP    = 5, /* class 1 received file data without metadata, no file info, so drop */
    CF_TxnState_INVALID = 6
} CF_TxnState_t;

typedef enum
{
    CF_TxSubState_METADATA         = 0,
    CF_TxSubState_FILEDATA         = 1,
    CF_TxSubState_EOF              = 2,
    CF_TxSubState_WAIT_FOR_EOF_ACK = 3,
    CF_TxSubState_WAIT_FOR_FIN     = 4,
    CF_TxSubState_SEND_FIN_ACK     = 5,
    CF_TxSubState_NUM_STATES       = 6
} CF_TxSubState_t;

typedef enum
{
    CF_RxSubState_FILEDATA         = 0,
    CF_RxSubState_EOF              = 1,
    CF_RxSubState_WAIT_FOR_FIN_ACK = 2,
    CF_RxSubState_NUM_STATES       = 3,
} CF_RxSubState_t;

typedef enum
{
    CF_RxEofRet_SUCCESS        = 0,
    CF_RxEofRet_FSIZE_MISMATCH = 1,
    CF_RxEofRet_BAD_EOF        = 2,
    CF_RxEofRet_INVALID        = 3,
} CF_RxEofRet_t;

typedef struct
{
    char src_filename[CF_FILENAME_MAX_LEN];
    char dst_filename[CF_FILENAME_MAX_LEN];
} CF_TxnFilenames_t;

typedef enum
{
    CF_Direction_RX  = 0,
    CF_Direction_TX  = 1,
    CF_Direction_NUM = 2,
} CF_Direction_t;

typedef struct CF_History
{
    CF_TxnFilenames_t       fnames;
    CF_CListNode_t          cl_node;
    CF_Direction_t          dir;
    CF_CFDP_ConditionCode_t cc;
    CF_EntityId_t           src_eid;  /* src_eid is always the source eid */
    CF_EntityId_t           peer_eid; /* peer_eid is always the "other guy", which is the same src_eid for RX */
    CF_TransactionSeq_t     seq_num;  /* stays constant for entire transfer */
} CF_History_t;

typedef struct CF_ChunkWrapper
{
    CF_ChunkList_t chunks;
    CF_CListNode_t cl_node;
} CF_ChunkWrapper_t;

typedef struct CF_Playback
{
    uint32            dir_id;
    CF_CFDP_Class_t   cfdp_class;
    CF_TxnFilenames_t fnames;
    uint16            num_ts; /* number of transactions -- 16 bit should be enough */
    uint8             priority;
    CF_EntityId_t     dest_id;

    bool busy;
    bool diropen;
    bool keep;
    bool counted;
} CF_Playback_t;

typedef struct CF_Poll
{
    CF_Playback_t pb;
    CF_Timer_t    interval_timer;
    bool          timer_set;
} CF_Poll_t;

typedef struct CF_TxS2_Data
{
    uint8 fin_cc; /* remember the cc in the received fin pdu to echo in eof-fin */
    uint8 acknak_count;
} CF_TxS2_Data_t;

typedef struct CF_TxState_Data
{
    CF_TxSubState_t sub_state;
    uint32          cached_pos;

    CF_TxS2_Data_t s2;
} CF_TxState_Data_t;

typedef struct CF_RxS2_Data
{
    uint32                    eof_crc;
    uint32                    eof_size;
    uint32                    rx_crc_calc_bytes;
    CF_CFDP_FinDeliveryCode_t dc;
    CF_CFDP_FinFileStatus_t   fs;
    uint8                     eof_cc; /* remember the cc in the received eof pdu to echo in eof-ack */
    uint8                     acknak_count;
} CF_RxS2_Data_t;

typedef struct CF_RxState_Data
{
    CF_RxSubState_t sub_state;
    uint32          cached_pos;

    CF_RxS2_Data_t r2;
} CF_RxState_Data_t;

typedef struct CF_Flags_Common
{
    uint8 q_index; /* which Q is this in? */
    bool  ack_timer_armed;
    bool  suspended;
    bool  canceled;
    bool  crc_calc;
} CF_Flags_Common_t;

typedef struct CF_Flags_Rx
{
    CF_Flags_Common_t com;

    bool md_recv; /* md received for r state */
    bool eof_recv;
    bool send_nak;
    bool send_fin;
    bool send_ack;
    bool inactivity_fired; /* used for r2 */
    bool complete;         /* r2 */
    bool fd_nak_sent;      /* latches that at least one nak has been sent for file data */
} CF_Flags_Rx_t;

typedef struct CF_Flags_Tx
{
    CF_Flags_Common_t com;

    bool md_need_send;
    bool cmd_tx; /* indicates transaction is commanded (ground) tx */
} CF_Flags_Tx_t;

typedef union CF_StateFlags
{
    CF_Flags_Common_t com;
    CF_Flags_Rx_t     rx;
    CF_Flags_Tx_t     tx;
} CF_StateFlags_t;

typedef union CF_StateData
{
    CF_TxState_Data_t s;
    CF_RxState_Data_t r;
} CF_StateData_t;

typedef struct CF_Transaction
{
    CF_TxnState_t state; /* each engine is commanded to do something, which is the overall state */

    CF_History_t      *history;          /* weird, but this also holds active filenames and possibly other info */
    CF_ChunkWrapper_t *chunks;           /* for gap tracking, only used on class 2 */
    CF_Timer_t         inactivity_timer; /* set to the overall inactivity timer of a remote */
    CF_Timer_t         ack_timer;        /* called ack_timer, but is also nak_timer */

    uint32    fsize; /* lseek() should be 64-bit on 64-bit system, but osal limits to 32-bit */
    uint32    foffs; /* offset into file for next read */
    osal_id_t fd;

    CF_Crc_t crc;

    uint8 keep;
    uint8 chan_num; /* if ever more than one engine, this may need to change to pointer */
    uint8 priority;

    CF_CListNode_t cl_node;

    CF_Playback_t *p; /* NULL if transaction does not belong to a playback */

    CF_StateData_t state_data;

    /* NOTE: the flags here look a little strange, because there are different flags for TX and RX.
     * Both types share the same type of flag, though. Since RX flags plus the global flags is
     * over one byte, storing them this way allows 2 bytes to cover all possible flags.
     * Please ignore the duplicate declarations of the "all" flags. :) */
    CF_StateFlags_t flags;

} CF_Transaction_t;

typedef enum
{
    CF_QueueIdx_PEND      = 0, /* first one on this list is active */
    CF_QueueIdx_TXA       = 1,
    CF_QueueIdx_TXW       = 2,
    CF_QueueIdx_RX        = 3,
    CF_QueueIdx_HIST      = 4,
    CF_QueueIdx_HIST_FREE = 5,
    CF_QueueIdx_FREE      = 6,
    CF_QueueIdx_NUM       = 7,
} CF_QueueIdx_t;

typedef enum
{
    CF_TickType_RX,
    CF_TickType_TXW_NORM,
    CF_TickType_TXW_NAK,
    CF_TickType_NUM_TYPES
} CF_TickType_t;

typedef struct CF_Channel
{
    CF_CListNode_t *qs[CF_QueueIdx_NUM];
    CF_CListNode_t *cs[CF_Direction_NUM];

    CFE_SB_PipeId_t pipe;

    uint32 num_cmd_tx;

    CF_Playback_t playback[CF_MAX_COMMANDED_PLAYBACK_DIRECTORIES_PER_CHAN];

    /* For polling directories, the configuration data is in a table. */
    CF_Poll_t poll[CF_MAX_POLLING_DIR_PER_CHAN];

    uint32 sem_id; /* semaphore id for output pipe */

    const CF_Transaction_t *cur; /* current transaction during channel cycle */

    uint8 tick_type;
} CF_Channel_t;

/* NOTE: the use of CF_CFDP_PduHeader_t below is correct, but the CF_PduRecvMsg_t and CF_PduSendMsg_t
 * structures are both longer than these definitions. They are always backed by a buffer
 * of size CF_MAX_PDU_SIZE */
typedef struct CF_PduRecvMsg
{
    CFE_MSG_CommandHeader_t hdr;
    CF_CFDP_PduHeader_t     ph;
} CF_PduRecvMsg_t;

typedef struct CF_PduSendMsg
{
    CFE_MSG_TelemetryHeader_t hdr;
    CF_CFDP_PduHeader_t       ph;
} CF_PduSendMsg_t;

typedef struct CF_Output
{
    CFE_SB_Buffer_t  *msg;
    CF_EncoderState_t encode;

    /* Temporary R/W buffer for holding output PDUs while working with them */
    CF_Logical_PduBuffer_t tx_pdudata;
} CF_Output_t;

typedef struct CF_Input
{
    CFE_SB_Buffer_t  *msg;
    CF_DecoderState_t decode;

    /* Temporary R/W buffer for holding input PDUs while working with them */
    CF_Logical_PduBuffer_t rx_pdudata;
} CF_Input_t;

/* An engine represents a pairing to a local EID
 *
 * Each engine can have at most CF_MAX_SIMULTANEOUS_TRANSACTIONS */
typedef struct CF_Engine
{
    CF_TransactionSeq_t seq_num; /* keep track of the next sequence number to use for sends */

    CF_Output_t out;
    CF_Input_t  in;

    /* NOTE: could have separate array of transactions as part of channel? */
    CF_Transaction_t transactions[CF_NUM_TRANSACTIONS];
    CF_History_t     histories[CF_NUM_HISTORIES];
    CF_Channel_t     channels[CF_NUM_CHANNELS];

    CF_ChunkWrapper_t chunks[CF_NUM_TRANSACTIONS * CF_Direction_NUM];
    CF_Chunk_t        chunk_mem[CF_NUM_CHUNKS_ALL_CHANNELS];

    uint32 outgoing_counter;
    uint8  enabled;
} CF_Engine_t;

typedef enum
{
    CF_SendRet_SUCCESS = 0,
    CF_SendRet_NO_MSG  = 1,
    CF_SendRet_ERROR   = 2, /* the send itself failed */
    CF_SendRet_FAILURE = 3, /* generic failure message not relating to message send */
} CF_SendRet_t;

#endif