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
 * @file aws_iot_mqtt_ble.c
 * @brief GATT service for transferring MQTT packets over BLE
 */

/* Build using a config header, if provided. */
#ifdef AWS_IOT_CONFIG_FILE
    #include AWS_IOT_CONFIG_FILE
#endif

#include "iot_ble_config.h"
#include "aws_iot_mqtt_ble.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include "aws_json_utils.h"

#define mqttBLECHAR_CONTROL_UUID_TYPE \
{ \
    .uu.uu128 = mqttBLECHAR_CONTROL_UUID,\
    .ucType   = eBTuuidType128 \
}
#define mqttBLECHAR_TX_MESG_UUID_TYPE \
{  \
    .uu.uu128 = mqttBLECHAR_TX_MESG_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_RX_MESG_UUID_TYPE \
{\
    .uu.uu128 = mqttBLECHAR_RX_MESG_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHARmqttBLE_LARGE_OBJECT_MTU_SIZE_UUID_TYPE \
{\
    .uu.uu128 = mqttBLECHARmqttBLE_LARGE_OBJECT_MTU_SIZE_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHARmqttBLE_LARGE_OBJECT_WINDOW_SIZE_UUID_TYPE \
{\
    .uu.uu128 = mqttBLECHARmqttBLE_LARGE_OBJECT_WINDOW_SIZE_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_LARGE_OBJECT_TIMEOUT_UUID_TYPE \
{\
    .uu.uu128 = mqttBLECHAR_LARGE_OBJECT_TIMEOUT_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_LARGE_OBJECT_RETRIES_UUID_TYPE \
{\
    .uu.uu128 = mqttBLECHAR_LARGE_OBJECT_RETRIES_UUID,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE1 \
{\
    .uu.uu128 = mqttBLECHAR_TX_LARGE_MESG_UUID1,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE2 \
{\
    .uu.uu128 = mqttBLECHAR_TX_LARGE_MESG_UUID2,\
    .ucType  = eBTuuidType128\
}
#define mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE3 \
{\
    .uu.uu128 = mqttBLECHAR_TX_LARGE_MESG_UUID3,\
    .ucType  = eBTuuidType128\
}
#define mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE4 \
{\
    .uu.uu128 = mqttBLECHAR_TX_LARGE_MESG_UUID4,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE1 \
{\
    .uu.uu128 = mqttBLECHAR_RX_LARGE_MESG_UUID1,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE2 \
{\
    .uu.uu128 = mqttBLECHAR_RX_LARGE_MESG_UUID2,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE3 \
{\
    .uu.uu128 = mqttBLECHAR_RX_LARGE_MESG_UUID3,\
    .ucType  = eBTuuidType128\
}

#define mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE4 \
{\
    .uu.uu128 = mqttBLECHAR_RX_LARGE_MESG_UUID4,\
    .ucType  = eBTuuidType128\
}
#define mqttBLECCFG_UUID_TYPE \
{\
    .uu.uu16 = mqttBLECCFG_UUID,\
    .ucType  = eBTuuidType16\
}
/**
 * @brief UUID for Device Information Service.
 *
 * This UUID is used in advertisement for the companion apps to discover and connect to the device.
 */
#define mqttBLESERVICE_UUID_TYPE \
{ \
    .uu.uu128 = mqttBLESERVICE_UUID,\
    .ucType   = eBTuuidType128\
}


static uint16_t pusHandlesBuffer[mqttBLEMAX_SVC_INSTANCES][eMQTTBLE_NUMBER];

#define CHAR_HANDLE( svc, ch_idx )        ( ( svc )->pusHandlesBuffer[ch_idx] )
#define CHAR_UUID( svc, ch_idx )          ( ( svc )->pxBLEAttributes[ch_idx].xCharacteristic.xUuid )
#define DESCR_HANDLE( svc, descr_idx )    ( ( svc )->pusHandlesBuffer[descr_idx] )


/*-----------------------------------------------------------------------------------------------------*/
static MqttBLEService_t xMqttBLEServices[ mqttBLEMAX_SVC_INSTANCES ] = { 0 };
static BTService_t xBLEServices[ mqttBLEMAX_SVC_INSTANCES ] = { 0 };

static const BTAttribute_t pxAttributeTable[] = {
     {
          .xServiceUUID = mqttBLESERVICE_UUID_TYPE
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_CONTROL_UUID_TYPE,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropRead | eBTPropWrite )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_TX_MESG_UUID_TYPE,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM ),
              .xProperties = (  eBTPropRead | eBTPropNotify  )
          }
     },
     {
         .xAttributeType = eBTDbDescriptor,
         .xCharacteristicDescr =
         {
             .xUuid = mqttBLECCFG_UUID_TYPE,
             .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_RX_MESG_UUID_TYPE,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropRead | eBTPropWrite )
         }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHARmqttBLE_LARGE_OBJECT_MTU_SIZE_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropRead  )
             }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHARmqttBLE_LARGE_OBJECT_WINDOW_SIZE_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropRead  )
             }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHAR_LARGE_OBJECT_TIMEOUT_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropRead  )
             }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHAR_LARGE_OBJECT_RETRIES_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropRead  )
             }
     },

     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
              .xUuid = mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE1,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
          }
     },
     {
         .xAttributeType = eBTDbDescriptor,
         .xCharacteristicDescr =
         {
             .xUuid = mqttBLECCFG_UUID_TYPE,
             .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE2,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
          }
     },
     {
         .xAttributeType = eBTDbDescriptor,
         .xCharacteristicDescr =
         {
             .xUuid = mqttBLECCFG_UUID_TYPE,
             .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE3,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
          }
     },
     {
         .xAttributeType = eBTDbDescriptor,
         .xCharacteristicDescr =
         {
             .xUuid = mqttBLECCFG_UUID_TYPE,
             .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_TX_LARGE_MESG_UUID_TYPE4,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
          }
     },
     {
         .xAttributeType = eBTDbDescriptor,
         .xCharacteristicDescr =
         {
             .xUuid = mqttBLECCFG_UUID_TYPE,
             .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
          }
     },
     {
         .xAttributeType = eBTDbCharacteristic,
         .xCharacteristic =
         {
              .xUuid = mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE1,
              .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
              .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
          }
     },
     {
              .xAttributeType = eBTDbDescriptor,
              .xCharacteristicDescr =
              {
                  .xUuid = mqttBLECCFG_UUID_TYPE,
                  .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
              }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE2,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
             }
     },
     {
             .xAttributeType = eBTDbDescriptor,
             .xCharacteristicDescr =
             {
                     .xUuid = mqttBLECCFG_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
             }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE3,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
             }
     },
     {
             .xAttributeType = eBTDbDescriptor,
             .xCharacteristicDescr =
             {
                     .xUuid = mqttBLECCFG_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
             }
     },
     {
             .xAttributeType = eBTDbCharacteristic,
             .xCharacteristic =
             {
                     .xUuid = mqttBLECHAR_RX_LARGE_MESG_UUID_TYPE4,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM ),
                     .xProperties = ( eBTPropWriteNoResponse | eBTPropNotify )
             }
     },
     {
             .xAttributeType = eBTDbDescriptor,
             .xCharacteristicDescr =
             {
                     .xUuid = mqttBLECCFG_UUID_TYPE,
                     .xPermissions = ( IOT_BLE_CHAR_READ_PERM | IOT_BLE_CHAR_WRITE_PERM )
             }
     }
};


