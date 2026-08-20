#include "drv_can1.h"
#include "parameter.h"
#include "string.h"
#include "hc32_ll.h"
#include "bcmu_cur.h"

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/* Unit definition of CAN in this example. */
#define CAN_SEL_UNIT1                   (1U)
#define CAN_SEL_UNIT2                   (2U)

/* Select a CAN unit. */
#define CAN_UNIT_SEL                    (CAN_SEL_UNIT1)

/* Definitions according to 'CAN_UNIT_SEL'. */
#if (CAN_UNIT_SEL == CAN_SEL_UNIT1)
#define CAN_UNIT                        (CM_CAN1)
#define CAN_PERIPH_CLK                  (FCG1_PERIPH_CAN1)

#define CAN_TX_PORT                     (GPIO_PORT_E)
#define CAN_TX_PIN                      (GPIO_PIN_14)
#define CAN_TX_PIN_FUNC                 (GPIO_FUNC_60)

#define CAN_RX_PORT                     (GPIO_PORT_E)
#define CAN_RX_PIN                      (GPIO_PIN_15)
#define CAN_RX_PIN_FUNC                 (GPIO_FUNC_61)

#define CAN_CLK_UNIT                    (CLK_CAN1)
#define CAN_CLK_SRC                     (CLK_CANCLK_SYSCLK_DIV6)

/* configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 定义的5  不能比这个小 否则操作系统会死等 */
#define CAN_INT_PRIO                    (DDL_IRQ_PRIO_DEFAULT)//(DDL_IRQ_PRIO_03)    
#define CAN_INT_SRC                     (INT_SRC_CAN1_HOST)
#define CAN_INT_IRQn                    (INT092_IRQn)

#elif (CAN_UNIT_SEL == CAN_SEL_UNIT2)
#define CAN_UNIT                        (CM_CAN2)
#define CAN_PERIPH_CLK                  (FCG1_PERIPH_CAN2)

#define CAN_TX_PORT                     (GPIO_PORT_B)
#define CAN_TX_PIN                      (GPIO_PIN_13)
#define CAN_TX_PIN_FUNC                 (GPIO_FUNC_62)

#define CAN_RX_PORT                     (GPIO_PORT_B)
#define CAN_RX_PIN                      (GPIO_PIN_12)
#define CAN_RX_PIN_FUNC                 (GPIO_FUNC_63)

#define CAN_CLK_UNIT                    (CLK_CAN2)
#define CAN_CLK_SRC                     (CLK_CANCLK_SYSCLK_DIV6)

/* configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 定义的5  不能比这个小 否则操作系统会死等 */
#define CAN_INT_PRIO                    (DDL_IRQ_PRIO_DEFAULT)
#define CAN_INT_SRC                     (INT_SRC_CAN2_HOST)
#define CAN_INT_IRQn                    (INT092_IRQn)	
#else
#error "The unit is NOT supported!!!"
#endif

//中断类型
#define CAN_INT_SEL                     (CAN_INT_PTB_TX      | \
                                         CAN_INT_RX          | \
                                         CAN_INT_ERR_INT)
                                             
//接收数量
#define CAN_RX_FRAME_NUM                (8U)

can_handle_st can1;
SemaphoreHandle_t semCounting_can1recv; 

const static char *m_s8ErrorTypeStr[] = {
    "NO error.",
    "Bit Error.",
    "Form Error.",
    "Stuff Error.",
    "ACK Error.",
    "CRC Error.",
    "Other Error.",
    "Error type is NOT defined.",
};

/**
  ******************************************************************************
  * @brief  CAN interrupt callback.
  * @param  None
  * @return None
  * @note   
  ******************************************************************************
  */
