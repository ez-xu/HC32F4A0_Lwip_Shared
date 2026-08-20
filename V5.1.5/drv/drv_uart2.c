#include "drv_uart2.h"
#include "parameter.h"
#include "hc32_ll.h"
#include "ev_hc32f4a0_lqfp176_bsp.h"

/* USART RX/TX pin definition */
#define USART_RX_PORT                   (GPIO_PORT_D)
#define USART_RX_PIN                    (GPIO_PIN_14)
#define USART_RX_GPIO_FUNC              (GPIO_FUNC_35)

#define USART_TX_PORT                   (GPIO_PORT_D)
#define USART_TX_PIN                    (GPIO_PIN_13)
#define USART_TX_GPIO_FUNC              (GPIO_FUNC_34)

/* USART unit definition */
#define USART_UNIT                      (CM_USART8)
#define USART_FCG_ENABLE()              (FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART8, ENABLE))

/* USART interrupt definition */
#define USART_RX_ERR_IRQn               (INT012_IRQn)
#define USART_RX_ERR_INT_SRC            (INT_SRC_USART8_EI)

#define USART_RX_FULL_IRQn              (INT013_IRQn)
#define USART_RX_FULL_INT_SRC           (INT_SRC_USART8_RI)

#define USART_TX_EMPTY_IRQn             (INT014_IRQn)
#define USART_TX_EMPTY_INT_SRC          (INT_SRC_USART8_TI)

#define USART_TX_CPLT_IRQn              (INT015_IRQn)
#define USART_TX_CPLT_INT_SRC           (INT_SRC_USART8_TCI)
		
#define RS485_EN_PORT					(GPIO_PORT_D)
#define RS485_EN_PIN					(GPIO_PIN_12)

#define RS485_EN_TX()					GPIO_SetPins(RS485_EN_PORT, RS485_EN_PIN)
#define RS485_EN_RX()					GPIO_ResetPins(RS485_EN_PORT, RS485_EN_PIN)

modbus_st uart2;
SemaphoreHandle_t semCounting_Usart2recv;
SemaphoreHandle_t semCounting_Usart2send;
int32_t drv_uart2_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate);

static void USART_RxFull_IrqCallback(void)
{
	uint16_t u16Data;
	long semRet;
	
	u16Data = (uint8_t)USART_ReadData(USART_UNIT);
	if(uart2.rece_ptr < MODBUS_BUF_SIZE)
	{
		uart2.rece_buf[uart2.rece_ptr++] = u16Data;
		xSemaphoreGiveFromISR(semCounting_Usart2recv,&semRet);
	}
	USART_ClearStatus(USART_UNIT,USART_FLAG_RX_FULL);
}

static void USART_TxEmpty_IrqCallback(void)
{	
	if (uart2.send_ptr < uart2.send_bytes)
	{
		USART_WriteData(USART_UNIT, (uint16_t)uart2.send_buf[uart2.send_ptr++]);
	}
	else
	{
		USART_FuncCmd(USART_UNIT, USART_INT_TX_CPLT, ENABLE);
	}
}

static void USART_TxComplete_IrqCallback(void)
{
	long semRet;
	
    USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_CPLT | USART_INT_TX_EMPTY), DISABLE);
	xSemaphoreGiveFromISR(semCounting_Usart2send,&semRet);
}