/**
 * Variable stores the MTU Size for the BLE connection.
 */
static uint16_t usBLEConnMTU;

/*------------------------------------------------------------------------------------------------------*/

/*
 * @brief Creates and starts  a GATT service instance.
 */
static BaseType_t prvInitServiceInstance( BTService_t * pxService );

/*
 * @brief Gets an MQTT proxy service instance given a GATT service.
 */
static MqttBLEService_t * prxGetServiceInstance( uint16_t usHandle );

/*
 * @brief Callback to register for events (read) on TX message characteristic.
 */
void vTXMesgCharCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief Callback to register for events (write) on RX message characteristic.
 *
 */
void vRXMesgCharCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief Callback to register for events (read) on TX large message characteristic.
 * Buffers a large message and sends the message in chunks of size MTU at a time as response
 * to the read request. Keeps the buffer until last message is read.
 */
void vTXLargeMesgCharCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief Callback to register for events (write) on RX large message characteristic.
 * Copies the individual write packets into a buffer untill a packet less than BLE MTU size
 * is received. Sends the buffered message to the MQTT layer.
 */
void vRXLargeMesgCharCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief Callback for Client Characteristic Configuration Descriptor events.
 */
void vClientCharCfgDescrCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief This is the callback to the control characteristic. It is used to toggle ( turn on/off)
 * MQTT proxy service by the BLE IOS/Android app.
 */
void vToggleMQTTService( IotBleAttributeEvent_t * pEventParam );


static void vLOTMTUCharCallback( IotBleAttributeEvent_t * pEventParam );
static void vLOTWindowCharCallback( IotBleAttributeEvent_t * pEventParam );
static void vLOTRetriesCharCallback( IotBleAttributeEvent_t * pEventParam );
static void vLOTTimeoutCharCallback( IotBleAttributeEvent_t * pEventParam );

