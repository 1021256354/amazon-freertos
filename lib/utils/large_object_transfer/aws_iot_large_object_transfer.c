/*
 * Amazon FreeRTOS
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
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file aws_iot_large_object_transfer.c
 * @brief File implements the large object transfer protocol.
 */

/**
 * Standard header files.
 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Build using a config header, if provided. */
#ifdef AWS_IOT_CONFIG_FILE
    #include AWS_IOT_CONFIG_FILE
#endif

/* Logging. */
#include "aws_iot_logging.h"

/*
 * FreeRTOS includes.
 */
#include "FreeRTOS.h"
#include "timers.h"
#include "aws_iot_large_object_transfer.h"


/* Configure logs for TASKPOOL functions. */
#ifdef AWS_IOT_LOG_LEVEL_LARGE_OBJECT_TRANSFER
#define _LIBRARY_LOG_LEVEL        AWS_IOT_LOG_LEVEL_LARGE_OBJECT_TRANSFER
#else
#ifdef AWS_IOT_LOG_LEVEL_GLOBAL
#define _LIBRARY_LOG_LEVEL    AWS_IOT_LOG_LEVEL_GLOBAL
#else
#define _LIBRARY_LOG_LEVEL    AWS_IOT_LOG_DEBUG
#endif
#endif

#define _LIBRARY_LOG_NAME    ( "LARGE_OBJ_TRANSFER" )
#include "aws_iot_logging_setup.h"



#define _INVALID_SESSION_ID                                          ( 0 )

#define _INCR_OFFSET( offset, windowSize, blockSize )                ( offset + ( windowSize * blockSize ) )
/*
 * Does a round up conversion of number of bits to number of bytes.
 */
#define _BITS_TO_BYTES_ROUNDUP( numBits )                          (  ( ( size_t ) numBits + 7 ) >> 3 )
/*
 *
 * Size of Bitmap used to represent the missing block numbers in a window.
 */

#define _BITMAP_LENGTH( windowSize )                ( _BITS_TO_BYTES_ROUNDUP(  windowSize ) )

#define _SESSION_ID( pucBlock )                      ( * ( ( uint16_t * ) pucBlock ) )

#define _SESSION_ID_LENGTH                           ( 2 )

#define _BLOCK_NUMBER( pucBlock )                    ( *( ( uint16_t * ) ( pucBlock + _SESSION_ID_LENGTH ) ) )

#define _BLOCK_NUMBER_LENGTH                         ( 2 )

#define _FLAGS( pucBlock )                           ( *( ( uint8_t *) ( pucBlock + _SESSION_ID_LENGTH + _BLOCK_NUMBER_LENGTH ) ) )

#define _FLAGS_LENGTH                                ( 1 )

#define _BLOCK_HEADER_LENGTH                         ( _SESSION_ID_LENGTH + _FLAGS_LENGTH + _BLOCK_NUMBER_LENGTH )

#define _RESERVED_BITS_MASK                          (  ( uint8_t ) 0xF8 )

#define _FLAGS_INTIALIZER                            _RESERVED_BITS_MASK

#define _LAST_BLOCK_MASK                             (  ( uint8_t ) 0x1 )

#define _RESUME_SESSION_MASK                         (  ( uint8_t ) 0x2 )

#define _ODD_WINDOW_MASK                             (  ( uint8_t ) 0x4 )

#define _BLOCK_DATA( pucBlock )                      ( ( ( uint8_t * ) ( pucBlock +  _BLOCK_HEADER_LENGTH ) ) )

#define _MAX_BLOCK_DATA_LEN( usMTU )                 ( ( usMTU - _BLOCK_HEADER_LENGTH ) )

#define _BLOCK_LENGTH( xDataLength )                 ( ( xDataLength + _BLOCK_HEADER_LENGTH ))

#define _ACK_HEADER_LENGTH                              ( 3 )

#define _BITMAP_LEN( xACKLength )                    ( xACKLength - _ACK_HEADER_LENGTH )

#define _ACK_LENGTH( xBitMapLength )                 ( xBitMapLength + _ACK_HEADER_LENGTH )

#define _MIN_ACK_LENGTH                              _ACK_HEADER_LENGTH

#define _ERROR_CODE( pucACK )                        ( * ( ( uint8_t * ) ( pucACK + 2 ) ) )

#define _BITMAP( pucACK )                            ( ( uint8_t * ) ( pucACK + _ACK_HEADER_LENGTH ) )

#define _SESSION_FREE( xState )                      ( xState != eAwsIotLargeObjectTransferOpen || xState != eAwsIotLargeObjectTransferResumable )


