/*
 * File      : drv_usart.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006-2013, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2010-03-29     Bernard      remove interrupt Tx and DMA Rx mode
 * 2013-05-13     aozima       update for kehong-lingtai.
 * 2015-01-31     armink       make sure the serial transmit complete in putc()
 * 2016-05-13     armink       add DMA Rx mode
 * 2017-01-19     aubr.cool    add interrupt Tx mode
 * 2017-04-13     aubr.cool    correct Rx parity err
 * 2017-10-20     ZYH          porting to HAL Libraries(with out DMA)
 * 2017-11-15     ZYH          update to 3.0.0
 */
#include "board.h"
#include <rtdevice.h>

static void DMA_Configuration(struct rt_serial_device *serial);


/* STM32 uart driver */
struct stm32_uart
{
	UART_HandleTypeDef huart;
	IRQn_Type irq;
	struct stm32_uart_dma
    {
        /* dma channel */
        DMA_Channel_TypeDef *rx_ch;
        /* dma global flag */
        uint32_t rx_gl_flag;
        /* dma irq channel */
        uint8_t rx_irq_ch;
        /* setting receive len */
        rt_size_t setting_recv_len;
        /* last receive index */
        rt_size_t last_recv_index;
    } dma;
};

static rt_err_t stm32_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct stm32_uart *uart;
    
      RT_ASSERT(serial != RT_NULL);
      RT_ASSERT(cfg != RT_NULL);
    
      uart = (struct stm32_uart *)serial->parent.user_data;
    
      uart->huart.Init.BaudRate   = cfg->baud_rate;
      uart->huart.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
      uart->huart.Init.Mode       = UART_MODE_TX_RX;
      uart->huart.Init.OverSampling = UART_OVERSAMPLING_16;
    
      switch (cfg->data_bits)
      {
      case DATA_BITS_8:
          uart->huart.Init.WordLength = UART_WORDLENGTH_8B;
          break;
      case DATA_BITS_9:
          uart->huart.Init.WordLength = UART_WORDLENGTH_9B;
          break;
      default:
          uart->huart.Init.WordLength = UART_WORDLENGTH_8B;
          break;
      }
      switch (cfg->stop_bits)
      {
      case STOP_BITS_1:
          uart->huart.Init.StopBits   = UART_STOPBITS_1;
          break;
      case STOP_BITS_2:
          uart->huart.Init.StopBits   = UART_STOPBITS_2;
          break;
      default:
          uart->huart.Init.StopBits   = UART_STOPBITS_1;
          break;
      }
      switch (cfg->parity)
      {
      case PARITY_NONE:
          uart->huart.Init.Parity     = UART_PARITY_NONE;
          break;
      case PARITY_ODD:
          uart->huart.Init.Parity     = UART_PARITY_ODD;
          break;
      case PARITY_EVEN:
          uart->huart.Init.Parity     = UART_PARITY_EVEN;
          break;
      default:
          uart->huart.Init.Parity     = UART_PARITY_NONE;
          break;
      }
      if (HAL_UART_Init(&uart->huart) != HAL_OK)
      {
          return RT_ERROR;
      }
    
      return RT_EOK;
}

static rt_err_t stm32_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct stm32_uart* uart;
    rt_uint32_t ctrl_arg = (rt_uint32_t)(arg);

    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;

    switch (cmd)
    {
        /* disable interrupt */
    case RT_DEVICE_CTRL_CLR_INT:
        /* disable rx irq */
        NVIC_DisableIRQ(uart->irq);
        /* disable interrupt */
        __HAL_UART_DISABLE_IT(&uart->huart, USART_IT_RXNE);
        break;
        /* enable interrupt */
    case RT_DEVICE_CTRL_SET_INT:
        /* enable rx irq */
        NVIC_EnableIRQ(uart->irq);
        /* enable interrupt */
        __HAL_UART_ENABLE_IT(&uart->huart, USART_IT_RXNE);
        break;
    case RT_DEVICE_CTRL_CONFIG:
        /* enable dma rx irq */
		if(ctrl_arg == RT_DEVICE_FLAG_DMA_RX)
			{
			__HAL_UART_ENABLE_IT(&uart->huart, USART_IT_IDLE);
			DMA_Configuration(serial);
		}
        break;
    }
    return RT_EOK;
}

static int stm32_uart_putc(struct rt_serial_device *serial, char c)
{
    struct stm32_uart* uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;
    while(__HAL_UART_GET_FLAG(&uart->huart,UART_FLAG_TXE) == RESET);
    uart->huart.Instance->DR = c;
    return 1;
}

