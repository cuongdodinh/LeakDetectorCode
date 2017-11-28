//16.05.17 Avkhadiev Rustem


#include <App/Global.h>
#include <App/RadioTask.h>
#include "App/Protocol.h"

#include <stdlib.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Error.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>

/* TI-RTOS Header files */
#include <ti/drivers/PIN.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/Power.h>

/* Board Header files */
#include "Board.h"

/* EasyLink API Header files */
#include "easylink/EasyLink.h"
#include <sce/scif.h>

#include <ti/devices/cc13x0/driverlib/aon_batmon.h>
#include <ti/devices/cc13x0/driverlib/sys_ctrl.h>

#define RFEASYLINKTX_TASK_STACK_SIZE    1024
#define RFEASYLINKTX_TASK_PRIORITY      2

#define ERROR_PACKET_MAX_LEN         EASYLINK_MAX_DATA_LENGTH - PACKET_OFFSET_PAYLOAD

#define ACK_TIMEOUT_MS 3
#define MAC_LEN  8
#define PASSWORD_LEN  8

Task_Struct radioTask;    /* not static so you can see in ROV */
static Task_Params radioTaskParams;
static uint8_t radioTaskStack[RFEASYLINKTX_TASK_STACK_SIZE];

static _u8 seqNumber = 0;

_u8 maxRetries = 200;
_u8 sourceAddress = 0xFF;
_u8 mac[8];
_u8 password [8] = {'1','1','1','2','2','3','3','3'};
_i8  lastRSSI = 0;

_u8 radioAliveCounter = 0;

EasyLink_TxPacket txPacket;
EasyLink_RxPacket rxPacket;

_u8 errorLogBuffer [ERROR_PACKET_MAX_LEN];
_u8 errorLogBufferLen = 0;
_u8 errorPacketsSentSinceLastAlivePacket = 0;


Event_Struct radioEvent;
Event_Handle radioEventHandle;

void RadioTaskSendAck ();

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
_u8 GetRandomNum ()
{
 //   _u32 t1, t2;

//    t1 = Clock_getTicks();

    _u8 result;
    _u32 t = Clock_getTicks();

    _u8* tPtr = (_u8*)&t;
    result = *tPtr;
    result += *(tPtr + 1);
    result += *(tPtr + 2);
    result += *(tPtr + 3);

//    t2 = Clock_getTicks();

//    Log ("GetRandomNum time: %u", t2 * Clock_tickPeriod - t1 * Clock_tickPeriod);

    return result;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void AddToErrorLog (_u8 errorCode, _u8 errorValue)
{
    if (errorLogBufferLen + 2 > ERROR_PACKET_MAX_LEN)
        return;

    errorLogBuffer[errorLogBufferLen] = errorCode;
    errorLogBuffer[errorLogBufferLen + 1] = errorValue;

    errorLogBufferLen += 2;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ProcessBridgePacket()
{
    switch (rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE])
    {
    case PACKET_TYPE_SETTINGS:
        if (rxPacket.len != PACKET_TYPE_SETTINGS_LEN)
        {
            Log ("PACKET_TYPE_SETTINGS Error rxPacket.len: %d", rxPacket.len);
            AddToErrorLog (ERROR_ProcessBridgePacket1, rxPacket.len);
            return;
        }
        if (memcmp (&rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1], password, PASSWORD_LEN) != 0)
        {
            Log ("PACKET_TYPE_SETTINGS Error password: %u", rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1]);
            AddToErrorLog (ERROR_ProcessBridgePacket4, rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1]);
            return;
        }
        maxRetries = rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1 + PASSWORD_LEN];
        Log ("maxRetries: %u", maxRetries);
        break;

    case PACKET_TYPE_REGISTER_NODE:
        if (rxPacket.len != PACKET_TYPE_REGISTER_NODE_LEN)
        {
            Log ("PACKET_TYPE_REGISTER_NODE Error rxPacket.len: %d", rxPacket.len);
            AddToErrorLog (ERROR_ProcessBridgePacket2, rxPacket.len);
            return;
        }

        if (memcmp (&rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1], password, PASSWORD_LEN) != 0)
        {
            Log ("PACKET_TYPE_REGISTER_NODE Error password: %u", rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1]);
            AddToErrorLog (ERROR_ProcessBridgePacket3, rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1]);
            return;
        }

        sourceAddress = rxPacket.payload[BRIDGE_PACKET_OFFSET_PACKET_TYPE + 1 + PASSWORD_LEN];
