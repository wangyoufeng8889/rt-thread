#include <board.h>
#include <rtthread.h>
#include <drivers/pin.h>

#define pin_led_G 	8	///PC0,greed led
#define pin_led_B 	9	///PC1,blue led
#define pin_led_R 	10	///PC2,red led

int led_hw_init(void)
{

	rt_pin_mode(pin_led_R,PIN_MODE_OUTPUT);
	rt_pin_write(pin_led_R, 1);

	rt_pin_mode(pin_led_G,PIN_MODE_OUTPUT);
	rt_pin_write(pin_led_G, 1);	

	rt_pin_mode(pin_led_B,PIN_MODE_OUTPUT);
	rt_pin_write(pin_led_B, 1);
	return 0;
}
///on:1,off:0
int led_ctr(rt_uint32_t led,rt_bool_t onoff)
{
	if(onoff == 0)
		{
		rt_pin_write(led,0);
	}
	else
		{
		rt_pin_write(led,1);
	}
	return 0;
}


void led_thread_entry(void *parameter)
{
	led_hw_init();

	while (1)
	{
		led_ctr(pin_led_R,0);
		rt_thread_delay(rt_tick_from_millisecond(1000));
		led_ctr(pin_led_R,1);
		rt_thread_delay(rt_tick_from_millisecond(1000));
	}
}





