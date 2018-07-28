#include <board.h>
#include <rtthread.h>
#include "app_main.h"
int app_thread_entry(void)
{
    /* user app entry */
	rt_kprintf("hello");
	rt_thread_t init_thread;
	init_thread = rt_thread_create("app_led",
								   led_thread_entry, RT_NULL,
								   1024, 8, 20);
	if (init_thread != RT_NULL)
	{
		rt_thread_startup(init_thread);
		rt_kprintf("create app_led thread\n");
	}
	init_thread = rt_thread_create("app_air800",
								   air800_thread_entry, RT_NULL,
								   1024, 5, 20);
	if (init_thread != RT_NULL)
	{
		rt_thread_startup(init_thread);
		rt_kprintf("create app_air800 thread\n");
	}
#if 0
	init_thread = rt_thread_create("app_bms",
								   BMS_thread_entry, RT_NULL,
								   1024, 8, 20);
	if (init_thread != RT_NULL)
	{
		rt_thread_startup(init_thread);
		rt_kprintf("create app_bms thread\n");
	}

	init_thread = rt_thread_create("app_air800_server",
									   air800_server_thread_entry, RT_NULL,
									   1024, 1, 20);
	if (init_thread != RT_NULL)
	{
		rt_thread_startup(init_thread);
		rt_kprintf("create app_air800_server thread\n");
	}
	#endif

    return 0;
}