void prvCloseSession( AwsIotLargeObjectSession_t* pxSession, AwsIotLargeObjectSessionType_t xType, BaseType_t xResumable )
{
    BaseType_t xResult;
    AwsIotLargeObjectTransferStatus_t xState;


    xState = ( xResumable == pdTRUE ) ? eAwsIotLargeObjectTransferResumable : eAwsIotLargeObjectTransferClosed;


    if( xType == AWS_IOT_LARGE_OBJECT_SESSION_SEND )
    {

        pxSession->xSend.xState = xState;
        xResult = xTimerStop( pxSession->xSend.xRetransmitTimer, portMAX_DELAY );
        configASSERT( xResult == pdTRUE );
    }
    else
    {
        pxSession->xRecv.xState = xState;
        xResult = xTimerStop( pxSession->xRecv.xAckTimer, portMAX_DELAY );
        configASSERT( xResult == pdTRUE );
    }
}


 AwsIotLargeObjectTransferError_t prxSendBlock(
         AwsIotLargeObjectTransferNetworkIface_t* pxNetwork,
         uint16_t usSessionId,
         uint16_t usBlockNum,
         BaseType_t xLastBlock,
         BaseType_t xResume,
         AwsIotLargeObjectWindowType_t xWindowType,
         const uint8_t *pucBlockData,
         size_t xDataLength )

 {
     size_t xBlockLength = _BLOCK_LENGTH( xDataLength );
     size_t xSent;
     uint8_t* pucBlock = NULL;
     AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
     uint8_t ucFlags = _FLAGS_INTIALIZER;

     pucBlock = AwsIotNetwork_Malloc( xBlockLength );
     if( pucBlock != NULL )
     {
         _SESSION_ID( pucBlock ) = usSessionId;
         _BLOCK_NUMBER( pucBlock ) = usBlockNum;

         if( xLastBlock )
         {
             ucFlags |= _LAST_BLOCK_MASK;
         }

         if( xResume )
         {
             ucFlags |= _RESUME_SESSION_MASK;
         }

         if( xWindowType == AWS_IOT_LARGE_OBJECT_WINDOW_ODD )
         {
             ucFlags |= _ODD_WINDOW_MASK;
         }

         _FLAGS( pucBlock ) = ucFlags;

         memcpy( _BLOCK_DATA( pucBlock ), pucBlockData, xDataLength );

         xSent = pxNetwork->send( pxNetwork->pvConnection,
                              pucBlock,
                              xBlockLength );
         if( xSent < xBlockLength )
         {
             xError = AWS_IOT_LARGE_OBJECT_TRANSFER_NETWORK_ERROR;
         }
         AwsIotNetwork_Free( pucBlock );
     }
     else
     {
         xError = AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY;
     }

     return xError;
 }


AwsIotLargeObjectTransferError_t prxSendWindow( AwsIotLargeObjectSendSession_t *pxSession )
{
    size_t xBlockOffset, xBlockLength;
    const uint8_t *pucBlock;
    uint16_t usBlockNum ;
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
    BaseType_t xLastBlock = pdFALSE;
    AwsIotLargeObjectTransferContext_t *pxContext = (  AwsIotLargeObjectTransferContext_t* ) ( pxSession->pvContext );


    if( pxSession->xOffset < pxSession->xObjectLength )
    {
        /** Start transmitting blocks up to window size or last block **/
        for ( usBlockNum = 0; ( usBlockNum < pxSession->usWindowSize ) && ( !xLastBlock ); usBlockNum++ )
        {
            xBlockOffset = pxSession->xOffset + ( usBlockNum * pxSession->usBlockSize );
            xBlockLength = pxSession->usBlockSize;

            if( ( xBlockOffset + xBlockLength ) >= pxSession->xObjectLength )
            {
                xBlockLength = ( pxSession->xObjectLength - xBlockOffset );
                xLastBlock = pdTRUE;
            }

            pucBlock = ( pxSession->pucObject + xBlockOffset );

            xError = prxSendBlock( &pxContext->xNetworkIface,
                                   pxSession->usSessionID,
                                   usBlockNum,
                                   xLastBlock,
                                   pdFALSE,
                                   pxSession->xWindowType,
                                   pucBlock,
                                   xBlockLength );

            if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                break;
            }
        }
    }

    return xError;
}


void prvRetransmitWindow( TimerHandle_t xTimer )
{
    AwsIotLargeObjectTransferError_t xError;
    AwsIotLargeObjectSendSession_t* pxSession = ( AwsIotLargeObjectSendSession_t *) pvTimerGetTimerID( xTimer );
    configASSERT( pxSession != NULL );
    BaseType_t xStatus;

    if( pxSession->usRetriesLeft > 0 )
    {
        xError = prxSendWindow( pxSession );
        if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
         {
             AwsIotLogError( "Failed to retransmit window, session = %d\n", pxSession->usSessionID );
             prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdTRUE );
         }
         else
         {
             pxSession->usRetriesLeft--;
             xStatus = xTimerStart( xTimer, portMAX_DELAY );
             configASSERT( xStatus == pdTRUE );
         }
    }
    else
    {
        AwsIotLogError( "No retries remaining for session id: %d\n", pxSession->usSessionID );
        prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdTRUE);
    }
}