static void CAN_IrqCallback(void)
{
	uint32_t u32Status;
    stc_can_error_info_t stcErr;

    u32Status = CAN_GetStatusValue(CAN_UNIT);
    if (u32Status != 0U) {
        CAN_ClearStatus(CAN_UNIT, u32Status);
    }
	//PTB发送
    if ((u32Status & CAN_FLAG_PTB_TX) != 0U) {
		stc_can_tx_frame_t stcTx1 = {0};
		frame_st frame;
		if(can1.tx_queue.read(&can1.tx_queue, &frame) == 0)
		{
			stcTx1.u32Ctrl = 0x0UL;
			stcTx1.IDE     = frame.frame_type; //扩展帧
			stcTx1.DLC     = frame.num_data;
			stcTx1.u32ID   = frame.id;
			memcpy(stcTx1.au8Data, frame.data, 8);
			CAN_FillTxFrame(CAN_UNIT, CAN_TX_BUF_PTB, &stcTx1);
			CAN_StartTx(CAN_UNIT, CAN_TX_REQ_PTB);
		}
	}
	//接收中断
    if ((u32Status & CAN_FLAG_RX) != 0U) {
		BaseType_t semRet;
		stc_can_rx_frame_t	stc_can_rx_frame;
		/* Get all received frames. */
		 while (CAN_GetRxFrame(CAN_UNIT, &stc_can_rx_frame) == LL_OK)
		 {
			frame_st frame;
			frame.id = stc_can_rx_frame.u32ID;
			memcpy(frame.data, stc_can_rx_frame.au8Data, 8);
			frame.frame_type = stc_can_rx_frame.IDE;
			frame.num_data = stc_can_rx_frame.DLC;
			
			if(can_filter(frame.id, frame.data) == 0)
			{
			    if(can1.rx_queue.write(&can1.rx_queue, &frame) == 0)
				{
					xSemaphoreGiveFromISR(semCounting_can1recv,&semRet);
				}
				else
				{
					can1.rx_queue.empty(&can1.rx_queue);
				}
			}
		}
        CAN_ClearStatus(CAN_UNIT, CAN_FLAG_RX);
    }
	if ((u32Status & CAN_FLAG_ERR_INT) != 0U) {
        if ((u32Status & CAN_FLAG_BUS_OFF) != 0U) {
            DDL_Printf(PRINT_LEVEL_DEBUG, "---> CAN BUS OFF\r\n");
        } else {
            (void)CAN_GetErrorInfo(CAN_UNIT, &stcErr);
            DDL_Printf(PRINT_LEVEL_DEBUG, "---> CAN error type: %u, %s\r\n", stcErr.u8ErrorType, m_s8ErrorTypeStr[stcErr.u8ErrorType]);
        }
    }
}

/**
 * @brief  Specifies communication clock.
 * @param  None
 * @retval None
 */
static void CanCommClockConfig(void)
{
    CLK_SetCANClockSrc(CAN_CLK_UNIT, CAN_CLK_SRC);
}

/**
 * @brief  Specifies pin function for TXD and RXD.
 * @param  None
 * @retval None
 */
static void CanPinConfig(void)
{
    GPIO_SetFunc(CAN_TX_PORT, CAN_TX_PIN, CAN_TX_PIN_FUNC);
    GPIO_SetFunc(CAN_RX_PORT, CAN_RX_PIN, CAN_RX_PIN_FUNC);
}

/**
 * @brief  CAN initial configuration.
 * @param  None
 * @retval None
 */
