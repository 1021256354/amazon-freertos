/*
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file aws_iot_large_object_transfer.h
 * @brief The file provides an interface for large object transfer protocol. Large Object transfer is used
 * to send larger payloads, ( greater than link MTU size) directly to a peer.
 */


#ifndef AWS_IOT_LARGE_OBJECT_TRANSFER_H_
#define AWS_IOT_LARGE_OBJECT_TRANSFER_H_

#include <stdint.h>
#include <stddef.h>


#define IOT_LARGE_OBJECT_TRANSFER_MAX_WINDOW_SIZE  ( 32 )

#define IOT_LARGE_OBJECT_TRANSFER_BITMAP_SIZE  (  ( IOT_LARGE_OBJECT_TRANSFER_MAX_WINDOW_SIZE + 7 )  >> 3 )

/**
 * @brief Status codes passed in from large object transfer callback invocations.
 */
typedef enum AwsIotLargeObjectTransferStatus
{
    eAwsIotLargeObjectTransferInit = 0,
    eAwsIotLargeObjectTransferOpen,//!< eAwsIotLargeObjectTransferInProgress
    eAwsIotLargeObjectTransferResumable,
    eAwsIotLargeObjectTransferClosed
} AwsIotLargeObjectTransferStatus_t;

/**
 * @brief Error codes returned by large object transfer APIs.
 */
typedef enum AwsIotLargeObjectTransferError
{
      /** Errors which can also be returned by peer **/
    AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS = 0,
    AWS_IOT_LARGE_OBJECT_TRANSFER_MAX_SESSIONS_REACHED,
    AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_NOT_FOUND,
    AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_ABORTED,
    AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_WRONG_WINDOW,
    AWS_IOT_LARGE_OBJECT_TRANSFER_INVALID_PACKET,

    /** Other errors */
    AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY,
    AWS_IOT_LARGE_OBJECT_TRANSFER_NETWORK_ERROR,
    AWS_IOT_LARGE_OBJECT_TRANSFER_EXPIRED,
    AWS_IOT_LARGE_OBJECT_TRANSFER_INVALID_PARAM,
    AWS_IOT_LARGE_OBJECT_TRANSFER_INTERNAL_ERROR
} AwsIotLargeObjectTransferError_t;

typedef enum AwsIotLargeObjectSessionType
{
    AWS_IOT_LARGE_OBJECT_SESSION_SEND = 0,
    AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE

} AwsIotLargeObjectSessionType_t;

typedef enum AwsIotLargeObjectWindowType
{
    AWS_IOT_LARGE_OBJECT_WINDOW_EVEN = 0,
    AWS_IOT_LARGE_OBJECT_WINDOW_ODD

} AwsIotLargeObjectWindowType_t;

/**
 * @brief Network parameters negotiated for large object transfer.
 */
typedef struct AwsIotLargeObjectTransferParams
{
    uint16_t usMTU;                      /** !< usMTU : Maximum size of the packet which can be transmitted over the connection. */
    uint16_t windowSize;                 /** !< windowSize Number of blocks which can be transferred at once without receiving an acknowledgement. */
    uint16_t timeoutMilliseconds;        /** !< timeoutMilliseconds Timeout in milliseconds for one window of transfer. */
    uint16_t numRetransmissions;         /** !< numRetransmissions Number of retransmissions. */
} AwsIotLargeObjectTransferParams_t;


/**
 * @brief Callback used to receive bytes of maximum MTU size from a physical network.
 * Returns pdFALSE if message is incomplete. Caller needs to invoke the function again with the complete message.
 *         pdTRUE if the message parsing is complete.
 *
 */
typedef void( * AwsIotLargeObjectTransferReceiveCallback_t ) (
        void * pvContext,
        const void * pvReceivedData,
        size_t xDataLength );

typedef struct AwsIotLargeObjectTransferNetworkIface
{

    void *pvConnection;                                  /**!< Pointer to a connection. */

     size_t ( * send )(                           /**!< Function pointer to send data over a connection. */
            void * pvConnection,
            const void * const pvMessage ,
            size_t xLength );

    int32_t ( * setNetworkReceiveCallback )(            /**!< Function pointer to set the receive callback for the connection. */
            void * pvConnection,
            void* pvContext,
            AwsIotLargeObjectTransferReceiveCallback_t );

} AwsIotLargeObjectTransferNetworkIface_t;



typedef struct AwsIotLargeObjectSendSession
{
    uint16_t usSessionID;
    AwsIotLargeObjectTransferStatus_t xState;

    const void *pvContext;
    const uint8_t *pucObject;
    size_t xObjectLength;

    size_t   xOffset;
    uint16_t usWindowSize;
    uint16_t usBlockSize;
    AwsIotLargeObjectWindowType_t xWindowType;

    uint16_t usRetriesLeft;
    uint16_t usNumRetries;
    TimerHandle_t xRetransmitTimer;

} AwsIotLargeObjectSendSession_t;

