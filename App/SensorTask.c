//07.05.17 Avkhadiev Rustem

#include <App/RadioTask.h>
#include <App/SensorTask.h>
#include <App/Global.h>

#include <sce/scif.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>

#include <math.h>


#define BV(x)    (1 << (x))


/***** Prototypes *****/
static void newDataFromSensorCallback(void);

static void scCtrlReadyCallback(void)
{
} // scCtrlReadyCallback


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SensorTaskInit(void)
{
    // Initialize the Sensor Controller
    scifOsalInit();
    scifOsalRegisterCtrlReadyCallback(scCtrlReadyCallback);
    scifOsalRegisterTaskAlertCallback(newDataFromSensorCallback);
    scifInit(&scifDriverSetup);

    scifTaskData.leakAdc.cfg.instantThreshold = 80;
    scifTaskData.leakAdc.cfg.continuousThreshold = 20;
    scifTaskData.leakAdc.cfg.continuousAlarmPeriod = 5;

      scifStartRtcTicksNow(0x00010000);

    // Start Sensor Controller task
    scifStartTasksNbl(BV(SCIF_LEAK_ADC_TASK_ID));
}


//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void newDataFromSensorCallback(void)
{
    /* Clear the ALERT interrupt source */
    scifClearAlertIntSource();

    /* Acknowledge the alert event */
    scifAckAlertEvents();

    Event_post(radioEventHandle, RADIO_EVENT_ALARM);
}