static void CanInitConfig(uint32_t baudrate)
{
    stc_can_init_t stcCanInit;
	stc_can_filter_config_t astcFilter[8] = {0};
	uint32_t CAN_FILTER_SEL = 0;

	/* Configure CAN filter according to different working modes */
	if (SysParaKeep.can1_config == KeGong_bmu_can)
	{
		astcFilter[0].u32ID = 0x1800C800UL;
		astcFilter[0].u32IDMask = 0x00FF00FFUL;
		astcFilter[0].u32IDType = CAN_ID_EXT;
		astcFilter[1].u32ID = 0x000003C2UL;
		astcFilter[1].u32IDMask = 0x0000000FUL;
		astcFilter[1].u32IDType = CAN_ID_STD_EXT;
		astcFilter[2].u32ID = 0x06FFFF00UL;
		astcFilter[2].u32IDMask = 0x000000FFUL;
		astcFilter[2].u32IDType = CAN_ID_EXT;
		CAN_FILTER_SEL = (CAN_FILTER1 | CAN_FILTER2 | CAN_FILTER3);
	}
	else if(SysParaKeep.can1_config == KeGong_ems_can)
	{
		astcFilter[0].u32ID = 0x18FF007F | (0x7f+SysParaKeep.ExtAddr)<<8;  //0x18XX807F
		astcFilter[0].u32IDMask = 0x00FF0000UL;
		astcFilter[0].u32IDType = CAN_ID_EXT;
		astcFilter[1].u32ID = 0x06FF00FF | (0x7f+SysParaKeep.ExtAddr)<<8;  //0x06FFXXFF
		astcFilter[1].u32IDMask = 0x0000FF00UL;
		astcFilter[1].u32IDType = CAN_ID_EXT;
		astcFilter[2].u32ID = 0x0000FF00UL;  //广播帧					   //0xXXXXFFXX
		astcFilter[2].u32IDMask = 0x1FFF00FFUL;
		astcFilter[2].u32IDType = CAN_ID_EXT;
		astcFilter[3].u32ID = 0x000003C2UL;//hall
		astcFilter[3].u32IDMask = 0x0000000FUL;
		astcFilter[3].u32IDType = CAN_ID_STD_EXT;
		astcFilter[4].u32ID = 0x1c8e2000UL;//运达升级复位命令
		astcFilter[4].u32IDMask = 0x00000FF0UL;
		astcFilter[4].u32IDType = CAN_ID_EXT;
		astcFilter[5].u32ID = 0x180000C8UL;//bmu透传命令
		astcFilter[5].u32IDMask = 0x00FFFF00UL;
		astcFilter[5].u32IDType = CAN_ID_EXT;
		CAN_FILTER_SEL = (CAN_FILTER1 | CAN_FILTER2 | CAN_FILTER3 | CAN_FILTER4 | CAN_FILTER5 | CAN_FILTER6);
	}
	else
	{
		astcFilter[0].u32ID      = 0;
		astcFilter[0].u32IDMask  = 0x1FFFFFFF;
		astcFilter[0].u32IDType  = CAN_ID_STD_EXT;
		CAN_FILTER_SEL = CAN_FILTER1;
	}
    

    /* Initializes CAN. */
    (void)CAN_StructInit(&stcCanInit);
	// 波特率配置
	// 0：根据协议自动变化（传参）
	// 125：125kbps
	// 250：250kbps
	// 500：500kbps
	// 1000：1000kbps
	if ((SysParaKeep.can1_baud == 0 && baudrate == 1000000)
	||  (SysParaKeep.can1_baud == 1000))
	{
		stcCanInit.stcBitCfg.u32Prescaler = 2U;//2U;
		stcCanInit.stcBitCfg.u32TimeSeg1  = 16U;
		stcCanInit.stcBitCfg.u32TimeSeg2  = 4U;
		stcCanInit.stcBitCfg.u32SJW       = 4U;
	}
	else if ((SysParaKeep.can1_baud == 0 && baudrate == 500000)
		 ||  (SysParaKeep.can1_baud == 500))
	{
		stcCanInit.stcBitCfg.u32Prescaler = 2U;
		stcCanInit.stcBitCfg.u32TimeSeg1  = 32U;
		stcCanInit.stcBitCfg.u32TimeSeg2  = 8U;
		stcCanInit.stcBitCfg.u32SJW       = 8U;
	}
	else if ((SysParaKeep.can1_baud == 0 && baudrate == 125000)
		 ||  (SysParaKeep.can1_baud == 125))
	{
		stcCanInit.stcBitCfg.u32Prescaler = 8U;//2U;
		stcCanInit.stcBitCfg.u32TimeSeg1  = 32U;
		stcCanInit.stcBitCfg.u32TimeSeg2  = 8U;
		stcCanInit.stcBitCfg.u32SJW       = 8U;
	}
	else//(baudrate == 250000)
	{
		stcCanInit.stcBitCfg.u32Prescaler = 4U;
		stcCanInit.stcBitCfg.u32TimeSeg1  = 32U;
		stcCanInit.stcBitCfg.u32TimeSeg2  = 8U;
		stcCanInit.stcBitCfg.u32SJW       = 8U;
	}
    stcCanInit.pstcFilter             = astcFilter;
    stcCanInit.u16FilterSelect        = CAN_FILTER_SEL;
    stcCanInit.u8WorkMode             = CAN_WORK_MD_NORMAL;

    /* Enable peripheral clock of CAN. */
    FCG_Fcg1PeriphClockCmd(CAN_PERIPH_CLK, ENABLE);
    (void)CAN_Init(CAN_UNIT, &stcCanInit);
    /* Enable the interrupts, the status flags can be read. */
    CAN_IntCmd(CAN_UNIT, CAN_INT_ALL, DISABLE);
    /* Enalbe the interrupts that needed. */
    CAN_IntCmd(CAN_UNIT, CAN_INT_SEL, ENABLE);
}

