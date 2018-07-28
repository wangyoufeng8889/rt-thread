#include <board.h>
#include <rtthread.h>
#include <drivers/pin.h>
#include <at.h>

#include "app_main.h"

#define pin_air800_PKEY 			25	///PC5,air800_PKEY 
#define pin_air800_DTR 			26	///PB0,air800_DTR
#define pin_air800_RESET 			27	///PB1,air800_RESET
#define pin_air800_POWER_EN 	34	///PB13,air800_POWER_EN
int air800_ctr_io_hw_init(void)
{
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

	rt_pin_mode(pin_air800_PKEY,PIN_MODE_OUTPUT);
	rt_pin_write(pin_air800_PKEY, 0);

	rt_pin_mode(pin_air800_DTR,PIN_MODE_OUTPUT);
	rt_pin_write(pin_air800_DTR, 1);

	rt_pin_mode(pin_air800_RESET,PIN_MODE_OUTPUT);
	rt_pin_write(pin_air800_RESET, 0);

	rt_pin_mode(pin_air800_POWER_EN,PIN_MODE_OUTPUT);
	rt_pin_write(pin_air800_POWER_EN, 0);
	return 0;
}
///on:1,off:0
int air800_pkey_ctr(rt_bool_t onoff)
{
	rt_pin_write(pin_air800_PKEY,onoff);
	return 0;
}
///on:1,off:0
int air800_DTR_ctr(rt_bool_t onoff)
{
	rt_pin_write(pin_air800_DTR,onoff);
	return 0;
}
///on:1,off:0
int air800_RESET_ctr(rt_bool_t onoff)
{
	rt_pin_write(pin_air800_RESET,onoff);
	return 0;
}
///on:1,off:0
int air800_POWER_EN_ctr(rt_bool_t onoff)
{
	rt_pin_write(pin_air800_POWER_EN,onoff);
	return 0;
}
static rt_uint8_t air800_INIT_status=0;
rt_int8_t air800_Reset(uint8_t nub)
{
	//ģ��ϵ�
	air800_INIT_status = 0;
	rt_kprintf("L218E restart");
	rt_event_control(event_air800, RT_IPC_CMD_RESET, RT_NULL);
	return 0;
}
    


rt_int8_t air800_init(void)
{
	static rt_uint32_t recved;
	static at_response_t resp = RT_NULL;
	if(RT_EOK == rt_event_recv(event_air800, air800_Power_OK, RT_EVENT_FLAG_OR, RT_WAITING_NO, &recved))
		{
	}
	if((recved&air800_Power_OK) == RT_FALSE)
		{
		if(air800_INIT_status == 0)
			{
			air800_Reset(0);
			air800_POWER_EN_ctr(0);
			rt_thread_delay(rt_tick_from_millisecond(1000));
			air800_POWER_EN_ctr(1);
			air800_pkey_ctr(0);
			rt_thread_delay(rt_tick_from_millisecond(3000));
			air800_INIT_status++;
			//rt_kprintf("air800 power on\n");
		}
		else if(air800_INIT_status == 1)
			{
			air800_pkey_ctr(1);
			rt_thread_delay(rt_tick_from_millisecond(3000));
			air800_pkey_ctr(0);
			rt_thread_delay(rt_tick_from_millisecond(5000));
			air800_INIT_status++;
			//rt_kprintf("air800 power key on\n");
		}
		else if(air800_INIT_status == 2)
			{
			resp = at_create_resp(32, 0, rt_tick_from_millisecond(3000));
			if (resp == RT_NULL)
			{
				LOG_E("No memory for response structure!");
				return -RT_ENOMEM;
			}
			/* 发送 AT 命令并接收 AT Server 响应数据，数据及信息存放在 resp 结构
			体中 */
			if (at_exec_cmd(resp, "ATE0") != RT_EOK)
			{
				//LOG_E("AT client send commands failed, response error or timeout !");
			}
			else
				{
				air800_INIT_status++;
				LOG_D(resp->buf);
			}
			at_delete_resp(resp);
			rt_thread_delay(rt_tick_from_millisecond(1000));
		}
		else if(air800_INIT_status == 3)
			{
			resp = at_create_resp(32, 0, rt_tick_from_millisecond(10000));
			if (!resp)
			{
				LOG_E("No memory for response structure!");
				return -RT_ENOMEM;
			}
			/* 发送 AT 命令并接收 AT Server 响应数据，数据及信息存放在 resp 结构
			体中 */
			if (at_exec_cmd(resp, "AT+CGREG?") != RT_EOK)
			{
				//LOG_E("AT client send commands failed, response error or timeout !");
			}
			else
				{
				LOG_D(resp->buf);
			}
			at_delete_resp(resp);
			rt_thread_delay(rt_tick_from_millisecond(1000));
		}
	}
	else if((recved&air800_INIT_OK) == RT_FALSE)
		{

	}
}

rt_event_t event_air800 = RT_NULL;
void air800_thread_entry(void *parameter)
{
	air800_ctr_io_hw_init();
	event_air800 = (rt_event_t)rt_malloc(sizeof(struct rt_event));
	rt_event_init(event_air800, "event_air800", RT_IPC_FLAG_FIFO);
	while (1)
	{	
		air800_init();
		rt_thread_delay(rt_tick_from_millisecond(1000));
	}
}














