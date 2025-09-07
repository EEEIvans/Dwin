/**
 * @file interface_curve.h
 * @brief 迪文屏曲线功能
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __INTERFACE_CURVE_H__
#define __INTERFACE_CURVE_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#define DWIN_CURVE_DATA_MAX_COUNT	20	//一条曲线最多只读取20个数据，也就是曲线显示的只有最新的20个数据
#define MAX_COUNT DWIN_CURVE_DATA_MAX_COUNT
#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
#define DWIN_CURVE_MAX_COUNT			32		//曲线最大数量
#define DWIN_CURVE_WINDOW_MAX_COUNT		16		//曲线窗口最大数量
#define DWIN_CURVE_IN_WINDOW_MAX_COUNT	8		//单个窗口曲线最大数量
#define DWIN_CURVE_CHANNEL_MAX_COUNT	8		//单个窗口曲线通道数
/* 迪文屏曲线通道地址 */
#define DWIN_CURVE_CHANNEL1		0x0301
#define DWIN_CURVE_CHANNEL2		0x0303
#define DWIN_CURVE_CHANNEL3		0x0305
#define DWIN_CURVE_CHANNEL4		0x0307
#define DWIN_CURVE_CHANNEL5		0x0309
#define DWIN_CURVE_CHANNEL6		0x030B
#define DWIN_CURVE_CHANNEL7		0x030D
#define DWIN_CURVE_CHANNEL8		0x030F
//判断传入的 val 是否属于预定义的8个DWIN曲线通道值之一
#define IS_DWIN_CURVE_CHANNEL(val)	(DWIN_CURVE_CHANNEL1 == (val) \
	|| DWIN_CURVE_CHANNEL2 == (val) \
	|| DWIN_CURVE_CHANNEL3 == (val) \
	|| DWIN_CURVE_CHANNEL4 == (val) \
	|| DWIN_CURVE_CHANNEL5 == (val) \
	|| DWIN_CURVE_CHANNEL6 == (val) \
	|| DWIN_CURVE_CHANNEL7 == (val) \
	|| DWIN_CURVE_CHANNEL8 == (val) \
	)
/*============================ TYPES =========================================*/
/* 定义函数指针类型，当被指向的函数的参数为rt_uint16_t value，被指向的函数执行相关功能 */ 
typedef rt_uint16_t (*curve_data_adjust_t)(rt_uint16_t value);

/* 曲线数据结构体，配置曲线的一些相关功能*/
typedef struct curve_data
{
	struct curve_data_queue queue;	// 曲线数据队列，队列本身的初始化以及用于添加、读取曲线数据
	rt_uint16_t curve_channel;		// 曲线通道
	rt_mutex_t mutex;				// 互斥量，曲线数据存数据和取数据可能是在不同线程进行的，使用互斥量让添加数据取数据互斥进行能保证线程安全
	curve_data_adjust_t adjust_fun;	// 数值转换函数,把can接收到的数据转换成迪文屏能显示的数据
}curve_data_t;
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/* 曲线显示 */
void curve_show(rt_int16_t curve_window_id, rt_bool_t all_data);

/* 曲线窗口第一次显示 */
rt_bool_t is_curve_window_first_show(rt_int16_t curve_window_id);
/* 曲线窗口非第一次显示 */
void set_curve_window_not_first_show(rt_int16_t curve_window_id);

/* 获取曲线窗口id */
rt_int16_t get_current_curve_window(void);
/* 设置曲线窗口id */
void set_current_curve_window(rt_int16_t curve_window_id);

/* 添加曲线数据 */
void add_curve_data(rt_uint16_t curve_id, rt_uint16_t data);
/* 添加曲线到窗口 */
void add_curve_to_window(rt_uint16_t curve_id, rt_uint16_t curve_window_id);

/* 清空曲线 */
void clean_curve(void);

/* 初始化曲线配置 */
void init_curve(rt_uint16_t curve_id, rt_uint16_t curve_channel, curve_data_adjust_t adjust_fun);

/* 默认曲线数据 */
rt_uint16_t default_curve_data_adjust(rt_uint16_t data);

/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __INTERFACE_CURVE_H__ */
