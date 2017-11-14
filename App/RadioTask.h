#include <ti/sysbios/knl/Event.h>
#include <App/Global.h>

#define RADIO_EVENT_ALL                  0xFFFFFFFF
#define RADIO_EVENT_ALARM         (_u32)(1 << 0)
#define RADIO_EVENT_SEND_ALIVE    (_u32)(1 << 1)

extern Event_Handle radioEventHandle;
extern _u8 radioAliveCounter;

void RadioTaskInit ();