static int stm32_uart_getc(struct rt_serial_device *serial)
{
    int ch;
    struct stm32_uart* uart;
    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)serial->parent.user_data;
    ch = -1;
    if (__HAL_UART_GET_FLAG(&uart->huart,UART_FLAG_RXNE) != RESET)
    {
        ch = uart->huart.Instance->DR & 0xff;
    }
    return ch;
}
static rt_size_t stm32_uart_dma(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, int direction)
{
	if(direction ==RT_SERIAL_DMA_RX)
		{

	}
	else if(direction ==RT_SERIAL_DMA_TX)
		{

	}
}

/**
 * Uart common interrupt process. This need add to uart ISR.
 *
 * @param serial serial device
 */
static void uart_isr(struct rt_serial_device *serial) {
    struct stm32_uart *uart = (struct stm32_uart *) serial->parent.user_data;
	static rt_uint32_t length,curCNDTR;
    RT_ASSERT(uart != RT_NULL);

	if((__HAL_UART_GET_FLAG(&uart->huart, UART_FLAG_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->huart,UART_IT_RXNE) != RESET))
	{
		rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
		__HAL_UART_CLEAR_FLAG(&uart->huart,UART_FLAG_RXNE);
	}
	else if((__HAL_UART_GET_FLAG(&uart->huart, UART_FLAG_IDLE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&uart->huart,UART_IT_IDLE) != RESET))
		{
		curCNDTR = uart->dma.setting_recv_len - __HAL_DMA_GET_COUNTER(uart->huart.hdmarx);
		if(uart->dma.last_recv_index <= curCNDTR)
			{
			length = curCNDTR - uart->dma.last_recv_index;//获取dma接收数据数量
		}
		else
			{
			length = curCNDTR + uart->dma.setting_recv_len - uart->dma.last_recv_index;//获取dma接收数据数量
		}
		uart->dma.last_recv_index = curCNDTR;
		if(length > 0)
			{
			rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_DMADONE|length<<8);
		}
	}
	else
		{
		//__HAL_UART_CLEAR_PEFLAG(&uart->huart);
	}
	__HAL_UART_CLEAR_PEFLAG(&uart->huart);
}

static const struct rt_uart_ops stm32_uart_ops =
{
    stm32_uart_configure,
    stm32_uart_control,
    stm32_uart_putc,
    stm32_uart_getc,
    stm32_uart_dma,
};

#if defined(RT_USING_UART1)
/* UART1 device driver structure */
struct stm32_uart uart1 =
{
    {USART1},
    USART1_IRQn,
    	{
		DMA1_Channel5,
        DMA_FLAG_GL5,
        DMA1_Channel5_IRQn,
        0,
        0
	}
};
struct rt_serial_device serial1;

void USART1_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial1);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* RT_USING_UART1 */

#if defined(RT_USING_UART2)
/* UART1 device driver structure */
struct stm32_uart uart2 =
{
    {USART2},
    USART2_IRQn,
    	{
		DMA1_Channel6,
        DMA_FLAG_GL6,
        DMA1_Channel6_IRQn,
        0,
        0
	}
};
struct rt_serial_device serial2;

void USART2_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial2);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* RT_USING_UART2 */

#if defined(RT_USING_UART3)
/* UART1 device driver structure */
struct stm32_uart uart3 =
{
    {USART3},
    USART3_IRQn,
    	{
		DMA1_Channel3,
        DMA_FLAG_GL3,
        DMA1_Channel3_IRQn,
        0,
        0
	}
};
struct rt_serial_device serial3;

void USART3_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial3);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* RT_USING_UART2 */
#if defined(RT_USING_UART4)
/* UART1 device driver structure */
struct stm32_uart uart4 =
{
    {UART4},
    UART4_IRQn,
    	{
		DMA1_Channel1,
        DMA_FLAG_GL1,
        DMA1_Channel1_IRQn,
        0,
        0
	}
};
struct rt_serial_device serial4;

void UART4_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial4);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* RT_USING_UART1 */

#if defined(RT_USING_UART5)
/* UART1 device driver structure */
struct stm32_uart uart5 =
{
    {UART5},
    UART5_IRQn,
    	{
		0,
        0,
        0,
        0,
        0
	}
};
struct rt_serial_device serial5;

void UART5_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial5);

    /* leave interrupt */
    rt_interrupt_leave();
}
#endif /* RT_USING_UART2 */

