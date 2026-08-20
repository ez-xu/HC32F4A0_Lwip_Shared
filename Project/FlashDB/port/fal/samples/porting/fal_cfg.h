/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_


/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev hc32_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &hc32_onchip_flash,	                                           	 \
}
/* ====================== Partition Configuration ========================== */
#define FAL_PART_HAS_TABLE_CFG
#ifdef FAL_PART_HAS_TABLE_CFG
/* 运行时双分区表方案：两张表都编译进固件，由fal_partition.c根据
 * g_flash_layout_version(在initandset.c中检测boot区版本号设置)运行时选择：
 * - V1(400K/B002): app 400K + app_bkup 400K，fdb_tsdb1在0x100000占1MB
 * - V2(800K/B003): app 800K(合并旧app+app_bkup)，app_bkup移到0x100000占800K，fdb_tsdb1缩减到224K
 * 其他分区(update/keep/soc/log/fdb_kvdb1)地址不变
 */
/* 布局版本标志（g_flash_layout_version的取值） */
#define FLASH_LAYOUT_V1        1   /* V1布局: B002/400K, 备份区0x06C000, TSDB 0x100000/1MB */
#define FLASH_LAYOUT_V2        2   /* V2布局: B003/800K, 备份区0x100000, TSDB 0x1C8000/224K */
/* V1布局(400K) - Bootloader为B002时使用 */
#define FAL_PART_TABLE_V1                                                                      \
{                                                                                           \
    {FAL_PART_MAGIC_WORD,        "boot",     "hc32_onchip",  0x000000,   32*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "app",     "hc32_onchip",  0x008000,  400*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "app_bkup",     "hc32_onchip",  0x06C000,  400*1024, 0},      \
    {FAL_PART_MAGIC_WORD,      "update",     "hc32_onchip",  0x0D0000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,        "keep",     "hc32_onchip",  0x0D2000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "keep_bkup",     "hc32_onchip",  0x0D4000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "soc",     "hc32_onchip",  0x0D6000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "soc_bkup",     "hc32_onchip",  0x0D8000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "log",     "hc32_onchip",  0x0DA000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "log_bkup",     "hc32_onchip",  0x0DC000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "fdb_kvdb1",     "hc32_onchip",  0x0DE000,  136*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "fdb_tsdb1",     "hc32_onchip",  0x100000, 1024*1024, 0},      \
}
/* V2布局(800K) - Bootloader为B003时使用（默认） */
#define FAL_PART_TABLE_V2                                                                      \
{                                                                                           \
    {FAL_PART_MAGIC_WORD,        "boot",     "hc32_onchip",  0x000000,   32*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "app",     "hc32_onchip",  0x008000,  800*1024, 0},      \
    {FAL_PART_MAGIC_WORD,      "update",     "hc32_onchip",  0x0D0000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,        "keep",     "hc32_onchip",  0x0D2000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "keep_bkup",     "hc32_onchip",  0x0D4000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "soc",     "hc32_onchip",  0x0D6000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "soc_bkup",     "hc32_onchip",  0x0D8000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,         "log",     "hc32_onchip",  0x0DA000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "log_bkup",     "hc32_onchip",  0x0DC000,    8*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "fdb_kvdb1",     "hc32_onchip",  0x0DE000,  136*1024, 0},      \
    {FAL_PART_MAGIC_WORD,    "app_bkup",     "hc32_onchip",  0x100000,  800*1024, 0},      \
    {FAL_PART_MAGIC_WORD,   "fdb_tsdb1",     "hc32_onchip",  0x1C8000,  224*1024, 0},      \
}
/* 兼容性保留：fal_partition.c已改为运行时选择V1/V2，此处保留FAL_PART_TABLE指向V2 */
#define FAL_PART_TABLE FAL_PART_TABLE_V2
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
