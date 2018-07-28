#ifndef __AIR_800_H__
#define __AIR_800_H__


typedef enum
{
	air800_Power_OK                       			= 1<<4,  /*!< */
	air800_INIT_OK                       			= 1<<5,  /*!< */
	air800_BLE_INIT_OK						= 1<<6,  /*!< */
	air800_GPS_INIT_OK						= 1<<7,  /*!< */
	air800_TCP_0_CONNECT_OK                		= 1<<8,  /*!< */
	air800_TCP_1_CONNECT_OK                 	= 1<<9,  /*!< */
	air800_AUTH_TO_SEVER_OK                 	= 1<<10,  /*!< */
	air800_LOGIN_TO_SEVER_OK                	= 1<<11,  /*!< */
	air800_UPDATEKEY_TO_SEVER_OK        	= 1<<12,  /*!< */
	air800_OTA_NeedDownFirmware           	= 1<<13,  /*!< */

	air800_BLE_Pair_OK						= 1<<14,  /*!< */

	air800_OTA_AUTH_TO_SEVER_OK			= 1<<20,  /*!< */
	air800_OTA_LOGIN_TO_SEVER_OK			= 1<<21,  /*!< */

	air800_OTA_UpToComponent				= 1<<22,  /*!< */

} EventTypeair800_t;


extern rt_event_t event_air800;

void air800_thread_entry(void *parameter);






#endif

