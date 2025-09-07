/**
 * @file interface_dwin.h
 * @brief 迪文屏串口输入输出
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __INTERFACE_DWIN_H__
#define __INTERFACE_DWIN_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/* 协议帧格式定义 */
#define DWIN_DATA_FRAME_MAX_LENGTH		255		//迪文数据帧最大长度
#define DWIN_DATA_BYTE_COUNT_INDEX		2		//迪文屏数据字节总数对应的字节所在数据数组的位置，也就是数据列表第三个元素位置
#define DWIN_COMMAND_WRITE				0x82	//写指令，也就是数据列表第四个元素位置
#define DWIN_COMMAND_READ				0x83	//读指令，也就是数据列表第四个元素位置
#define DWIN_DATA_FRAME_ADDRESS_INDEX	4		//迪文数据帧地址对应的字节所在数据数组的位置，也就是第五、第六字节
#define DWIN_DATA_COUNT_INDEX			6		//读迪文数据的第七字节，是读取字节的总数，写迪文数据没有写入字节个数的规则
#define DWIN_DATA_OFFSET_INDEX			7		//读数据偏移量，也就是第八个字节开始才是读到的数据
#define DWIN_WRITE_DATA_OFFSET			6		//写数据偏移量，也就是第七个字节开始才是写入的数据

#define SWAP_16(val)	((((val) & 0xFF) << 8) | (((val) >> 8) & 0xFF))//两字节数据存储地址调换
/*============================ TYPES =========================================*/
//迪文数据格式结构体
typedef struct dwin_data_frame_format
{
	rt_uint16_t head;			//帧头
	rt_uint8_t byte_count;		//后续字节数
	rt_uint8_t command;			//读或写指令
	rt_uint16_t var_address;	//迪文变量地址
}dwin_data_frame_format_t;
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
void init_dwin_serial(void);
void dwin_serial_send(rt_uint8_t *buff, rt_uint32_t size);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __INTERFACE_DWIN_H__ */