/*
 * @brief Resets the send and receive buffer for the given MQTT Service.
 * Any pending MQTT connection should be closed or the service should be disabled
 * prior to resetting the buffer.
 *
 * @param[in]  pxService Pointer to the MQTT service.
 */
void prvCloseSessions( MqttBLEService_t* pxService );

/*
 * @brief Callback for BLE connect/disconnect. Toggles the Proxy state to off on a BLE disconnect.
 */
static void vConnectionCallback( BTStatus_t xStatus,
                                 uint16_t connId,
                                 bool bConnected,
                                 BTBdaddr_t * pxRemoteBdAddr );

/*
 * @brief Callback to receive notification when the MTU of the BLE connection changes.
 */
static void vMTUChangedCallback( uint16_t connId,
                                 uint16_t usMtu );


static void vLOTReceiveCallback (
        void *pvConnection,
        uint16_t usSessionID,
        const uint8_t * pucData,
        size_t xDataLength,
        BaseType_t xComplete );

static size_t vLOTSendBlock(
       void * pvConnection,
       const void * const pvMessage ,
       size_t xLength );

static int32_t vLOTSetNetworkReceiveCallback (
           void * pvConnection,
           void* pvContext,
           AwsIotLargeObjectTransferReceiveCallback_t  xCallback);

/*
 * @brief stdio.h conflict with posix library on some platform so using extern snprintf so not to include it.
 */
extern int snprintf( char *, size_t, const char *, ... );
/*------------------------------------------------------------------------------------*/

static const IotBleAttributeEventCallback_t pxCallBackArray[eMQTTBLE_NUMBER] =
        {
  NULL,
  vToggleMQTTService,
  vTXMesgCharCallback,
  vClientCharCfgDescrCallback,
  vRXMesgCharCallback,
  vLOTMTUCharCallback,
  vLOTWindowCharCallback,
  vLOTTimeoutCharCallback,
  vLOTRetriesCharCallback,
  vTXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vTXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vTXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vTXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vRXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vRXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vRXLargeMesgCharCallback,
  vClientCharCfgDescrCallback,
  vRXLargeMesgCharCallback,
  vClientCharCfgDescrCallback
 };

/*-----------------------------------------------------------*/

static MqttBLEService_t * prxGetServiceInstance( uint16_t usHandle )
{
    uint8_t ucId;
    MqttBLEService_t * pxMQTTSvc = NULL;

    for( ucId = 0; ucId < mqttBLEMAX_SVC_INSTANCES; ucId++ )
    {
        /* Check that the handle is included in the service. */
        if(( usHandle > pusHandlesBuffer[ucId][0] )&&
          (usHandle <= pusHandlesBuffer[ucId][eMQTTBLE_NUMBER - 1]))
        {
            pxMQTTSvc = &xMqttBLEServices[ ucId ];
            break;
        }
    }

    return pxMQTTSvc;
}

/*-----------------------------------------------------------*/