static void CanIrqConfig(void)
{
    stc_irq_signin_config_t stcIrq;

    stcIrq.enIntSrc    = CAN_INT_SRC;
    stcIrq.enIRQn      = CAN_INT_IRQn;
    stcIrq.pfnCallback = &CAN_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrq);
    NVIC_ClearPendingIRQ(stcIrq.enIRQn);
    NVIC_SetPriority(stcIrq.enIRQn, CAN_INT_PRIO);
	NVIC_EnableIRQ(stcIrq.enIRQn);
}

/**
 * @brief  Set CAN PHY STB pin as low.
 * @param  None
 * @retval None
 */
static void CanPhyEnable(void)
{
#if (BSP_TCA9539_ENABLE == DDL_ON)		
//    BSP_IO_Init();
    BSP_CAN_STB_IO_Init();

    /* Set PYH STB pin as low. */
    BSP_CAN_STBCmd(EIO_PIN_RESET);
#endif	
}

/**
  ******************************************************************************
  * @brief  can初始化函数
  * @param  this: CAN句柄指针
  * @param  baudrate: CAN波特率（预留，当前使用固定配置）
  * @return None
  * @note   配置CAN外设、中断和信号量
  ******************************************************************************
  */
void drv_can1_init(can_handle_st *this, uint32_t baudrate)
{
	(void)this;
	
    /* Configures CAN. */
    CanCommClockConfig();
    CanPinConfig();
    CanInitConfig(baudrate);
    CanIrqConfig();
    CanPhyEnable();
	
	if(semCounting_can1recv == NULL)
	{
		semCounting_can1recv = xSemaphoreCreateCounting(255,0);  //最大255 当前0
	}
}

/**
  ******************************************************************************
  * @brief  can接收数据
  * @param  this: CAN句柄指针
  * @param  id: CAN ID指针
  * @param  data: 数据缓冲区指针
  * @return 0:成功 1:失败/超时
  * @note   默认超时300ms
  ******************************************************************************
  */
uint8_t drv_can1_receive(can_handle_st *this, uint32_t *id, uint8_t *data)
{
	if(this == NULL || id == NULL || data == NULL)
	{
		return 1;
	}
	
	if(xSemaphoreTake(semCounting_can1recv, (TickType_t)300) == pdTRUE)
	{
		uint8_t ret_u8 = 0;
		frame_st frame;
		taskENTER_CRITICAL();
		ret_u8 = this->rx_queue.read(&this->rx_queue, &frame);
		taskEXIT_CRITICAL();
		
		if(ret_u8 == 0)
		{
			*id = frame.id;
			memcpy(data, frame.data, 8);
			return 0;
		}
		return 1;
	}
	return 1;
}

/**
  ******************************************************************************
  * @brief  can接收数据（详细信息）
  * @param  this: CAN句柄指针
  * @param  frame: 帧结构体指针
  * @param  ms: 超时时间（毫秒）
  * @return 0:成功 1:失败/超时
  * @note   返回完整的帧信息（ID、数据、帧类型、数据长度）
  ******************************************************************************
  */
uint8_t drv_can1_receive_detail(can_handle_st *this, frame_st *frame, uint16_t ms)
{
	if(this == NULL || frame == NULL)
	{
		return 1;
	}
	
	if(xSemaphoreTake(semCounting_can1recv, (TickType_t)ms) == pdTRUE)
	{
		uint8_t ret_u8 = 0;
		taskENTER_CRITICAL();
		ret_u8 = this->rx_queue.read(&this->rx_queue, frame);
		taskEXIT_CRITICAL();
		
		if(ret_u8 == 0)
		{
			return 0;
		}
		return 1;
	}
	return 1;
}

/**
  ******************************************************************************
  * @brief  can发送数据
  * @param  this: CAN句柄指针
  * @param  id: CAN ID
  * @param  data: 数据缓冲区指针
  * @return 0:成功 1:失败
  * @note   默认发送扩展帧，8字节数据
  ******************************************************************************
  */
