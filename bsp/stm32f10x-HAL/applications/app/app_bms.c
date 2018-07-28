#include <board.h>
#include <rtthread.h>


static rt_err_t bms_rx_ind(rt_device_t dev, rt_size_t size)
{
   	/* release semaphore to let finsh thread rx data */
    	//rt_sem_release(&shell->rx_sem);
	rt_sem_release(sem_bms_rx);
    return RT_EOK;
}
#define BMS_CMD_WRITE 	0x5A
#define BMS_CMD_READ 	0xA5
#define BMS_CMD_1		0x03
#define BMS_CMD_2		0x04
#define BMS_CMD_3		0x05


rt_uint8_t bms_cmd(bms_cmd_t* bms_cmd,bms_rsp_t*bms_rsp)
{
	static rt_uint8_t buf[100];
	static rt_uint8_t *pbuf=RT_NULL;
	static rt_uint8_t count=0;
	static rt_uint8_t revlen=0;
	static rt_uint8_t i=0;
	static rt_uint16_t crc16=0;
	rt_memset(buf, 0x00, 100);
	pbuf = (rt_uint8_t*)bms_cmd;
	crc16 = 0;
	count = bms_cmd->datalen+2;
	for(i=1;i<count;i++)
		{
		crc16 += pbuf[i];
	}
	crc16 = ~crc16 + 1;
	count = 0;
	///cmd buf
	buf[count++] = 0xDD;//header
	rt_memcpy(buf+count,pbuf,bms_cmd->datalen+3);
	count += bms_cmd->datalen+3;

	buf[count++] = (rt_uint8_t)(crc16>>8);
	buf[count++] = (rt_uint8_t)crc16;
	buf[count++] = 0x77;//endflag
	rt_device_write(bms_uart, 0, buf, count);
	rt_memset(buf, 0x00, 100);
	rt_sem_take(sem_bms_rx, RT_WAITING_FOREVER);
	revlen = rt_device_read(bms_uart, -1, buf, 100);
	if(revlen < 5)
		{
		return RT_FALSE;
	}
	for(i=0;i<100;i++)
		{
		if(buf[i] == 0xDD)
			break;
	}
	if(i==100)
		{
		return RT_FALSE;
	}
	pbuf = buf+1+i;
	count = i;
	for(i=0;i<100;i++)
		{
		if(buf[i] == 0x77)
			break;
	}
	if(i==100)
		{
		return RT_FALSE;
	}
	count = i - count-3;
	crc16 = 0;
	for(i=1;i<count;i++)
		{
		crc16 += pbuf[i];
	}
	crc16 = ~crc16 + 1;
	if(crc16 != (pbuf[count]<<8 | pbuf[count+1]))
		{
		return RT_FALSE;
	}
	rt_memcpy((rt_uint8_t*)bms_rsp,pbuf,count);
	return RT_TRUE;
}

rt_event_t event_bms = RT_NULL;
rt_sem_t sem_bms_rx = RT_NULL;
rt_device_t bms_uart = RT_NULL;
#define BMS_UART_NAME "uart2" 
const rt_size_t bms_buf_len = 100;

void BMS_thread_entry(void *parameter)
{
	rt_uint8_t *bms_revbuf;
	bms_cmd_t bms_cmd_data;
	bms_rsp_t bms_rsp_data;
	bms_revbuf = (rt_uint8_t*)rt_malloc(sizeof(rt_uint8_t)*bms_buf_len);
	if(bms_revbuf == RT_NULL)
		{
		rt_kprintf("bms_revbuf malloc fail\n");
	}
	sem_bms_rx = (rt_sem_t)rt_malloc(sizeof(struct rt_semaphore));
	rt_sem_init(sem_bms_rx, "sem_bms_rx", 0, RT_IPC_FLAG_FIFO);
	if (bms_uart == RT_NULL)
	{
		bms_uart = rt_device_find(BMS_UART_NAME);
		if (bms_uart == RT_NULL)
		{
			rt_kprintf("finsh: can not find device: %s\n", BMS_UART_NAME);
		}
		if (rt_device_open(bms_uart, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX | \
		 				RT_DEVICE_FLAG_STREAM) == RT_EOK)
		{
			rt_device_set_rx_indicate(bms_uart, bms_rx_ind);
		}
	}
	while(1)
		{
		rt_memset((rt_uint8_t*)&bms_cmd_data,0x00,sizeof(bms_cmd_t));
		rt_memset((rt_uint8_t*)&bms_rsp_data,0x00,sizeof(bms_rsp_t));
		bms_cmd_data.WR = BMS_CMD_READ;
		bms_cmd_data.cmd = BMS_CMD_2;
		bms_cmd_data.datalen = 0;
		bms_cmd(&bms_cmd_data,&bms_rsp_data);
		rt_kprintf("cmd:%x,status:%x,datalen:%d",bms_rsp_data.cmd,bms_rsp_data.status,bms_rsp_data.datalen);

		rt_thread_delay(1000);
		
	}
}