static AwsIotLargeObjectTransferError_t prxSendACK(
        AwsIotLargeObjectTransferNetworkIface_t* pxNetwork,
        uint16_t usSessionId,
        AwsIotLargeObjectTransferError_t xErrorCode,
        const uint8_t *pucBitMap,
        size_t xBitMapLength )
{
    uint8_t *pucAck = NULL;
    size_t xAckLength = _ACK_LENGTH( xBitMapLength );
    size_t xSent;
    AwsIotLargeObjectTransferError_t xResult = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;

    pucAck = AwsIotNetwork_Malloc( xAckLength );
    if( pucAck != NULL )
    {
        _SESSION_ID( pucAck ) = usSessionId;
        _ERROR_CODE( pucAck ) = ( uint8_t ) xErrorCode;
        if( xBitMapLength > 0 )
        {
            memcpy( _BITMAP( pucAck ), pucBitMap, xBitMapLength );
        }

        xSent = pxNetwork->send( pxNetwork->pvConnection,
                                 pucAck,
                                 xAckLength );
        if( xSent < xAckLength )
        {
            xResult = AWS_IOT_LARGE_OBJECT_TRANSFER_NETWORK_ERROR;
        }

        AwsIotNetwork_Free( pucAck );
    }
    else
    {
        xResult = AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY;
    }

    return xResult;
}


 static void prvTimerSendACK( TimerHandle_t xTimer )
 {
     AwsIotLargeObjectTransferContext_t *pxContext;
     AwsIotLargeObjectTransferError_t xError;
     AwsIotLargeObjectReceiveSession_t* pxSession = ( AwsIotLargeObjectReceiveSession_t *) pvTimerGetTimerID( xTimer );

     configASSERT( pxSession != NULL );

     pxContext = ( AwsIotLargeObjectTransferContext_t * ) ( pxSession->pvContext );

     xError = prxSendACK(
             &pxContext->xNetworkIface,
             pxSession->usSessionID,
             AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS,
             pxSession->ucBlockBitMap,
             _BITMAP_LENGTH( pxSession->usWindowSize ) );

     if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
     {
         AwsIotLogWarn( "Failed to send acknowledgment for session id: %d", pxSession->usSessionID );
     }
 }

AwsIotLargeObjectTransferError_t prxInitSession( AwsIotLargeObjectTransferContext_t *pxContext,
                                                 AwsIotLargeObjectSessionType_t xType,
                                                 AwsIotLargeObjectSession_t *pxSession )

{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
    size_t xBufferSize;

    if( xType == AWS_IOT_LARGE_OBJECT_SESSION_SEND )
    {
        pxSession->xSend.xState = eAwsIotLargeObjectTransferInit;
        pxSession->xSend.pvContext = pxContext;
        pxSession->xSend.usSessionID = _INVALID_SESSION_ID;
        pxSession->xSend.usWindowSize = pxContext->xParameters.windowSize;
        pxSession->xSend.usBlockSize  = _MAX_BLOCK_DATA_LEN( pxContext->xParameters.usMTU );
        pxSession->xSend.usNumRetries = pxContext->xParameters.numRetransmissions;

        pxSession->xSend.xRetransmitTimer =
                xTimerCreate(
                        "RetransmitTimer",
                        pdMS_TO_TICKS( pxContext->xParameters.timeoutMilliseconds * 2 ),
                        pdFALSE,
                        ( pxSession ),
                        prvRetransmitWindow );

        if( pxSession->xSend.xRetransmitTimer == NULL )
        {
            xError =  AWS_IOT_LARGE_OBJECT_TRANSFER_INTERNAL_ERROR;
        }

    }
    else
    {
        pxSession->xRecv.xState = eAwsIotLargeObjectTransferInit;
        pxSession->xRecv.pvContext = pxContext;
        pxSession->xRecv.usSessionID = _INVALID_SESSION_ID;
        pxSession->xRecv.usWindowSize = pxContext->xParameters.windowSize;
        pxSession->xRecv.usBlockSize  = _MAX_BLOCK_DATA_LEN( pxContext->xParameters.usMTU );
        pxSession->xRecv.usNumRetries = pxContext->xParameters.numRetransmissions;
        pxSession->xRecv.usNumWindowBlocks = pxContext->xParameters.windowSize;

        xBufferSize = (pxSession->xRecv.usBlockSize * pxSession->xRecv.usWindowSize );
        pxSession->xRecv.pucBuffer = AwsIotNetwork_Malloc( xBufferSize );

        if( pxSession->xRecv.pucBuffer == NULL )
        {
            xError =  AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY;
        }
        else
        {
            pxSession->xRecv.xAckTimer =
                    xTimerCreate(
                            "ACKTimer",
                            pdMS_TO_TICKS( pxContext->xParameters.timeoutMilliseconds ),
                            pdFALSE,
                            ( &pxSession ),
                            prvTimerSendACK );

            if( pxSession->xRecv.xAckTimer == NULL )
            {
                xError =  AWS_IOT_LARGE_OBJECT_TRANSFER_INTERNAL_ERROR;
            }
        }
    }

    return xError;
}

 void prvDestroySession( AwsIotLargeObjectSessionType_t xType,
                         AwsIotLargeObjectSession_t *pxSession )

 {
     BaseType_t xStatus;

     if( xType == AWS_IOT_LARGE_OBJECT_SESSION_SEND )
     {
         if( xTimerIsTimerActive( pxSession->xSend.xRetransmitTimer ) == pdTRUE )
         {
             xStatus = xTimerStop( pxSession->xSend.xRetransmitTimer, portMAX_DELAY );
             configASSERT( xStatus == pdTRUE );
         }

         xStatus = xTimerDelete( pxSession->xSend.xRetransmitTimer, portMAX_DELAY );
         configASSERT( xStatus == pdTRUE );

         memset( pxSession, 0x00, sizeof( AwsIotLargeObjectSession_t ) );
     }
     else
     {
         if( pxSession->xRecv.pucBuffer != NULL )
         {
             AwsIotNetwork_Free( pxSession->xRecv.pucBuffer );
         }

         if( xTimerIsTimerActive( pxSession->xRecv.xAckTimer ) == pdTRUE )
         {
             {
                 xStatus = xTimerStop( pxSession->xRecv.xAckTimer, portMAX_DELAY );
                 configASSERT( xStatus == pdTRUE );
             }
         }

         xStatus = xTimerDelete( pxSession->xRecv.xAckTimer, portMAX_DELAY );
         configASSERT( xStatus == pdTRUE );
     }
 }


