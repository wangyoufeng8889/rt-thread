#ifndef __APP_BMS_H__
#define __APP_BMS_H__

typedef struct {
	rt_uint8_t WR;
	rt_uint8_t cmd;
	rt_uint8_t datalen;
	rt_uint8_t databuf[10];
}bms_cmd_t;
typedef struct {
	rt_uint8_t cmd;
	rt_uint8_t status;
	rt_uint8_t datalen;
	rt_uint8_t databuf[100];
}bms_rsp_t;


extern rt_sem_t sem_bms_rx;
extern rt_device_t bms_uart;

void BMS_thread_entry(void *parameter);

#endif

