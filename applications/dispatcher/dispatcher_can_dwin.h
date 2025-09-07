/**
 * @file dispatcher_can_dwin.h
 * @brief 代码功能简介
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __DISPATCHER_CAN_DWIN_H__
#define __DISPATCHER_CAN_DWIN_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/* 自定义的函数指针类型，这种类型的变量传入的参数是id、*buff、size，当某个函数的参数与这种类型的变量的参数相同，这个类型的变量就指向那个函数 */
typedef void (*can_data_parser_hook)(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size);
/* 自定义的函数指针类型，这种类型的变量传入的参数是id、*buff、size，当某个函数的参数与这种类型的变量的参数相同，这个类型的变量就指向那个函数 */
typedef void (*dwin_data_parser_hook)(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size);
/**
 * @brief CAN消息分发器配置
 * @var id    CAN消息ID
 * @var hook  对应的处理函数
 */
typedef struct can_dispatcher
{
	rt_uint32_t id;				//这个id用来与接收到的数据的id进行比对
	can_data_parser_hook hook;	//hook的参数是id、*buff、size，用来指向参数相同的函数
}can_dispatcher_t;
/**
 * @brief 迪文屏自动加载数据分发器配置
 * @var address 迪文屏寄存器地址
 * @var hook    对应的处理函数
 */
typedef struct dwin_dispatcher
{
	rt_uint32_t address;		//这个address用来与接收到的数据的address进行比对
	dwin_data_parser_hook hook;	//hook的参数是id、*buff、size，用来指向参数相同的函数
}dwin_dispatcher_t;
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/*分发器系统初始化函数*/
void init_can_dispatcher(can_dispatcher_t *list, rt_size_t count);
void init_dwin_dispatcher(dwin_dispatcher_t *list, rt_size_t count);
/*分发器处理函数*/
void can_data_parser(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size);
void dwin_auto_load_data_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size);
/*默认分发器处理函数，用于调试时输出提示信息*/
void default_can_data_parser(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size);
void default_dwin_auto_load_data_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __DISPATCHER_CAN_DWIN_H__ */
