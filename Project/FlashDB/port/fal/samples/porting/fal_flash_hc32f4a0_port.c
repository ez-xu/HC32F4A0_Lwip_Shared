/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fal.h>
#include "hc32_ll.h"
#include "cmsis_os.h"
#include "drv_gpio.h"

/*
HC32F4A0 Flash特性:
    - Flash扇区大小: 8KB (0x2000)
    - 写入粒度: 64 bit (8字节)
    - 擦除单位: 扇区 (Sector)
    - 写入前必须先擦除
    - 支持单扇区操作
*/

#define FLASH_SECTOR_SIZE   (8192U)    // 8KB per sector
#define FLASH_WRITE_UNIT    (8U)       // 8 bytes per write
#define FLASH_TIMEOUT       (5000U)    // Flash操作超时时间(ms)

static int ef_err_port_cnt = 0;
int on_ic_read_cnt  = 0;
int on_ic_write_cnt = 0;

// 喂狗函数,用户可根据实际看门狗实现
void feed_dog(void)
{
    // 如果有看门狗需要在这里喂狗
    WTD_Reset();
}

/**
 * @brief  初始化Flash设备
 * @retval 1: 成功, 0: 失败
 */
static int init(void)
{
    /* HC32F4A0 Flash无需特殊初始化 */
    return 1;
}

/**
 * @brief  从Flash读取数据
 * @param  offset: 相对于Flash起始地址的偏移量
 * @param  buf: 读取数据的缓冲区
 * @param  size: 读取的字节数
 * @retval 实际读取的字节数
 */
static int read(long offset, uint8_t *buf, size_t size)
{
    size_t i;
    uint32_t addr = hc32_onchip_flash.addr + offset;

    // 检查地址对齐(虽然读取不强制要求4字节对齐,但建议对齐)
    if (addr % 4 != 0)
    {
        ef_err_port_cnt++;
    }

    // 直接从Flash地址读取数据
    for (i = 0; i < size; i++, addr++, buf++)
    {
        *buf = *(uint8_t *)addr;
        /* 每256字节喂一次狗，防止GC/recovery大量read时看门狗超时 */
        if ((i & 0xFF) == 0)
        {
            feed_dog();
        }
    }
    
    on_ic_read_cnt++;
    return size;
}

/**
 * @brief  向Flash写入数据
 * @param  offset: 相对于Flash起始地址的偏移量
 * @param  buf: 要写入的数据缓冲区
 * @param  size: 写入的字节数
 * @retval 成功返回写入字节数, 失败返回-1
 * @note   HC32F4A0要求8字节对齐写入
 */
static int write(long offset, const uint8_t *buf, size_t size)
{
    size_t i, j = 0;
    uint32_t addr = hc32_onchip_flash.addr + offset;
    uint32_t data_write[2];  // 8字节数据缓冲区
    uint32_t read_data[2];
    uint16_t timeout;
    en_int_status_t flag1, flag2;

    // 检查地址是否4字节对齐
    if (addr % 4 != 0)
    {
        ef_err_port_cnt++;
        return -1;
    }

    // 等待Flash就绪
    timeout = 0;
    do {
        flag1 = EFM_GetStatus(EFM_FLAG_RDY);
        flag2 = EFM_GetStatus(EFM_FLAG_RDY1);
        if (++timeout > FLASH_TIMEOUT)
        {
            return -1;
        }
    } while ((SET != flag1) || (SET != flag2));

	taskENTER_CRITICAL();
	
    // 使能Flash写操作
    EFM_FWMC_Cmd(ENABLE);

    // 按4字节单位写入
    for (i = 0; i < size; i += 4, buf += 4, addr += 4)
    {
        // 拷贝数据到对齐的缓冲区
        memcpy((uint8_t *)data_write, buf, 8);

		EFM_SingleSectorOperateCmd(addr / FLASH_SECTOR_SIZE, ENABLE);
		
        // 写入4字节数据
        if (LL_OK != EFM_ProgramWord(addr, data_write[j]))
		{
			size = 0;
			break;
		}
		
		EFM_SingleSectorOperateCmd(addr / FLASH_SECTOR_SIZE, DISABLE);

        // 读回校验
        read_data[0] = *(uint32_t *)addr;
        
        if (read_data[0] != data_write[0])
        {
            size = 0;
			break;
        }

        // Flash操作可能耗时较长,喂狗防止超时
        feed_dog();
    }

    // 禁止Flash写操作
    EFM_FWMC_Cmd(DISABLE);

	taskEXIT_CRITICAL();
	
    on_ic_write_cnt++;
    return size;
}

/**
 * @brief  擦除Flash扇区
 * @param  offset: 相对于Flash起始地址的偏移量
 * @param  size: 擦除的字节数
 * @retval 成功返回擦除字节数, 失败返回-1
 * @note   HC32F4A0按扇区擦除,扇区大小为8KB
 */
static int erase(long offset, size_t size)
{
    uint32_t addr = hc32_onchip_flash.addr + offset;
    size_t erase_sectors, i;
    uint16_t timeout;
    en_int_status_t flag1, flag2;
    int32_t result;

    // 计算需要擦除的扇区数
    erase_sectors = size / FLASH_SECTOR_SIZE;
    if (size % FLASH_SECTOR_SIZE != 0)
    {
        erase_sectors++;
    }

    // 等待Flash就绪
    timeout = 0;
    do {
        flag1 = EFM_GetStatus(EFM_FLAG_RDY);
        flag2 = EFM_GetStatus(EFM_FLAG_RDY1);
        if (++timeout > FLASH_TIMEOUT)
        {
            return -1;
        }
    } while ((SET != flag1) || (SET != flag2));

	taskENTER_CRITICAL();
	
    // 使能Flash写操作
    EFM_FWMC_Cmd(ENABLE);

    // 逐个扇区擦除
    for (i = 0; i < erase_sectors; i++)
    {
        uint32_t sector_addr = addr + (FLASH_SECTOR_SIZE * i);
        uint32_t sector_num = sector_addr / FLASH_SECTOR_SIZE;

        // 禁用该扇区的写保护
        EFM_SingleSectorOperateCmd(sector_num, ENABLE);

        // 执行扇区擦除
        result = EFM_SectorErase(sector_addr);
        if (result != LL_OK)
        {
            size = 0;
			break;
        }

        // Flash操作可能耗时较长,喂狗防止超时
        feed_dog();
    }

    // 禁止Flash写操作
    EFM_FWMC_Cmd(DISABLE);
	
	taskEXIT_CRITICAL();

    return size;
}

/*
  HC32F4A0 Flash设备定义说明:
  
  "hc32_onchip" : Flash 设备的名字
  0x00000000: 对 Flash 操作的起始地址 (实际地址需根据芯片配置)
  512*1024: Flash 的总大小 (512KB, 根据实际芯片型号调整)
  8*1024: Flash 扇区大小 (8KB)
  {init, read, write, erase}: Flash 的操作函数
  64: 设置写粒度，单位 bit (HC32F4A0为64bit即8字节)
 */

const struct fal_flash_dev hc32_onchip_flash =
{
    .name       = "hc32_onchip",
    .addr       = 0x00000000,        // Flash起始地址,根据实际分区调整
    .len        = 2024 * 1024,        // Flash总大小2MB
    .blk_size   = 8 * 1024,          // 扇区大小8KB
    .ops        = {init, read, write, erase},
    .write_gran = 32                 // 写入粒度64bit
};
