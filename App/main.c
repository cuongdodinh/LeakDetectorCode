 /* Standard C Libraries */
#include <App/Global.h>
#include <App/RadioTask.h>
#include <App/SensorTask.h>


#include <stdio.h>
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
#include <ti/devices/cc13x0/driverlib/sys_ctrl.h>


/* Board Header files */
#include "Board.h"

/* EasyLink API Header files */
#include "easylink/EasyLink.h"

UART_Handle uart;

Clock_Struct clkStruct;
_u8 aliveCounter = 1;

//---------------------------------------------------------------------------------------------------------
void aliveGuard (UArg arg0)
{
    if (aliveCounter != radioAliveCounter)
        SysCtrlSystemReset();

    Event_post(radioEventHandle, RADIO_EVENT_SEND_ALIVE);
    aliveCounter++;
}

/*
 *  ======== main ========
 */
int main(void)
{
    /* Call driver init functions. */
    Board_initGeneral();

    Board_initUART();

    SensorTaskInit();

    RadioTaskInit ();

    /* Construct BIOS Objects */
    Clock_Params clkParams;


    Clock_Params_init(&clkParams);
    clkParams.period = SEND_ALIVE_PERIOD;
    clkParams.startFlag = TRUE;

    /* Construct a periodic Clock Instance */
    Clock_construct(&clkStruct, (Clock_FuncPtr) aliveGuard, SEND_ALIVE_PERIOD, &clkParams);


    /* Start BIOS */
    BIOS_start();

    return (0);
}