static BaseType_t prxSendNotification( MqttBLEService_t * pxMQTTService,
                                MQTTBLEAttributes_t xCharacteristic,
                                uint8_t * pucData,
                                size_t xLen )
{
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    BaseType_t xStatus = pdFALSE;

    xAttrData.handle = CHAR_HANDLE( pxMQTTService->pxServicePtr, xCharacteristic );
    xAttrData.uuid = CHAR_UUID( pxMQTTService->pxServicePtr, xCharacteristic );
    xAttrData.pData = pucData;
    xAttrData.size = xLen;
    xResp.pAttrData = &xAttrData;
    xResp.attrDataOffset = 0;
    xResp.eventStatus = eBTStatusSuccess;
    xResp.rspErrorStatus = eBTRspErrorNone;

    if( IotBle_SendIndication( &xResp, pxMQTTService->usBLEConnId, false ) == eBTStatusSuccess )
    {
        xStatus = pdTRUE;
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

static BaseType_t prvInitServiceInstance( BTService_t * pxService )
{
    BTStatus_t xStatus;
    BaseType_t xResult = pdFAIL;
    IotBleEventsCallbacks_t xCallback;

      xStatus = IotBle_CreateService( pxService, (IotBleAttributeEventCallback_t *)pxCallBackArray );
    if( xStatus == eBTStatusSuccess )
    {
          xResult = pdPASS;
    }

      if( xResult == pdPASS )
{
              xCallback.pConnectionCb = vConnectionCallback;

              if( IotBle_RegisterEventCb( eBLEConnection, xCallback ) != eBTStatusSuccess )
    {
                      xResult = pdFAIL;
    }
    }

    return xResult;
}

void prvCloseSessions( MqttBLEService_t* pxService )
{
    if( pxService->xConnection.usLOTSendUUID != 0 )
    {
        AwsIotLargeObjectTransfer_CloseSession( &pxService->xConnection.xLOTContext,
                                                AWS_IOT_LARGE_OBJECT_SESSION_SEND,
                                                pxService->xConnection.usLOTSendUUID );
        pxService->xConnection.usLOTSendUUID = 0;
    }

}

/*-----------------------------------------------------------*/

void vToggleMQTTService( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxService;
    jsmntok_t xTokens[ mqttBLEMAX_TOKENS ];
    int16_t sNumTokens, sProxyEnable;
    BaseType_t xResult;
    char cMsg[ mqttBLESTATE_MSG_LEN + 1 ];

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;
        xResp.pAttrData->handle = pxWriteParam->attrHandle;
        pxService = prxGetServiceInstance( pxWriteParam->attrHandle );
        configASSERT( ( pxService != NULL ) );

        if( !pxWriteParam->isPrep )
        {
            sNumTokens = JsonUtils_Parse( ( const char * ) pxWriteParam->pValue, pxWriteParam->length, xTokens, mqttBLEMAX_TOKENS );

            if( sNumTokens > 0 )
            {
                xResult = JsonUtils_GetInt16Value( ( const char * ) pxWriteParam->pValue, xTokens, sNumTokens, mqttBLESTATE, strlen( mqttBLESTATE ), &sProxyEnable );

                if( xResult == pdTRUE )
                {
                    if( sProxyEnable == pdTRUE )
                    {
                    	pxService->bIsEnabled = true;
                    }
                    else
                    {
                    	pxService->bIsEnabled = false;
                        prvCloseSessions( pxService );
                    }

                    xResp.eventStatus = eBTStatusSuccess;
                }
            }
        }

        if( pEventParam->xEventType == eBLEWrite )
        {
            xResp.pAttrData->pData = pxWriteParam->pValue;
            xResp.attrDataOffset = pxWriteParam->offset;
            xResp.pAttrData->size = pxWriteParam->length;
            IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
        }
    }
    else if( pEventParam->xEventType == eBLERead )
    {
        pxService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxService != NULL ) );

        xResp.pAttrData->handle =pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        xResp.pAttrData->pData = ( uint8_t * ) cMsg;
        xResp.attrDataOffset = 0;
        xResp.pAttrData->size = snprintf( cMsg, mqttBLESTATE_MSG_LEN, mqttBLESTATE_MESSAGE, pxService->bIsEnabled);
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}
/*-----------------------------------------------------------*/

void vTXMesgCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleReadEventParams_t * pxReadParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxReadParam = pEventParam->pParamRead;
        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.pAttrData->pData = NULL;
        xResp.pAttrData->size = 0;
        xResp.attrDataOffset = 0;
        xResp.eventStatus = eBTStatusSuccess;

        IotBle_SendResponse( &xResp, pxReadParam->connId, pxReadParam->transId );
    }
}

/*-----------------------------------------------------------*/

void vTXLargeMesgCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
      IotBleAttributeData_t xAttrData = { 0 };
      IotBleEventResponse_t xResp;
      MqttBLEService_t * pxService;

      xResp.pAttrData = &xAttrData;
      xResp.rspErrorStatus = eBTRspErrorNone;
      xResp.eventStatus = eBTStatusFail;
      xResp.attrDataOffset = 0;

      if( (  pEventParam->xEventType == eBLEWriteNoResponse ) )
      {
          pxWriteParam = pEventParam->pParamWrite;
          pxService = prxGetServiceInstance( pxWriteParam->attrHandle );
          configASSERT( ( pxService != NULL ) );

          xResp.pAttrData->handle = pxWriteParam->attrHandle;
          xResp.pAttrData->uuid = CHAR_UUID( pxService->pxServicePtr, eMQTTBLE_CHAR_TX_LARGE_MESG1 );

          if(( pxService->xConnection.pxMqttConnection != NULL ) &&
                  ( pxService->bIsEnabled ) )
          {
              pxService->xConnection.xLOTReceiveCallback(
                      pxService->xConnection.pvLOTReceiveContext,
                      pxWriteParam->pValue,
                      pxWriteParam->length );

              xResp.eventStatus = eBTStatusSuccess;

          }
      }
      else
      {
          configPRINTF(("ERROR, RX large should receive only write commands\n" ));
      }
}

/*-----------------------------------------------------------*/

void vRXLargeMesgCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxService;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( (  pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;
        pxService = prxGetServiceInstance( pxWriteParam->attrHandle );
        configASSERT( ( pxService != NULL ) );

        xResp.pAttrData->handle = pxWriteParam->attrHandle;
        xResp.pAttrData->uuid = CHAR_UUID( pxService->pxServicePtr, eMQTTBLE_CHAR_RX_LARGE_MESG1 );

        if(( pxService->xConnection.pxMqttConnection != NULL ) &&
				( pxService->bIsEnabled ) )
        {
        	pxService->xConnection.xLOTReceiveCallback(
        	        pxService->xConnection.pvLOTReceiveContext,
        	        pxWriteParam->pValue,
        	        pxWriteParam->length );

        	xResp.eventStatus = eBTStatusSuccess;

        }
    }
    else
    {
        configPRINTF(("ERROR, RX large should receive only write commands\n" ));
    }
}

/*-----------------------------------------------------------*/

void vRXMesgCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxService;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;
        pxService = prxGetServiceInstance( pxWriteParam->attrHandle );
        configASSERT( ( pxService != NULL ) );
        xResp.pAttrData->handle = pxWriteParam->attrHandle;
        xResp.pAttrData->uuid = CHAR_UUID( pxService->pxServicePtr, eMQTTBLE_CHAR_RX_MESG );

        if( ( !pxWriteParam->isPrep ) &&
               		( pxService->xConnection.pxMqttConnection != NULL ) &&
       				( pxService->bIsEnabled ) )
        {
        		( void ) AwsIotMqtt_ReceiveCallback( pxService->xConnection.pxMqttConnection,
        				( const void * ) pxWriteParam->pValue,
						0,
						pxWriteParam->length,
						NULL );
        		xResp.eventStatus = eBTStatusSuccess;
        }

        if( pEventParam->xEventType == eBLEWrite )
        {
            xResp.attrDataOffset = pxWriteParam->offset;
            xResp.pAttrData->pData = pxWriteParam->pValue;
            xResp.pAttrData->size = pxWriteParam->length;
            IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
        }
    }
}

/*-----------------------------------------------------------*/

void vClientCharCfgDescrCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleWriteEventParams_t * pxWriteParam;
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxMQTTService;
    uint16_t usCCFGValue;

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( ( pEventParam->xEventType == eBLEWrite ) || ( pEventParam->xEventType == eBLEWriteNoResponse ) )
    {
        pxWriteParam = pEventParam->pParamWrite;
        xResp.pAttrData->handle = pxWriteParam->attrHandle;
        pxMQTTService = prxGetServiceInstance( pxWriteParam->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );

        if( pxWriteParam->length == 2 )
        {
            usCCFGValue = ( pxWriteParam->pValue[ 1 ] << 8 ) | pxWriteParam->pValue[ 0 ];

            if( pEventParam->pParamWrite->attrHandle == pxMQTTService->pxServicePtr->pusHandlesBuffer[eMQTTBLE_CHAR_DESCR_TX_MESG])
            {
                 pxMQTTService->usCCFGEnabled = usCCFGValue;
            }else 
            {
                 pxMQTTService->usCCFGEnabled = usCCFGValue;
            }
            
            xResp.eventStatus = eBTStatusSuccess;
        }

        if( pEventParam->xEventType == eBLEWrite )
        {
            xResp.pAttrData->pData = pxWriteParam->pValue;
            xResp.pAttrData->size = pxWriteParam->length;
            xResp.attrDataOffset = pxWriteParam->offset;
            IotBle_SendResponse( &xResp, pxWriteParam->connId, pxWriteParam->transId );
        }
    }
    else if( pEventParam->xEventType == eBLERead )
    {
        pxMQTTService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );

        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        if( pEventParam->pParamWrite->attrHandle == pxMQTTService->pxServicePtr->pusHandlesBuffer[eMQTTBLE_CHAR_DESCR_TX_MESG])
        {
            xResp.pAttrData->pData = ( uint8_t * ) &pxMQTTService->usCCFGEnabled;
        }else 
        {
            xResp.pAttrData->pData = ( uint8_t * ) &pxMQTTService->usCCFGEnabled;
        }

        xResp.pAttrData->size = 2;
        xResp.attrDataOffset = 0;
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}


static void vLOTMTUCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxMQTTService;
    char cMesg[ 10 ] = { 0 };

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxMQTTService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );
        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        xResp.pAttrData->pData = ( uint8_t * ) cMesg;
        xResp.pAttrData->size = snprintf( cMesg, sizeof( cMesg ), "%d", mqttBLE_LARGE_OBJECT_BLOCK_SIZE );
        xResp.attrDataOffset = 0;
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}