int rt_hw_usart_init(void)
{
    struct stm32_uart* uart;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
#if defined(RT_USING_UART1)
    uart = &uart1;
    config.baud_rate = BAUD_RATE_115200;
	config.bufsz = 1024;//DMA数据缓冲
	
    serial1.ops    = &stm32_uart_ops;
    serial1.config = config;
	
    //MX_USART_UART_Init(&uart->huart);
    /* register UART1 device */
    rt_hw_serial_register(&serial1, "uart1",
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_RX ,
                          uart);
#endif /* RT_USING_UART1 */

#if defined(RT_USING_UART2)
	uart = &uart2;
	config.baud_rate = BAUD_RATE_9600;
	config.bufsz = 128;//DMA数据缓冲
	serial2.ops    = &stm32_uart_ops;
	serial2.config = config;

	//MX_USART_UART_Init(&uart->huart);
	/* register UART1 device */
	rt_hw_serial_register(&serial2, "uart2",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_RX ,
	                      uart);
#endif /* RT_USING_UART1 */

#if defined(RT_USING_UART3)
	uart = &uart3;
	config.baud_rate = BAUD_RATE_115200;		
	config.bufsz = 1024;//DMA数据缓冲
	
	serial3.ops    = &stm32_uart_ops;
	serial3.config = config;
	
	//MX_USART_UART_Init(&uart->huart);
	/* register UART1 device */
	rt_hw_serial_register(&serial3, "uart3",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_RX ,
	                      uart);
#endif /* RT_USING_UART1 */
#if defined(RT_USING_UART4)
	uart = &uart4;
	config.baud_rate = BAUD_RATE_115200;		
	config.bufsz = 1024;//DMA数据缓冲
	
	serial4.ops    = &stm32_uart_ops;
	serial4.config = config;
	
	//MX_USART_UART_Init(&uart->huart);
	/* register UART1 device */
	rt_hw_serial_register(&serial4, "uart4",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_DMA_RX ,
	                      uart);
#endif /* RT_USING_UART1 */

#if defined(RT_USING_UART5)
	uart = &uart5;
	config.baud_rate = BAUD_RATE_115200;
	
	serial5.ops    = &stm32_uart_ops;
	serial5.config = config;

	//MX_USART_UART_Init(&uart->huart);
	/* register UART1 device */
	rt_hw_serial_register(&serial5, "uart5",
	                      RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX ,
	                      uart);
#endif /* RT_USING_UART1 */

}

INIT_BOARD_EXPORT(rt_hw_usart_init);

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  	__HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */
    
  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
  	__HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration    
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  	__HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
  else if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspInit 0 */

  /* USER CODE END UART4_MspInit 0 */
    /* UART3 clock enable */
    __HAL_RCC_UART4_CLK_ENABLE();
  	__HAL_RCC_GPIOC_CLK_ENABLE();
    /**UART3 GPIO Configuration    
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* UART4 interrupt Init */
    HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspInit 1 */

  /* USER CODE END UART4_MspInit 1 */
  }
  else if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspInit 0 */

  /* USER CODE END UART5_MspInit 0 */
    /* UART5 clock enable */
    __HAL_RCC_UART5_CLK_ENABLE();
  	__HAL_RCC_GPIOC_CLK_ENABLE();
  	__HAL_RCC_GPIOD_CLK_ENABLE();
    /**UART5 GPIO Configuration    
    PC12     ------> UART5_TX
    PD2      ------> UART5_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART5 interrupt Init */
    HAL_NVIC_SetPriority(UART5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART5_IRQn);
  /* USER CODE BEGIN USART5_MspInit 1 */

  /* USER CODE END USART5_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();
  
    /**USART2 GPIO Configuration    
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
  else if(uartHandle->Instance==UART4)
  {
  /* USER CODE BEGIN UART4_MspDeInit 0 */

  /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**UART4 GPIO Configuration    
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART4_IRQn);
  /* USER CODE BEGIN UART4_MspDeInit 1 */

  /* USER CODE END UART4_MspDeInit 1 */
  }
  else if(uartHandle->Instance==UART5)
  {
  /* USER CODE BEGIN UART5_MspDeInit 0 */

  /* USER CODE END UART5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART5_CLK_DISABLE();
  
    /**UART5 GPIO Configuration    
    PC12     ------> UART5_TX
    PD2      ------> UART5_RX 
    */
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_12);
	HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    /* UART5 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART5_IRQn);
  /* USER CODE BEGIN UART5_MspDeInit 1 */

  /* USER CODE END UART5_MspDeInit 1 */
  }
} 