AwsIotLargeObjectTransferError_t prxOpenReceiveSession( AwsIotLargeObjectReceiveSession_t *pxSession, uint16_t usSessionId )
{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;

    pxSession->usSessionID = usSessionId;
    pxSession->xOffset = 0;
    pxSession->xBufferLength = 0;
    pxSession->usNumBlocksReceived = 0;
    pxSession->usNumWindowBlocks = pxSession->usWindowSize;
    pxSession->xWindowType = AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;
    pxSession->usRetriesLeft = pxSession->usNumRetries;
    pxSession->xLastWindow = pdFALSE;
    /** Reset the bitmap to all 1 **/
    memset( pxSession->ucBlockBitMap, 0xFF, sizeof( pxSession->ucBlockBitMap ) );
    pxSession->xState = eAwsIotLargeObjectTransferOpen;

    return xError;
}

AwsIotLargeObjectTransferError_t prxOpenSendSession( AwsIotLargeObjectSendSession_t *pxSession, uint16_t usSessionId, const uint8_t *pucObject, size_t xObjectLength )
{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
    BaseType_t xResult;

    pxSession->usSessionID = usSessionId;
    pxSession->pucObject = pucObject;
    pxSession->xObjectLength = xObjectLength;
    pxSession->xOffset = 0;
    pxSession->xWindowType = AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;
    pxSession->usRetriesLeft = pxSession->usNumRetries;

    xError = prxSendWindow( pxSession );

    if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
    {
        xResult = xTimerStart( pxSession->xRetransmitTimer, portMAX_DELAY );
        configASSERT( xResult == pdTRUE );
        pxSession->xState = eAwsIotLargeObjectTransferOpen;
    }

    return xError;
}

static BaseType_t prxIsBlockReceived( const uint8_t *pucBitMap, uint16_t usBlockNum )
{
    BaseType_t xRet = pdFALSE;
    uint16_t ulIndex = ( usBlockNum >> 3 ); /* Divide by 8 */
    uint8_t usBits   = ( usBlockNum & 0x7 );
    uint8_t ucValue =  pucBitMap[ ulIndex ];

    if( ( ucValue & ( 0x1 << usBits ) ) == 0 )
    {
        xRet = pdTRUE;
    }

    return xRet;
}

static void prvSetBlockReceived( uint8_t *pucBitMap, uint16_t usBlockNum )
{
    uint16_t usIndex = ( usBlockNum >> 3 ); /* Divide by 8 */
    uint8_t ucBits   = ( usBlockNum & 0x7 );
    uint8_t ucValue = pucBitMap[ usIndex ];
    pucBitMap[ usIndex ] = ucValue & ( ~( 0x1 << ucBits ) );
}

static AwsIotLargeObjectTransferError_t prxRetransmitMissingBlocks(
        AwsIotLargeObjectSendSession_t *pxSession,
        const uint8_t *pucBitMap,
        size_t xBitMapLength )
{
    size_t xBlockOffset, xBlockLength;
    uint16_t usBlockNum ;
    const uint8_t *pucBlock;
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
    BaseType_t xLastBlock = pdFALSE;
    AwsIotLargeObjectTransferContext_t *pxContext = (  AwsIotLargeObjectTransferContext_t* ) ( pxSession->pvContext );

    for ( usBlockNum = 0; ( usBlockNum < pxSession->usWindowSize ) && ( !xLastBlock ); usBlockNum++ )
    {
        if( prxIsBlockReceived( pucBitMap, usBlockNum ) == pdTRUE )
        {
            xBlockOffset = pxSession->xOffset + ( usBlockNum * pxSession->usBlockSize );
            xBlockLength = pxSession->usBlockSize;

            if( ( xBlockOffset + xBlockLength ) >= pxSession->xObjectLength )
            {
                xBlockLength = ( pxSession->xObjectLength - xBlockOffset );
                xLastBlock = pdTRUE;
            }

            pucBlock = ( pxSession->pucObject + xBlockOffset );

            xError = prxSendBlock( &pxContext->xNetworkIface,
                                   pxSession->usSessionID,
                                   usBlockNum,
                                   xLastBlock,
                                   pdFALSE,
                                   pxSession->xWindowType,
                                   pucBlock,
                                   xBlockLength );

            if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                break;
            }
        }
    }

    return xError;
}