static void vLOTWindowCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxMQTTService;
    char cMesg[ 10 ] = { 0 };

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxMQTTService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );
        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        xResp.pAttrData->pData = ( uint8_t * ) cMesg;
        xResp.pAttrData->size = snprintf( cMesg, sizeof( cMesg ), "%d", mqttBLE_LARGE_OBJECT_WINDOW_SIZE );
        xResp.attrDataOffset = 0;
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}


static void vLOTTimeoutCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxMQTTService;
    char cMesg[ 10 ] = { 0 };

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxMQTTService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );
        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        xResp.pAttrData->pData = ( uint8_t * ) cMesg;
        xResp.pAttrData->size = snprintf( cMesg, sizeof( cMesg ), "%d", mqttBLE_LARGE_OBJECT_WINDOW_INTERVAL_MS );
        xResp.attrDataOffset = 0;
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}

static void vLOTRetriesCharCallback( IotBleAttributeEvent_t * pEventParam )
{
    IotBleAttributeData_t xAttrData = { 0 };
    IotBleEventResponse_t xResp;
    MqttBLEService_t * pxMQTTService;
    char cMesg[ 10 ] = { 0 };

    xResp.pAttrData = &xAttrData;
    xResp.rspErrorStatus = eBTRspErrorNone;
    xResp.eventStatus = eBTStatusFail;
    xResp.attrDataOffset = 0;

    if( pEventParam->xEventType == eBLERead )
    {
        pxMQTTService = prxGetServiceInstance( pEventParam->pParamRead->attrHandle );
        configASSERT( ( pxMQTTService != NULL ) );
        xResp.pAttrData->handle = pEventParam->pParamRead->attrHandle;
        xResp.eventStatus = eBTStatusSuccess;
        xResp.pAttrData->pData = ( uint8_t * ) cMesg;
        xResp.pAttrData->size = snprintf( cMesg, sizeof( cMesg ), "%d", mqttBLE_LARGE_OBJECT_WINDOW_RETRIES );
        xResp.attrDataOffset = 0;
        IotBle_SendResponse( &xResp, pEventParam->pParamRead->connId, pEventParam->pParamRead->transId );
    }
}

/*-----------------------------------------------------------*/

static void vConnectionCallback( BTStatus_t xStatus,
                                 uint16_t connId,
                                 bool bConnected,
                                 BTBdaddr_t * pxRemoteBdAddr )
{
    uint8_t ucId;
    MqttBLEService_t *pxService;
    
    if( xStatus == eBTStatusSuccess )
    {
        for( ucId = 0; ucId < mqttBLEMAX_SVC_INSTANCES; ucId++ )
        {
            pxService = &xMqttBLEServices[ ucId ];
            if( bConnected == true )
            {
                pxService->usBLEConnId = connId;
            }
            else
            {
                configPRINTF( ( "Disconnect received for MQTT service instance %d\n", ucId ) );
                pxService->bIsEnabled = false;
                pxService->xConnection.pxMqttConnection = NULL;
                prvCloseSessions( pxService );
            }
        }
    }
}

/*-----------------------------------------------------------*/

static void vMTUChangedCallback( uint16_t connId,
                                 uint16_t usMtu )
{
	/* Change the MTU size for the connection */
	if( usBLEConnMTU != usMtu )
	{
		configPRINTF( ( "Changing MTU size for BLE connection from %d to %d\n", usBLEConnMTU, usMtu ) );
		usBLEConnMTU = usMtu;
	}

}

static void vLOTReceiveCallback (
        void *pvConnection,
        uint16_t usSessionID,
        const uint8_t * pucData,
        size_t xDataLength,
        BaseType_t xComplete )
{
    MqttBLEService_t* pxService = ( MqttBLEService_t* ) pvConnection;

    configASSERT( xComplete == pdTRUE );

    ( void ) AwsIotMqtt_ReceiveCallback(
            pxService->xConnection.pxMqttConnection,
            pucData,
            0,
            xDataLength,
            NULL );

}



static size_t vLOTSendBlock(
       void * pvConnection,
       const void * const pvMessage ,
       size_t xLength )
{

    MqttBLEService_t* pxService = (MqttBLEService_t* ) pvConnection;
    size_t xSent = 0;

    if( prxSendNotification( pxService, eMQTTBLE_CHAR_TX_LARGE_MESG1, ( uint8_t *) pvMessage, xLength ) == pdTRUE )
    {
        xSent = xLength;
    }

    return xSent;
}

static int32_t vLOTSetNetworkReceiveCallback (
           void * pvConnection,
           void* pvContext,
           AwsIotLargeObjectTransferReceiveCallback_t  xCallback)
{
    MqttBLEService_t* pxService = (MqttBLEService_t* ) pvConnection;
    pxService->xConnection.pvLOTReceiveContext = pvContext;
    pxService->xConnection.xLOTReceiveCallback = xCallback;

    return pdTRUE;
}

