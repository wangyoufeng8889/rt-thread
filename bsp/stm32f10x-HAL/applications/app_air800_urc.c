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

static struct at_urc urc_table[] = {
	{"RDY", "\r\n", urc_RDY},
	{"+CFUN:", "\r\n", urc_CFUN}
};






int at_client_port_init(void)
{
	at_set_urc_table(urc_table, sizeof(urc_table)/sizeof(urc_table[0]));
	return 0;
}