//      Log ("sourceAddress: %u", sourceAddress);
        break;
    }

    RadioTaskSendAck ();
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RadioTaskSendPacket (bool getAck)
{
    seqNumber++;

    txPacket.absTime = 0;
    txPacket.payload[PACKET_OFFSET_SRC_ADDR]    = sourceAddress;
    txPacket.payload[PACKET_OFFSET_SEQ_NUMBER]  = seqNumber;

    txPacket.dstAddr[0] = BRIDGE_ADDRESS;

    int s = 0;

    //_u32 t1;//, t2, t3, t4;

    for (s = 0; s < maxRetries; s++)
    {
        //t1 = Clock_getTicks();
        EasyLink_transmit(&txPacket);

        if (getAck == false)
            return false;
//        t2 = Clock_getTicks();

        rxPacket.absTime = 0;
        rxPacket.rxTimeout = EasyLink_ms_To_RadioTime(ACK_TIMEOUT_MS);


//        t3 = Clock_getTicks();
        EasyLink_Status result = EasyLink_receive(&rxPacket);
//        t4 = Clock_getTicks();

//        Log ("%d  %u  TX: %u   RX: %u", s, t1* Clock_tickPeriod / 1000000, t2 * Clock_tickPeriod - t1 * Clock_tickPeriod, t4 * Clock_tickPeriod - t3 * Clock_tickPeriod);
//          Log ("%d  %u  TX", s, t1* Clock_tickPeriod / 1000);

        if (result == EasyLink_Status_Success)
        {
            bool hasError = false;
            if (rxPacket.len < MIN_PACKET_LEN)
            {
                Log ("Error rxPacket.len: %d", rxPacket.len);
                AddToErrorLog (ERROR_RadioTaskSendPacket1, rxPacket.len);
                hasError = true;
            }
            if (rxPacket.payload[PACKET_OFFSET_PACKET_TYPE] != PACKET_TYPE_ACK)
            {
                Log ("Error packet type: %d", rxPacket.payload[PACKET_OFFSET_PACKET_TYPE]);
                AddToErrorLog (ERROR_RadioTaskSendPacket2, rxPacket.len);
                hasError = true;
            }
            if (rxPacket.payload[PACKET_OFFSET_SRC_ADDR] != BRIDGE_ADDRESS)
            {
                Log ("Error sourceAddress: %d", rxPacket.payload[PACKET_OFFSET_SRC_ADDR]);
                AddToErrorLog (ERROR_RadioTaskSendPacket3, rxPacket.len);
                hasError = true;
            }
            if (rxPacket.payload[PACKET_OFFSET_SEQ_NUMBER] != seqNumber)
            {
                Log ("Error seqNumber: %d", rxPacket.payload[PACKET_OFFSET_SEQ_NUMBER]);
                AddToErrorLog (ERROR_RadioTaskSendPacket4, rxPacket.len);
                hasError = true;
            }
            if (rxPacket.dstAddr[0] != sourceAddress)
            {
                Log ("Error dstAddr: %d", rxPacket.dstAddr[0]);
                AddToErrorLog (ERROR_RadioTaskSendPacket5, rxPacket.len);
                hasError = true;
            }

            if (hasError == false)
            {
                lastRSSI = rxPacket.rssi;

                break;
            }
        }
        else
        {
            Log ("Error Ack: %d", s);
            AddToErrorLog (ERROR_RadioTaskSendPacket6, s);
        }


        _u32 sleepTicks = (1000000 / 256 / Clock_tickPeriod) * GetRandomNum();
        Task_sleep (sleepTicks);
        //Log ("sleepTicks: %u", sleepTicks);

    }

    if (s == maxRetries)
        return false;

    if (rxPacket.len > MIN_PACKET_LEN)
        ProcessBridgePacket();

    return true;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RadioTaskSendAck ()
{
    txPacket.absTime = 0;
    txPacket.payload[PACKET_OFFSET_PACKET_TYPE] = PACKET_TYPE_ACK;
    txPacket.payload[PACKET_OFFSET_SRC_ADDR]    = sourceAddress;
    txPacket.payload[PACKET_OFFSET_SEQ_NUMBER]  = seqNumber;

    txPacket.dstAddr[0] = BRIDGE_ADDRESS;
    txPacket.len = MIN_PACKET_LEN;

    EasyLink_transmit(&txPacket);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RadioTaskSendAlarm ()
{
    txPacket.payload[PACKET_OFFSET_PACKET_TYPE] = PACKET_TYPE_ALARM;

    memcpy (&txPacket.payload[PACKET_OFFSET_PAYLOAD], mac, MAC_LEN);

    _u8 adcValueToSend = 0;

    if (scifTaskData.leakAdc.state.alert == 1)
    {
        if (scifTaskData.leakAdc.output.adcValue > 255)
            adcValueToSend = 255;
        else
            adcValueToSend = scifTaskData.leakAdc.output.adcValue;
    }

    txPacket.payload[PACKET_OFFSET_PAYLOAD + MAC_LEN]     = adcValueToSend;

    txPacket.len = PACKET_OFFSET_PAYLOAD + MAC_LEN + 1;

    int s;
    for (s = 0; s < MAX_SEND_ALARM_ATTEMPTS; s++)
    {
        if (RadioTaskSendPacket (true) == true)
            break;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RadioTaskSendAlive (_u8 aliveType)
{
    txPacket.payload[PACKET_OFFSET_PACKET_TYPE] = aliveType;

    memcpy (&txPacket.payload[PACKET_OFFSET_PAYLOAD], mac, MAC_LEN);

    txPacket.payload[PACKET_OFFSET_PAYLOAD + MAC_LEN]     = scifTaskData.leakAdc.output.adcValue;

    txPacket.payload[PACKET_OFFSET_PAYLOAD + MAC_LEN + 1] = (_u8)((AONBatMonBatteryVoltageGet()) >> 3);
    txPacket.payload[PACKET_OFFSET_PAYLOAD + MAC_LEN + 2] = lastRSSI;

    txPacket.len = PACKET_OFFSET_PAYLOAD + MAC_LEN + 3;

    RadioTaskSendPacket (true);

    errorPacketsSentSinceLastAlivePacket = 0;

    radioAliveCounter++;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RadioTaskSendErrorLog ()
{
    if (errorPacketsSentSinceLastAlivePacket >= MAX_ERROR_PACKETS_PER_SEND_ALIVE_PERIOD)
        return;

    Log ("RadioTaskSendErrorLog: %d\n\r", errorLogBufferLen);

    txPacket.payload[PACKET_OFFSET_PACKET_TYPE] = PACKET_TYPE_ERROR;

    memcpy (&txPacket.payload[PACKET_OFFSET_PAYLOAD], errorLogBuffer, errorLogBufferLen);

    txPacket.len = PACKET_OFFSET_PAYLOAD + errorLogBufferLen;

    errorLogBufferLen = 0;

    RadioTaskSendPacket (true);

    errorPacketsSentSinceLastAlivePacket++;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void radioTaskFxn (UArg arg0, UArg arg1)
{
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.readEcho = UART_ECHO_OFF;
    uart = UART_open(Board_UART0, &uartParams);

    const char title[] = "Leak detector started:\r\n";
    UART_write(uart, title, sizeof(title));

    Log ("Start radio: %u\n\r", Clock_getTicks() * Clock_tickPeriod);

    EasyLink_getIeeeAddr(mac); //8 bytes


  //  PIN_setOutputValue(pinHandle, Board_PIN_LED1, 1);

    EasyLink_init(EasyLink_Phy_50kbps2gfsk);
//    EasyLink_init(EasyLink_Phy_625bpsLrm);

    EasyLink_setFrequency(868000000);

    EasyLink_setRfPwr(12); //12 dBm

    RadioTaskSendAlive (PACKET_TYPE_REBOOT_REPORT);

    while (1)
    {
        uint32_t events = Event_pend(radioEventHandle, 0, RADIO_EVENT_ALL, BIOS_WAIT_FOREVER);

        if (events & RADIO_EVENT_ALARM)
        {
            Log ("adcValue: %u      time: %u", scifTaskData.leakAdc.output.adcValue, Clock_getTicks());
            //Log ("randomNum: %u", GetRandomNum());

            RadioTaskSendAlarm ();
        }

        if (events & RADIO_EVENT_SEND_ALIVE)
        {
            Log ("Send Alive %d", seqNumber);
            RadioTaskSendAlive (PACKET_TYPE_ALIVE);
        }

        if (errorLogBufferLen != 0)
            RadioTaskSendErrorLog ();

    }

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RadioTaskInit ()
{

    /* Create event used internally for state changes */
    Event_Params eventParam;
    Event_Params_init(&eventParam);
    Event_construct(&radioEvent, &eventParam);
    radioEventHandle = Event_handle(&radioEvent);

    Task_Params_init(&radioTaskParams);
    radioTaskParams.stackSize = RFEASYLINKTX_TASK_STACK_SIZE;
    radioTaskParams.priority = RFEASYLINKTX_TASK_PRIORITY;
    radioTaskParams.stack = &radioTaskStack;
    radioTaskParams.arg0 = (UInt)1000000;

    Task_construct(&radioTask, radioTaskFxn, &radioTaskParams, NULL);
}