static void vLOTSendCallback( uint16_t usSessionID, BaseType_t xResult )
{
    uint8_t ucId;
    MqttBLEService_t* pxService;

    for( ucId = 0; ucId < mqttBLEMAX_SVC_INSTANCES; ucId++ )
    {
        pxService = &xMqttBLEServices[ ucId ];
        if( pxService->xConnection.usLOTSendUUID == usSessionID )
        {
            xSemaphoreGive( pxService->xConnection.xLOTSendLock );
            pxService->xConnection.usLOTSendUUID = 0;
            break;
        }
    }

}


/*-----------------------------------------------------------*/

BaseType_t AwsIotMqttBLE_Init( void )
{
    BaseType_t xRet;
    uint8_t ucId;
    IotBleEventsCallbacks_t xCallback;
    MqttBLEService_t *pxService;
    AwsIotLargeObjectTransferError_t xError;

    usBLEConnMTU = mqttBLEDEFAULT_MTU_SIZE;

    for( ucId = 0; ucId < mqttBLEMAX_SVC_INSTANCES; ucId++ )
    {
        pxService = &xMqttBLEServices[ ucId ];

        /* Initialize service */
        pxService->bIsInit = false;
        pxService->bIsEnabled = false;
        pxService->pxServicePtr = &xBLEServices[ucId];
        pxService->pxServicePtr->pusHandlesBuffer = pusHandlesBuffer[ucId];
        pxService->pxServicePtr->ucInstId = ucId;
        pxService->pxServicePtr->xNumberOfAttributes = eMQTTBLE_NUMBER;
        pxService->pxServicePtr->pxBLEAttributes = (BTAttribute_t *)pxAttributeTable;

        xRet = prvInitServiceInstance( pxService->pxServicePtr );

        if( xRet == pdPASS )
        {
            pxService->bIsInit = true;
            xCallback.pConnectionCb = vConnectionCallback;

            if( IotBle_RegisterEventCb( eBLEConnection, xCallback ) != eBTStatusSuccess )
            {
                xRet = pdFAIL;
            }
        }

        if( xRet == pdPASS )
        {
            xCallback.pMtuChangedCb = vMTUChangedCallback;
            if( IotBle_RegisterEventCb( eBLEMtuChanged, xCallback ) != eBTStatusSuccess )
            {
                xRet = pdFALSE;
            }
        }

        if( xRet == pdPASS )
        {
            pxService->xConnection.xSendTimeout = pdMS_TO_TICKS( mqttBLEDEFAULT_SEND_TIMEOUT_MS );
            pxService->xConnection.xLOTSendLock = xSemaphoreCreateBinary();
            if( pxService->xConnection.xLOTSendLock != NULL )
            {
                ( void ) xSemaphoreGive( pxService->xConnection.xLOTSendLock );
            }
            else
            {
                xRet = pdFAIL;
            }
        }

        if( xRet == pdPASS )
        {
            pxService->xConnection.xLOTContext.xSendCompleteCallback = vLOTSendCallback;
            pxService->xConnection.xLOTContext.xReceiveCallback = vLOTReceiveCallback;
            pxService->xConnection.xLOTContext.xNetworkIface.pvConnection = pxService;
            pxService->xConnection.xLOTContext.xNetworkIface.send = vLOTSendBlock;
            pxService->xConnection.xLOTContext.xNetworkIface.setNetworkReceiveCallback = vLOTSetNetworkReceiveCallback;
            pxService->xConnection.xLOTContext.xParameters.usWindowSize = mqttBLE_LARGE_OBJECT_WINDOW_SIZE;
            pxService->xConnection.xLOTContext.xParameters.usWindowIntervalMS = mqttBLE_LARGE_OBJECT_WINDOW_INTERVAL_MS;
            pxService->xConnection.xLOTContext.xParameters.usWindowRetries = mqttBLE_LARGE_OBJECT_WINDOW_RETRIES;
            pxService->xConnection.xLOTContext.xParameters.usMTU = mqttBLE_LARGE_OBJECT_BLOCK_SIZE;

            xError = AwsIotLargeObjectTransfer_Init( &pxService->xConnection.xLOTContext, 1, 1 );
            if( xError != AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
            {
                configPRINTF(( "Failed to initialize large object transfer context " ));
                xRet = pdFALSE;
            }
        }


        if( xRet == pdFALSE )
        {
            if( pxService->xConnection.xLOTSendLock != NULL )
            {
                vSemaphoreDelete( pxService->xConnection.xLOTSendLock );
            }

            break;
        }
    }

    return xRet;
}

/*-----------------------------------------------------------*/

BaseType_t AwsIotMqttBLE_CreateConnection( AwsIotMqttConnection_t* pxMqttConnection, AwsIotMqttBLEConnection_t* pxConnection )
{
    BaseType_t xRet = pdFALSE;
    MqttBLEService_t* pxService;
    int lX;

    for( lX = 0; lX < mqttBLEMAX_SVC_INSTANCES; lX++ )
    {
    	pxService = &xMqttBLEServices[ lX ];
        if( ( pxService->bIsEnabled ) && ( pxService->xConnection.pxMqttConnection == NULL ) )
        {
        	pxService->xConnection.pxMqttConnection = pxMqttConnection;
        	*pxConnection =  ( AwsIotMqttBLEConnection_t ) pxService;
        	xRet = pdTRUE;
        }
    }

    return xRet;
}

/*-----------------------------------------------------------*/

void AwsIotMqttBLE_CloseConnection( AwsIotMqttBLEConnection_t xConnection )
{
    MqttBLEService_t * pxService = ( MqttBLEService_t * ) xConnection;
    if( ( pxService != NULL ) && ( pxService->xConnection.pxMqttConnection != NULL ) )
    {
    	pxService->xConnection.pxMqttConnection = NULL;
    }
}

void AwsIotMqttBLE_DestroyConnection( AwsIotMqttBLEConnection_t xConnection )
{
    MqttBLEService_t* pxService = ( MqttBLEService_t * ) xConnection;
    if( ( pxService != NULL ) && ( pxService->xConnection.pxMqttConnection == NULL ) )
    {
        prvCloseSessions( pxService );
    }
}

/*-----------------------------------------------------------*/

BaseType_t AwsIotMqttBLE_SetSendTimeout( AwsIotMqttBLEConnection_t xConnection, uint16_t usTimeoutMS )
{
    BaseType_t xRet = pdFALSE;
    MqttBLEService_t * pxService = ( MqttBLEService_t * ) xConnection;

    if( pxService != NULL )
    {
        pxService->xConnection.xSendTimeout = pdMS_TO_TICKS( usTimeoutMS );
        xRet = pdTRUE;
    }
    return xRet;
}

/*-----------------------------------------------------------*/

size_t AwsIotMqttBLE_Send( void* pvConnection, const void * const pvMessage, size_t xMessageLength )
{
    MqttBLEService_t * pxService = ( MqttBLEService_t * ) pvConnection;
    size_t xSent = 0;
    TickType_t xRemainingTime = pxService->xConnection.xSendTimeout;
    TimeOut_t xTimeout;
    AwsIotLargeObjectTransferError_t xError;

    vTaskSetTimeOutState( &xTimeout );

    if( ( pxService != NULL ) && ( pxService->bIsEnabled ) && ( pxService->xConnection.pxMqttConnection != NULL ) )
    {
        if( xMessageLength < ( size_t ) mqttBLETRANSFER_LEN( usBLEConnMTU ) )
        {
            if( prxSendNotification( pxService, eMQTTBLE_CHAR_TX_MESG, ( uint8_t *) pvMessage, xMessageLength ) == pdTRUE )
            {
                xSent = xMessageLength;
            }
            else
            {
                 configPRINTF( ( "Failed to send notify for MQTT service\r\n" ));
            }
        }
        else
        {
            if( xSemaphoreTake( pxService->xConnection.xLOTSendLock, xRemainingTime ) == pdPASS )
            {
                xError = AwsIotLargeObjectTransfer_Send(
                        &pxService->xConnection.xLOTContext,
                        ( const uint8_t * ) pvMessage,
                        xMessageLength,
                        &pxService->xConnection.usLOTSendUUID );

                if( xError == AWS_IOT_LARGE_OBJECT_TRANSFER_SUCCESS )
                {
                    ( void ) xTaskCheckForTimeOut( &xTimeout, &xRemainingTime );
                    if( xSemaphoreTake( pxService->xConnection.xLOTSendLock, xRemainingTime ) == pdPASS )
                    {
                        xSent = xMessageLength;
                        xSemaphoreGive( pxService->xConnection.xLOTSendLock );
                    }
                    else
                    {
                        configPRINTF(( "Failed to complete large object send for MQTT service.\r\n" ));
                    }
                }
                else
                {
                    configPRINTF(( "Failed to complete large object send for MQTT service, error = %d.\r\n", xError ));
                    xSemaphoreGive( pxService->xConnection.xLOTSendLock );
                }
            }
        }
    }
    else
    {
        configPRINTF( ( "Failed to send data, mqtt service state:%d \n", pxService->bIsEnabled ) );
    }

    return xSent;
}
