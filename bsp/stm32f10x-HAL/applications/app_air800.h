#ifndef __AIR_800_H__
#define __AIR_800_H__


typedef enum
{
	air800_Power_OK                       	= 1<<0,  /*!< */
	air800_Baud_OK                       	= 1<<1,  /*!< */
	air800_ActNet_OK						= 1<<2,  /*!< */
} EventTypeair800_t;
#define rt_EVENT_FULL 0xFFFFFFFF

extern rt_event_t event_air800;
extern rt_sem_t sem_air800_power;

void air800_thread_entry(void *parameter);






#endif