static void prvProcessBlock( AwsIotLargeObjectReceiveSession_t* pxSession, const uint8_t* pucBlock, size_t xLength )
{
    AwsIotLargeObjectTransferContext_t *pxContext = ( AwsIotLargeObjectTransferContext_t * ) ( pxSession->pvContext );

    /** Block parameters **/
    uint16_t usBlockNum = _BLOCK_NUMBER( pucBlock );
    uint8_t ucFlags = _FLAGS( pucBlock );
    size_t xDataLength = _BLOCK_LENGTH( xLength );
    const uint8_t* pucBlockData = _BLOCK_DATA( pucBlock );
    BaseType_t xLastBlock = ( ( ucFlags & _LAST_BLOCK_MASK ) == _LAST_BLOCK_MASK );
    AwsIotLargeObjectWindowType_t xWindowType;

    size_t xBlockOffset;
    BaseType_t xTimerStatus;
    AwsIotLargeObjectTransferError_t xError;


    xWindowType =
            ( ( ucFlags & _ODD_WINDOW_MASK ) == _ODD_WINDOW_MASK ) ? AWS_IOT_LARGE_OBJECT_WINDOW_ODD : AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;

    if( xWindowType == pxSession->xWindowType )
    {

        if( usBlockNum < pxSession->usWindowSize )
        {
            if ( prxIsBlockReceived( pxSession->ucBlockBitMap, usBlockNum ) == pdFALSE )
            {
                /** Set the block number in the bitmap **/
                prvSetBlockReceived( pxSession->ucBlockBitMap, usBlockNum );

                /** Copy the block data into the block offset within the buffer **/
                xBlockOffset = usBlockNum * pxSession->usBlockSize;
                memcpy( ( pxSession->pucBuffer + xBlockOffset ),
                        pucBlockData,
                        xDataLength );

                /** Update the buffer length and the number of blocks received. **/
                pxSession->xBufferLength += xDataLength;
                pxSession->usNumBlocksReceived++;

                if( xLastBlock )
                {
                    pxSession->usNumWindowBlocks = ( usBlockNum + 1 );
                    pxSession->xLastWindow = pdTRUE;
                }

                if( pxSession->usNumBlocksReceived >= pxSession->usNumWindowBlocks )
                {
                    /** Receiver received all blocks in a window or upto the last block in the last window **/

                    /** Cancel the ACK timer **/
                    xTimerStatus = xTimerStop( pxSession->xAckTimer,  portMAX_DELAY );
                    configASSERT( xTimerStatus == pdTRUE );

                    /** Invoke the user callback for the window **/
                    pxContext->xReceiveCallback(
                            pxSession->usSessionID,
                            pxSession->pucBuffer,
                            pxSession->xBufferLength,
                            pxSession->xLastWindow );

                    /** Reset the buffer length back to zero **/
                    pxSession->xBufferLength = 0;

                    /** Reset the total number of window blocks and number of blocks received **/
                    pxSession->usNumBlocksReceived = 0;
                    pxSession->usNumWindowBlocks = pxSession->usWindowSize;



                    /** Reset the bitmap to all 1 **/
                    memset( pxSession->ucBlockBitMap, 0xFF, sizeof( pxSession->ucBlockBitMap ) );

                    /** Toggle window type **/
                    pxSession->xWindowType =
                            ( pxSession->xWindowType == AWS_IOT_LARGE_OBJECT_WINDOW_EVEN ) ? AWS_IOT_LARGE_OBJECT_WINDOW_ODD : AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;

                    /** Send an acknowledgment to the peer **/
                    xError = prxSendACK( &pxContext->xNetworkIface,
                                         pxSession->usSessionID,
                                         AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS,
                                         NULL,
                                         0 );

                    if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                    {
                        AwsIotLogWarn( "Failed to send acknowledgment for session id: %d", pxSession->usSessionID );

                    }
                    else
                    {
                        if( pxSession->xLastWindow== pdTRUE )
                        {
                            prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE, pdFALSE );

                        }
                    }
                }
            }
            else
            {
                AwsIotLogInfo( "Duplicate block received for session id: %d, blockNum: %d, ignoring..", pxSession->usSessionID, usBlockNum );
            }

        }
        else
        {
            AwsIotLogError( "Invalid block received for session id: %d, blockNum: %d", pxSession->usSessionID, usBlockNum );
            prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE, pdFALSE );
            ( void ) prxSendACK( &pxContext->xNetworkIface,
                                 pxSession->usSessionID,
                                 AWS_IOT_LARGE_OBJECT_TRANSFER_INVALID_PACKET,
                                 NULL,
                                 0 );
        }
    }
    else
    {
        AwsIotLogInfo( "Previous window received for session id: %d, windowType: %d", pxSession->usSessionID, xWindowType );

        /** Send an acknowledgment with wrong window error code **/
        xError = prxSendACK( &pxContext->xNetworkIface,
                             pxSession->usSessionID,
                             AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_WRONG_WINDOW,
                             NULL,
                             0 );

        if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
        {
            AwsIotLogWarn( "Failed to send acknowledgment for session id: %d", pxSession->usSessionID );
        }
    }
}

