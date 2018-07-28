#include <board.h>
#include <rtthread.h>
#include <at.h>
#include "app_main.h"


static void urc_RDY(const char *data, rt_size_t size)
{
	LOG_I("air800 start");
}
static void urc_CFUN(const char *data, rt_size_t size)
{
	LOG_I("air800 cfun");
}
static void urc_POWER(const char *data, rt_size_t size)
{
	if(rt_strstr(data,"DOWN"))
		{
		LOG_I("air800 power down");
		rt_event_control(event_air800, RT_IPC_CMD_RESET, RT_NULL);
	}
	else
		{
		LOG_I("air800 power up");
	}
}
static void urc_CPIN(const char *data, rt_size_t size)
{
	if(rt_strstr(data,"NOT READY"))
		{
		LOG_I("air800 cpin not ready");
	}
	else
		{
		LOG_I("air800 cpin ready");
	}
}
static void urc_CallReady(const char *data, rt_size_t size)
{

	LOG_I("air800 Call Ready");
	rt_sem_release(sem_air800_power);
	
}

static struct at_urc urc_table[] = {
	{"RDY", "\r\n", urc_RDY},
	{"+CFUN:", "\r\n", urc_CFUN},
	{"NORMAL POWER", "\r\n",urc_POWER},
	{"+CPIN:", "\r\n",urc_CPIN},
	{"Call Ready", "\r\n",urc_CallReady}
};






int at_client_port_init(void)
{
	at_set_urc_table(urc_table, sizeof(urc_table)/sizeof(urc_table[0]));
	return 0;
}