uint8_t drv_can1_send(can_handle_st *this, uint32_t id, uint8_t *data)
{
	if(this == NULL || data == NULL)
	{
		return 1;
	}
	
	// 检查PTB发送缓冲区是否空闲
	if(READ_REG8_BIT(CAN_UNIT->TCMD, CAN_TCMD_TPE) == 0U)
	{
		// 直接发送
		stc_can_tx_frame_t stcTx1 = {0};
		stcTx1.u32Ctrl = 0x0UL;
		stcTx1.IDE     = 1; //扩展帧
		stcTx1.u32ID   = id;
		stcTx1.DLC     = CAN_DLC8;
		memcpy(stcTx1.au8Data, data, 8);
		uint8_t ret = CAN_FillTxFrame(CAN_UNIT, CAN_TX_BUF_PTB, &stcTx1);
		CAN_StartTx(CAN_UNIT, CAN_TX_REQ_PTB);
		return 0;
	}
	
	// PTB忙，写入发送队列
	frame_st frame;
	frame.id = id;
	memcpy(frame.data, data, 8);
	frame.frame_type = 1; // 扩展帧
	frame.num_data = CAN_DLC8;
	
	uint8_t ret_u8 = 0;
	taskENTER_CRITICAL();
	ret_u8 = this->tx_queue.write(&this->tx_queue, &frame);
	taskEXIT_CRITICAL();
	
	if(ret_u8 != 0)
	{
		// 队列满，清空后重试
		this->tx_queue.empty(&this->tx_queue);
		return 1;
	}
	return 0;
}
/**
  ******************************************************************************
  * @brief  can发送数据（详细信息）
  * @param  this: CAN句柄指针
  * @param  frame: 帧结构体指针
  * @param  ms: 超时时间（预留，当前未使用）
  * @return 0:成功 1:失败
  * @note   支持自定义帧类型、数据长度
  ******************************************************************************
  */
uint8_t drv_can1_send_detail(can_handle_st *this, frame_st *frame, uint16_t ms)
{
	(void)ms; // 预留参数
	
	if(this == NULL || frame == NULL)
	{
		return 1;
	}
	
	// 检查PTB发送缓冲区是否空闲
	if(READ_REG8_BIT(CAN_UNIT->TCMD, CAN_TCMD_TPE) == 0U)
	{
		// 直接发送
		stc_can_tx_frame_t stcTx1 = {0};
		stcTx1.u32Ctrl = 0x0UL;
		stcTx1.IDE     = frame->frame_type;
		stcTx1.u32ID   = frame->id;
		stcTx1.DLC     = frame->num_data;
		memcpy(stcTx1.au8Data, frame->data, 8);
		uint8_t ret = CAN_FillTxFrame(CAN_UNIT, CAN_TX_BUF_PTB, &stcTx1);
		CAN_StartTx(CAN_UNIT, CAN_TX_REQ_PTB);
		return 0;
	}
	
	// PTB忙，写入发送队列
	uint8_t ret_u8 = 0;
	taskENTER_CRITICAL();
	ret_u8 = this->tx_queue.write(&this->tx_queue, frame);
	taskEXIT_CRITICAL();
	
	if(ret_u8 != 0)
	{
		// 队列满，清空后重试
		this->tx_queue.empty(&this->tx_queue);
		return 1;
	}
	return 0;
}

/* Returns:
 *  0 - 正常帧，调用方入队
 *  1 - 已处理（hall电流），调用方跳过
 *  2 - 广播限速，调用方break接收循环
 */
uint8_t can_filter(uint32_t id, uint8_t *data)
{
	if(id == (0x18017F7F + ((uint32_t)SysParaKeep.ExtAddr << 8))
	|| id == (0x1801807F + ((uint32_t)SysParaKeep.ExtAddr << 8)))
	{
		if(SysParaVoltiage.ms_cnt - SysParaVoltiage.can_ms <= 900)
			return 2;
		else
			SysParaVoltiage.can_ms = SysParaVoltiage.ms_cnt;
	}
	if(id == 0x3C2)
	{
		can_hall_get_cur(&g_canhall, data);
		return 1;
	}

	return 0;
}