static void prvProcessACK( AwsIotLargeObjectSendSession_t* pxSession, const uint8_t* pucACK, size_t xLength )
{
    AwsIotLargeObjectTransferError_t xErrorCode, xResult;
    const uint8_t *pucBitMap;
    uint16_t usBitMapLen;
    BaseType_t xStatus;


    if( xLength < _MIN_ACK_LENGTH )
    {
        AwsIotLogError("Invalid packet received for session id :%d", pxSession->usSessionID );
        prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession,
                                             AWS_IOT_LARGE_OBJECT_SESSION_SEND,
                                             pdFALSE );
    }
    else
    {

        /** Stop the Retransmit timer **/
        xStatus = xTimerStop( pxSession->xRetransmitTimer, portMAX_DELAY );
        configASSERT( xStatus == pdTRUE );


        /** Set the retry to maximum limit **/
        pxSession->usRetriesLeft = pxSession->usNumRetries;

        /** Get Error code from the receiver **/
        xErrorCode = ( AwsIotLargeObjectTransferError_t ) ( _ERROR_CODE( pucACK ) );

        if( xErrorCode == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
        {
            /** Check if there are any missing blocks **/
            usBitMapLen = _BITMAP_LEN( xLength );

            /** If missing blocks found, retransmit them **/
            if( usBitMapLen > 0 )
            {
                pucBitMap = _BITMAP( pucACK );

                xResult = prxRetransmitMissingBlocks(
                        pxSession,
                        pucBitMap,
                        usBitMapLen );

                if( xResult != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                {
                    AwsIotLogError( "Failed to re-transmit missing blocks for session id %d", pxSession->usSessionID );
                    prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession,
                                     AWS_IOT_LARGE_OBJECT_SESSION_SEND,
                                     pdTRUE );
                }
            }
            else
            {
                /** There are no missing blocks. Send the next window. **/
                pxSession->xOffset = _INCR_OFFSET(
                        pxSession->xOffset,
                        pxSession->usWindowSize,
                        pxSession->usBlockSize );

                /** Toggle window **/
                pxSession->xWindowType =
                        ( pxSession->xWindowType == AWS_IOT_LARGE_OBJECT_WINDOW_EVEN ) ? AWS_IOT_LARGE_OBJECT_WINDOW_ODD : AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;

                xResult = prxSendWindow( pxSession );

                if( xResult != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                {
                    prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdTRUE );
                }
            }
        }
        else if( xErrorCode == AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_WRONG_WINDOW )
        {
            /** Receiver acknowledged a previous window. Transmit the next window. **/
            pxSession->xOffset = _INCR_OFFSET(
                    pxSession->xOffset,
                    pxSession->usWindowSize,
                    pxSession->usBlockSize );

            /** Toggle window **/
            pxSession->xWindowType =
                               ( pxSession->xWindowType == AWS_IOT_LARGE_OBJECT_WINDOW_EVEN ) ? AWS_IOT_LARGE_OBJECT_WINDOW_ODD : AWS_IOT_LARGE_OBJECT_WINDOW_EVEN;

            /** Send the next window **/
            xResult = prxSendWindow( pxSession );

            if( xResult != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                /** Send failed. Mark the session as resumable */
                prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdTRUE );
            }
        }
        else
        {
            /** Mark all other cases as failed session **/
            prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdFALSE );
        }

        if( pxSession->xState == eAwsIotLargeObjectTransferOpen )
        {
            /** Start the Retransmit timer, if the session is still active **/
            xStatus = xTimerStart( pxSession->xRetransmitTimer, portMAX_DELAY );
            configASSERT( xStatus == pdTRUE );
        }

    }
}


