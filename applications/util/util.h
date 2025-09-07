/**
 * @file util.h
 * @brief CAN接口功能实现：CAN初始化、CAN数据侦听线程、CAN数据发送
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __UTIL_H__
#define __UTIL_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
#ifndef MAX_COUNT
#define MAX_COUNT	20				//队列最大容量（根据迪文屏数据帧最大长度设定）
#endif
/*============================ TYPES =========================================*/
/**
 * @struct array_link_element
 * @brief 队列元素结构（伪链表节点）
 */
struct array_link_element
{
	rt_uint16_t data;		// 迪文屏曲线数据均为2B
	rt_uint8_t next;		// 曲线数据个数不能超过128个；这与迪文数据帧最大长度有关
};
/**
 * @struct curve_data_queue
 * @brief 曲线数据循环队列控制结构
 */
struct curve_data_queue
{
	struct array_link_element  queue[MAX_COUNT];//数据存储数组
	rt_uint16_t head;		// 队列头指针
	rt_uint16_t tail;		// 队列尾指针
	rt_uint16_t read_point;	// 队列读指针（记录读取的位置）
};

typedef struct curve_data_queue curve_data_queue_t;//curve_data_queue_t结构体类型
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/*
初始化曲线队列；
添加数据；
读取所有数据；
读取新添加的数据；

*/
void curve_data_queue_init(curve_data_queue_t *queue);
void curve_data_queue_add_data(curve_data_queue_t *queue, rt_uint16_t data);
rt_uint16_t curve_data_queue_get_all_data(curve_data_queue_t *queue, rt_uint16_t *buff);
rt_uint16_t curve_data_queue_get_last_data(curve_data_queue_t *queue, rt_uint16_t *buff);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __UTIL_H__ */