typedef void( * AwsIotLargeObjectReceiveCallback_t ) (
        const uint16_t usSessionID,
        const uint8_t * pucData,
        size_t xDataLength,
        BaseType_t xMore );

typedef struct AwsIotLargeObjectReceiveSession
{
    uint16_t usSessionID;
    AwsIotLargeObjectTransferStatus_t xState;

    void *pvContext;
    uint8_t* pucBuffer;
    size_t xBufferLength;


    size_t xOffset;
    uint16_t usWindowSize;
    uint16_t usBlockSize;
    uint16_t usNumBlocksReceived;
    uint16_t usNumWindowBlocks;
    BaseType_t xLastWindow;
    uint8_t ucBlockBitMap[ IOT_LARGE_OBJECT_TRANSFER_BITMAP_SIZE ];
    AwsIotLargeObjectWindowType_t xWindowType;

    uint16_t usRetriesLeft;
    uint16_t usNumRetries;
    TimerHandle_t xAckTimer;

} AwsIotLargeObjectReceiveSession_t;

typedef union AwsIotLargeObjectSession
{
    AwsIotLargeObjectSendSession_t xSend;
    AwsIotLargeObjectReceiveSession_t xRecv;

} AwsIotLargeObjectSession_t;


/**
 * @brief Callback invoked on a send or receive session completion.
 */
typedef void ( *AwsIotLargeObjectSessionCompleteCallback_t )(
        AwsIotLargeObjectSessionType_t xSessionType,
        uint16_t usSessionID,
        AwsIotLargeObjectTransferError_t xResult );


/**
 * @brief Structure has the underlying context for the large object transfer sessions.
 * Context should be created before creating all large object sessions and destroyed only when all the sessions are completed.
 */
typedef struct AwsIotLargeObjectTransferContext
{

    AwsIotLargeObjectTransferNetworkIface_t xNetworkIface;               /**!< xNetworkIface Network interface to use for large object transfer.   */
    AwsIotLargeObjectSessionCompleteCallback_t xCompletionCallback;      /**!< xCompletionCallback Callback invoked on a large object send/receive completion */
    AwsIotLargeObjectReceiveCallback_t xReceiveCallback;                 /**!< xReceiveCallback Callback for streaming received large object to user */
    AwsIotLargeObjectTransferParams_t xParameters;                       /**!< xParameters Parameters used for large object transfer.              */
    uint16_t usNumSendSessions;                                /**!< usNumSendSessions Number of send sessions within the context.       */
    AwsIotLargeObjectSession_t* pxSendSessions;                /**!< pxSendSessions User allocated array to hold the send sessions.      */
    uint16_t usNumRecvSessions;                                /**!< usNumRecvSessions Number of receive sessions within the context.    */
    AwsIotLargeObjectSession_t* pxRecvSessions;                /**!< pxRecvSessions User allocated array to hold the receive sessions.   */

} AwsIotLargeObjectTransferContext_t;


/**
 * @brief Destroys the resources for a context.
 * Frees the resources associated with the context. All Sessions should be aborted, completed or failed state.
 */
AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Init(
        AwsIotLargeObjectTransferContext_t* pxContext,
        uint16_t usNumSendSessions,
        uint16_t usNumReceiveSessions );

/**
 * @brief Initiates sending a large object to a peer.
 * Initiates transfer of a large object by sending START message to the peer.
 */
AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Send(
        AwsIotLargeObjectTransferContext_t* pxContext,
        const uint8_t *pucObject,
        const size_t xObjectLength,
        uint16_t* pusSessionID );

/**
 * @brief Resumes a large object transfer session.
 * Only Sender can resume a previously timedout session. Failed or Aborted sessions cannot be resumed.
 *
 */
AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Resume( AwsIotLargeObjectTransferContext_t* pxContext, uint16_t usSessionID );

/**
 * @brief Aborts a large object transfer session.
 * Aborts an ongoing large object transfer session. Both receiver and sender can abort a large object transfer sesssion.
 */
AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_CloseSession( AwsIotLargeObjectTransferContext_t* pxContext,
                                                                  AwsIotLargeObjectSessionType_t xType,
                                                                  uint16_t usSessionID );

/**
 * @brief Destroys the resources for a context.
 * Frees the resources associated with the context. All Sessions should be aborted, completed or failed state.
 */
void AwsIotLargeObjectTransfer_Destroy( AwsIotLargeObjectTransferContext_t* pxContext );

#endif /* AWS_IOT_LARGE_OBJECT_TRANSFER_H_ */