static void prvNetworkReceiveCallback(
        void * pvContext,
        const void * pvReceivedData,
        size_t xDataLength )
{
    AwsIotLargeObjectTransferContext_t* pxContext = ( AwsIotLargeObjectTransferContext_t * ) pvContext;
    uint8_t *pucData = ( uint8_t * ) pvReceivedData;
    uint16_t usSessionID, usIndex;
    AwsIotLargeObjectSession_t *pxSession;
    BaseType_t xSessionFound = pdFALSE, xFreeSession = pdFALSE;
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;

    configASSERT( pxContext != NULL );
    usSessionID = _SESSION_ID( pucData );

    for( usIndex = 0; usIndex < pxContext->usNumSendSessions; usIndex++ )
    {
        pxSession = & ( pxContext->pxSendSessions[ usIndex ] );
        if( pxSession->xSend.usSessionID == usSessionID )
        {
            if( pxSession->xSend.xState == eAwsIotLargeObjectTransferOpen )
            {
                prvProcessACK( &pxSession->xSend, pucData, xDataLength );
            }
            else
            {
                AwsIotLogWarn("Packet received for invalid session state, id %d, state: %d", usSessionID, pxSession->xSend.xState );
                ( void ) prxSendACK( &pxContext->xNetworkIface,
                                     usSessionID,
                                     AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_ABORTED,
                                     NULL,
                                     0 );

            }
            xSessionFound = pdTRUE;
            break;
        }
    }

    if( !xSessionFound )
    {
        for( usIndex = 0; usIndex < pxContext->usNumRecvSessions; usIndex++ )
        {
            pxSession = & ( pxContext->pxRecvSessions[ usIndex ] );
            if( pxSession->xRecv.usSessionID == usSessionID )
            {
                if( ( pxSession->xRecv.xState == eAwsIotLargeObjectTransferOpen ) ||
                     ( pxSession->xRecv.xState ==  eAwsIotLargeObjectTransferResumable ) )
                {
                    prvProcessBlock( &pxSession->xRecv, pucData, xDataLength );
                }
                else
                {
                    AwsIotLogWarn("Packet received for invalid session state, id %d, state: %d", usSessionID, pxSession->xRecv.xState );;
                    ( void ) prxSendACK( &pxContext->xNetworkIface,
                                         usSessionID,
                                         AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_ABORTED,
                                         NULL,
                                         0 );

                }
                xSessionFound = pdTRUE;
                break;
            }
        }
    }

    if( !xSessionFound )
    {
        for( usIndex = 0; usIndex < pxContext->usNumRecvSessions; usIndex++ )
        {
            pxSession = & ( pxContext->pxRecvSessions[ usIndex ]);
            if( _SESSION_FREE( pxSession->xRecv.xState ) )
            {
                xFreeSession = pdTRUE;
                break;
            }
        }

        if( xFreeSession )
        {

            xError = prxOpenReceiveSession(
                                   &pxSession->xRecv,
                                   usSessionID );

            if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                prvProcessBlock( &pxSession->xRecv, pucData, xDataLength );
            }
            else
            {
                AwsIotLogError( "Cannot create a new session for session id %d, error = %d",
                                usSessionID,
                                xError );

                ( void ) prxSendACK( &pxContext->xNetworkIface,
                                     usSessionID,
                                     AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_NOT_FOUND,
                                     NULL,
                                     0 );
            }
        }
        else
        {
            AwsIotLogError( "Cannot create a new session for session id %d, max sessions reached.", usSessionID );
            ( void ) prxSendACK( &pxContext->xNetworkIface,
                                 usSessionID,
                                 AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_NOT_FOUND,
                                 NULL,
                                 0 );

        }
    }
}


AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Init(
        AwsIotLargeObjectTransferContext_t* pxContext,
        uint16_t usNumSendSessions,
        uint16_t usNumRecvSessions )
{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
    AwsIotLargeObjectTransferNetworkIface_t *pxNetworkIface;
    uint16_t usIdx;
    AwsIotLargeObjectSession_t *pxSession;

    if( pxContext != NULL )
    {
        pxNetworkIface = &( pxContext->xNetworkIface );

        pxNetworkIface->setNetworkReceiveCallback(
                pxNetworkIface->pvConnection,
                pxContext,
                prvNetworkReceiveCallback );

        /** Initialize send sessions **/

        pxContext->pxSendSessions = AwsIotNetwork_Malloc( sizeof( AwsIotLargeObjectSession_t ) * usNumSendSessions );
        if( pxContext->pxSendSessions != NULL )
        {

            pxContext->usNumSendSessions = usNumSendSessions;

            for( usIdx = 0; usIdx < pxContext->usNumSendSessions; usIdx++ )
            {
                pxSession = &pxContext->pxSendSessions[usIdx];
                memset( pxSession, 0x00, sizeof( AwsIotLargeObjectSession_t ));
                xError = prxInitSession( pxContext, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pxSession );
                if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                {
                    break;
                }
            }
        }
        else
        {
            xError = AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY;
        }

        if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
        {

            pxContext->pxRecvSessions = AwsIotNetwork_Malloc( sizeof( AwsIotLargeObjectSession_t ) * usNumRecvSessions );
            if( pxContext->pxRecvSessions != NULL )
            {

                pxContext->usNumRecvSessions = usNumRecvSessions;

                for( usIdx = 0; usIdx < pxContext->usNumRecvSessions; usIdx++ )
                {
                    pxSession = &pxContext->pxRecvSessions[usIdx];
                    memset( pxSession, 0x00, sizeof( AwsIotLargeObjectSession_t ));
                    xError = prxInitSession( pxContext, AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE, pxSession );
                    if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                    {
                        break;
                    }
                }
            }
            else
            {
                xError = AWS_IOT_LARGE_OBJECT_TRANSFER_NO_MEMORY;
            }
        }
    }
    else
    {
        xError = AWS_IOT_LARGE_OBJECT_TRANSFER_INVALID_PARAM;
    }

    return xError;
}

AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Send(
        AwsIotLargeObjectTransferContext_t* pxContext,
        const uint8_t *pucObject,
        const size_t xObjectLength,
        uint16_t* pusSessionID )
{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_MAX_SESSIONS_REACHED;
    uint32_t usIndex;
    AwsIotLargeObjectSession_t* pxSession;

    for( usIndex = 0; usIndex < pxContext->usNumSendSessions; usIndex++ )
    {
        pxSession = & ( pxContext->pxSendSessions[ usIndex ]);
        if( _SESSION_FREE( pxSession->xSend.xState ) )
        {
            xError = prxOpenSendSession(
                    &pxSession->xSend,
                    usIndex+1,
                    pucObject,
                    xObjectLength );
            if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
               *pusSessionID = usIndex;
            }

            break;
        }
    }

    return xError;
}

AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_Resume( AwsIotLargeObjectTransferContext_t* pxContext, uint16_t usSessionID )
{
    AwsIotLargeObjectTransferError_t xError = AWS_IOT_LARGE_OBJECT_TRANSFER_INVALID_PARAM;
    AwsIotLargeObjectSendSession_t *pxSession;
    uint16_t usIndex;
    BaseType_t xResult;


    for( usIndex = 0; usIndex < pxContext->usNumSendSessions; usIndex++ )
    {
        pxSession = & ( pxContext->pxSendSessions[ usIndex ].xSend);
        if( ( pxSession->usSessionID == usSessionID )
                && ( pxSession->xState == eAwsIotLargeObjectTransferResumable )
                && ( pxSession->xOffset < pxSession->xObjectLength ) )
        {
            xError = prxSendWindow( pxSession );
            if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                xResult = xTimerStart( pxSession->xRetransmitTimer, portMAX_DELAY );
                configASSERT( xResult == pdTRUE );
                pxSession->xState = eAwsIotLargeObjectTransferOpen;
            }
        }
    }

    return xError;
}

AwsIotLargeObjectTransferError_t AwsIotLargeObjectTransfer_CloseSession( AwsIotLargeObjectTransferContext_t* pxContext,
                                                                         AwsIotLargeObjectSessionType_t xType,
                                                                         uint16_t usSessionID )
{
    uint16_t usIdx;
    AwsIotLargeObjectTransferError_t xRet = AWS_IOT_LARGE_OBJECT_TRANSFER_SESSION_NOT_FOUND;
    AwsIotLargeObjectSession_t *pxSession;

    if( xType == AWS_IOT_LARGE_OBJECT_SESSION_SEND )
    {
        for( usIdx = 0; usIdx< pxContext->usNumSendSessions; usIdx++ )
        {
            pxSession = &pxContext->pxSendSessions[ usIdx ];
            if( pxSession->xSend.usSessionID == usSessionID )
            {
                prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_SEND, pdFALSE );
                xRet = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
            }
        }
    }
    else
    {
        for( usIdx = 0; usIdx< pxContext->usNumRecvSessions; usIdx++ )
         {
            pxSession = &pxContext->pxRecvSessions[ usIdx ];
             if( pxSession->xRecv.usSessionID == usSessionID )
             {
                 prvCloseSession( ( AwsIotLargeObjectSession_t * ) pxSession, AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE, pdFALSE );
                 xRet = AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS;
             }
         }
    }

    return xRet;

}

void AwsIotLargeObjectTransfer_Destroy( AwsIotLargeObjectTransferContext_t* pxContext )
{
    uint16_t usIdx;
    AwsIotLargeObjectSession_t *pxSession;

    if( pxContext->pxSendSessions != NULL )
    {
        for( usIdx = 0; usIdx < pxContext->usNumSendSessions; usIdx++ )
        {
            pxSession = &pxContext->pxSendSessions[ usIdx ];
            prvDestroySession( AWS_IOT_LARGE_OBJECT_SESSION_SEND, pxSession );
        }

        AwsIotNetwork_Free( pxContext->pxSendSessions );
        pxContext->pxSendSessions = NULL;
    }

    if( pxContext->pxRecvSessions != NULL )
    {

        for( usIdx = 0; usIdx < pxContext->usNumRecvSessions; usIdx++ )
        {
            pxSession = &pxContext->pxRecvSessions[ usIdx ];
            prvDestroySession( AWS_IOT_LARGE_OBJECT_SESSION_RECIEVE, pxSession );
        }
        AwsIotNetwork_Free( pxContext->pxRecvSessions );
        pxContext->pxRecvSessions = NULL;
    }
}