static void USART_RxError_IrqCallback(void)
{
    if (SET == USART_GetStatus(USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR))) {
        (void)USART_ReadData(USART_UNIT);
    }

    USART_ClearStatus(USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void INTC_IrqInstalHandler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) {
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

void drv_uart2_init(struct modbus* this)
{
	uint32_t u32Baudrate;
    stc_usart_uart_init_t stcUartInit;
    stc_irq_signin_config_t stcIrqSigninConfig;
	stc_gpio_init_t stcGpioInit;

	/* Configure USART RX/TX pin. */
    GPIO_SetFunc(USART_RX_PORT, USART_RX_PIN, USART_RX_GPIO_FUNC);
    GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);
	
	/* Enable peripheral clock */
    USART_FCG_ENABLE();
	
    /* Initialize UART. */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32ClockDiv = USART_CLK_DIV64;
	switch(SysParaKeep.Uart2Baud)
	{
		case 0: u32Baudrate = 4800; break;
		case 1: u32Baudrate = 9600; break;
		case 2: u32Baudrate = 19200; break;
		case 3: u32Baudrate = 38400; break;
		case 4: u32Baudrate = 57600; break;
		case 5: u32Baudrate = 115200; break;
		default: u32Baudrate = 9600; break;
	}	
	if(SysParaKeep.TestModeEnable == 1)
    {
		u32Baudrate = 9600;
	}
    stcUartInit.u32Baudrate = u32Baudrate;//9600UL;//115200UL;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_UART_Init(USART_UNIT, &stcUartInit, NULL)) {
		;
    }
	
    /* Register RX error IRQ handler && configure NVIC. */
    stcIrqSigninConfig.enIRQn = USART_RX_ERR_IRQn;
    stcIrqSigninConfig.enIntSrc = USART_RX_ERR_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &USART_RxError_IrqCallback;
    INTC_IrqInstalHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

    /* Register RX full IRQ handler && configure NVIC. */
    stcIrqSigninConfig.enIRQn = USART_RX_FULL_IRQn;
    stcIrqSigninConfig.enIntSrc = USART_RX_FULL_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &USART_RxFull_IrqCallback;
    INTC_IrqInstalHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

    /* Register TX empty IRQ handler && configure NVIC. */
    stcIrqSigninConfig.enIRQn = USART_TX_EMPTY_IRQn;
    stcIrqSigninConfig.enIntSrc = USART_TX_EMPTY_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &USART_TxEmpty_IrqCallback;
    INTC_IrqInstalHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

    /* Register TX complete IRQ handler && configure NVIC. */
    stcIrqSigninConfig.enIRQn = USART_TX_CPLT_IRQn;
    stcIrqSigninConfig.enIntSrc = USART_TX_CPLT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &USART_TxComplete_IrqCallback;
    INTC_IrqInstalHandler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);
	
    /* Enable RX function */
    USART_FuncCmd(USART_UNIT, (USART_RX | USART_INT_RX), ENABLE);
	
	// 收发使能
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(RS485_EN_PORT, RS485_EN_PIN, &stcGpioInit);	
	
	if(semCounting_Usart2recv == NULL)
	{
		semCounting_Usart2recv = xSemaphoreCreateCounting(1,0);   //最大1   当前0
	}
	if(semCounting_Usart2send == NULL)
	{
		semCounting_Usart2send = xSemaphoreCreateCounting(1,0);   //最大1   当前1
	}

	if(SysParaKeep.uart2_config == Printf_uart_DISENABLE
	|| SysParaKeep.uart2_config == Printf_uart_ERROR
	|| SysParaKeep.uart2_config == Printf_uart_INFO
	|| SysParaKeep.uart2_config == Printf_uart_DEBUG
	){
		DDL_PrintfInit(USART_UNIT, u32Baudrate, drv_uart2_PRINTF_Preinit);
	}
}
int32_t drv_uart2_PRINTF_Preinit(void *vpDevice, uint32_t u32Baudrate)
{
    uint32_t u32Div;
    float32_t f32Error;
    stc_usart_uart_init_t stcUartInit;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    (void)vpDevice;

    if (0UL != u32Baudrate) {
        /* Set TX port function */
        GPIO_SetFunc(USART_TX_PORT, USART_TX_PIN, USART_TX_GPIO_FUNC);

        /* Enable clock  */
		USART_FCG_ENABLE();

        /* Configure UART */
        (void)USART_UART_StructInit(&stcUartInit);
        stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
        (void)USART_UART_Init(USART_UNIT, &stcUartInit, NULL);

        for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
            USART_SetClockDiv(USART_UNIT, u32Div);
            i32Ret = USART_SetBaudrate(USART_UNIT, u32Baudrate, &f32Error);
            if ((LL_OK == i32Ret) && \
                ((-BSP_PRINTF_BAUDRATE_ERR_MAX <= f32Error) && (f32Error <= BSP_PRINTF_BAUDRATE_ERR_MAX))) {
                USART_FuncCmd(USART_UNIT, USART_TX, ENABLE);
                break;
            } else {
                i32Ret = LL_ERR;
            }
        }
    }
	// 收发使能
	stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(RS485_EN_PORT, RS485_EN_PIN, &stcGpioInit);	
	RS485_EN_TX();
    return i32Ret;
}

uint8_t drv_uart2_read(master_slave_mode mode, struct modbus* this)
{
	this->rece_ptr = 0;
	RS485_EN_RX();
	
	const int totalTimeout = 1000;      // 总超时时间1000ms
    const int noDataTimeout = 50;       // 无数据超时时间20ms
	uint32_t lastMs = SysParaVoltiage.ms_cnt;
	/*处于主机模式时，在read里必须待够55ms，实测所有波特率非常稳定*/
	/*处于从机模式时，为了满足3台uart同时通讯需要保证10ms接收不到数据就退出，实测所有波特率485接上非常稳定*/
	if(mode == MASTER)
	{
		vTaskDelay(pdMS_TO_TICKS(45));
	}
	while(SysParaVoltiage.ms_cnt - lastMs < totalTimeout)
	{
		if (xSemaphoreTake(semCounting_Usart2recv,(TickType_t)noDataTimeout) == pdFALSE
		&& this->rece_ptr >= 5)
		{
			// 已有数据且20ms内无新数据，认为接收完成
			this->rece_bytes = this->rece_ptr;
			return OK;
		}
	}
	return ERROR_READ;
}

uint8_t drv_uart2_write(struct modbus* this)
{
	this->send_ptr = 0;
	RS485_EN_TX();
	
	USART_FuncCmd(USART_UNIT, (USART_TX | USART_INT_TX_EMPTY), ENABLE);
	if(xSemaphoreTake(semCounting_Usart2send,(TickType_t)1000) == pdTRUE)
	{
		return OK;
	}
	
	return ERROR_WRITE;
}

