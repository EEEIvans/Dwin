/**
 * @file util.c
 * @brief 伪链表数据队列工具
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#include "util.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"util"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 初始化队列结构
 * @param queue 队列控制结构指针
 * @note 构建初始循环链表：索引0->1, 1->2, ... 19->0
 */
void curve_data_queue_init(curve_data_queue_t *queue)
{
	rt_uint16_t index;
	
	for (index = 0; index < MAX_COUNT; index++)
	{
		queue->queue[index].next = (index + 1) % MAX_COUNT;//队列元素的next值随着index的增加而增加，到最后一个位置变为0，实现循环
	}
	queue->head = queue->tail = queue->read_point = 0;// 读指针初始与头指针同步
}
/**
 * @brief 添加数据到队列
 * @param queue 队列控制结构指针
 * @param data  待存储的曲线数据
 * @note 当队列满时自动覆盖最旧数据（head前移）
 */void curve_data_queue_add_data(curve_data_queue_t *queue, rt_uint16_t data)
{
	queue->queue[queue->tail].data = data;//数据存入尾部位置
	//queue->tail访问队列尾指针，queue[queue->tail]就是尾节点，queue[queue->tail].data是尾节点的数据，queue->queue[queue->tail].data就是访问尾节点的数据
	queue->tail = queue->queue[queue->tail].next;//尾指针移动到下一位置（自动循环）。queue->queue[queue->tail].next得到尾节点的next指针，把它赋值给尾指针，那么尾指针指向下一个队列元素。
	if (queue->tail == queue->head)//检测队列是否已满（尾指针追上头指针）。前面的代码头指针没有移动，尾指针一直移动，只有尾指针回到0才会与头尾指针，也就是数据队列满了
	{
		queue->head = queue->queue[queue->head].next;//头指针后移：丢弃最旧数据。queue->queue[queue->tail].next得到头节点的next指针，把它赋值给头指针，那么头指针指向下一个队列元素。
	}
}
/**
 * @brief 获取队列所有有效数据
 * @param queue 队列控制结构指针
 * @param buff  输出缓冲区
 * @return index 实际读取的数据个数
 */
rt_uint16_t curve_data_queue_get_all_data(curve_data_queue_t *queue, rt_uint16_t *buff)
{
	rt_uint16_t index = 0;
	rt_uint16_t head;
	
	head = queue->head;//从头指针开始读取。头指针在0的位置，这行代码给头指针赋值为0
	while (head != queue->tail)// 遍历队列直到尾指针。头指针为0，而尾节点不为零，进入while循环内。头指针每次后移最终追上尾指针，跳出循环
	{
		buff[index++] = queue->queue[head].data;//读取队列数据到输出缓冲区，再index自加
		head = queue->queue[head].next;//头指针后移。queue->queue[queue->tail].next得到头节点的next指针，把它赋值给头指针，那么头指针指向下一个队列元素。
	}
	queue->read_point = queue->tail;// 读指针等于尾指针，记录读取位置（后续新增数据从此开始）
	
	return index;//返回实际读取的数据个数。每读一个数据index自加一次，index初值为0，当跳出循环时index的值就是实际读取的数据个数。
}
/**
 * @brief 获取上次读取后的新增数据
 * @param queue 队列控制结构指针
 * @param buff  输出缓冲区
 * @return index 读取新增数据个数
 */rt_uint16_t curve_data_queue_get_last_data(curve_data_queue_t *queue, rt_uint16_t *buff)
{
	rt_uint16_t index = 0;
	rt_uint16_t head;
	
	head = queue->read_point;// 从上次读取位置开始
	while (head != queue->tail)// 遍历新增数据（从read_point到tail）
	{
		buff[index++] = queue->queue[head].data;//读取队列数据到输出缓冲区，再index自加
		head = queue->queue[head].next;//头指针后移
	}
	queue->read_point = queue->tail;// 读指针等于尾指针，读取完毕
	
	return index;//返回实际读取的数据个数
}