static void DMA_Configuration(struct rt_serial_device *serial) {

	DMA_HandleTypeDef *hdma_usart_rx;
	struct stm32_uart* uart;
	struct rt_serial_rx_fifo* rx_fifo;
	hdma_usart_rx = (DMA_HandleTypeDef *)rt_malloc(sizeof(DMA_HandleTypeDef));
    uart = (struct stm32_uart *) serial->parent.user_data;
	rx_fifo = serial->serial_rx;
	uart->dma.setting_recv_len = serial->config.bufsz;
	uart->dma.last_recv_index = 0;
	if(uart->huart.Instance == USART1)
  	{
		__HAL_RCC_DMA1_CLK_ENABLE();
	 	//HAL_NVIC_SetPriority(uart->dma.rx_irq_ch, 5, 0);
		//HAL_NVIC_EnableIRQ(uart->dma.rx_irq_ch);
		/* USART1 DMA Init */
	    /* USART1_RX Init */
		hdma_usart_rx->Instance = uart->dma.rx_ch;
		hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
		hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_usart_rx->Init.Mode = DMA_CIRCULAR;
		hdma_usart_rx->Init.Priority = DMA_PRIORITY_VERY_HIGH;		
		if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK)
		{
		}
		__HAL_LINKDMA(&uart->huart,hdmarx,*hdma_usart_rx);
		HAL_UART_Receive_DMA(&uart->huart,rx_fifo->buffer,serial->config.bufsz);
	}
	else if(uart->huart.Instance == USART2)
  	{
		__HAL_RCC_DMA1_CLK_ENABLE();
	 	//HAL_NVIC_SetPriority(uart->dma.rx_irq_ch, 5, 0);
		//HAL_NVIC_EnableIRQ(uart->dma.rx_irq_ch);
		/* USART1 DMA Init */
	    /* USART1_RX Init */
		hdma_usart_rx->Instance = uart->dma.rx_ch;
		hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
		hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_usart_rx->Init.Mode = DMA_CIRCULAR;
		hdma_usart_rx->Init.Priority = DMA_PRIORITY_VERY_HIGH;		
		if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK)
		{
		}
		__HAL_LINKDMA(&uart->huart,hdmarx,*hdma_usart_rx);
		HAL_UART_Receive_DMA(&uart->huart,rx_fifo->buffer,serial->config.bufsz);
	}
	else if(uart->huart.Instance == USART3)
  	{
		__HAL_RCC_DMA1_CLK_ENABLE();
	 	//HAL_NVIC_SetPriority(uart->dma.rx_irq_ch, 5, 0);
		//HAL_NVIC_EnableIRQ(uart->dma.rx_irq_ch);
		/* USART1 DMA Init */
	    /* USART1_RX Init */
		hdma_usart_rx->Instance = uart->dma.rx_ch;
		hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
		hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_usart_rx->Init.Mode = DMA_CIRCULAR;
		hdma_usart_rx->Init.Priority = DMA_PRIORITY_VERY_HIGH;		
		if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK)
		{
		}
		__HAL_LINKDMA(&uart->huart,hdmarx,*hdma_usart_rx);
		HAL_UART_Receive_DMA(&uart->huart,rx_fifo->buffer,serial->config.bufsz);
	}
	else if(uart->huart.Instance == UART4)
  	{
		__HAL_RCC_DMA1_CLK_ENABLE();
	 	//HAL_NVIC_SetPriority(uart->dma.rx_irq_ch, 5, 0);
		//HAL_NVIC_EnableIRQ(uart->dma.rx_irq_ch);
		/* USART1 DMA Init */
	    /* USART1_RX Init */
		hdma_usart_rx->Instance = uart->dma.rx_ch;
		hdma_usart_rx->Init.Direction = DMA_PERIPH_TO_MEMORY;
		hdma_usart_rx->Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_usart_rx->Init.MemInc = DMA_MINC_ENABLE;
		hdma_usart_rx->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_usart_rx->Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_usart_rx->Init.Mode = DMA_CIRCULAR;
		hdma_usart_rx->Init.Priority = DMA_PRIORITY_VERY_HIGH;		
		if (HAL_DMA_Init(hdma_usart_rx) != HAL_OK)
		{
		}
		__HAL_LINKDMA(&uart->huart,hdmarx,*hdma_usart_rx);
		HAL_UART_Receive_DMA(&uart->huart,rx_fifo->buffer,serial->config.bufsz);
	}
}



