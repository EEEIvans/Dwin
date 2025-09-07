/**
 * @file interface_curve.c
 * @brief 迪文屏曲线功能
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
/*============================ INCLUDES ======================================*/
#include <board.h>
#include <string.h>

#include "interface_dwin.h"
#include "interface_curve.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"interface_curve"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
#define CLEAN_CURVE_CAHNNEL_INDEX	4	//
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*
	为了能清空曲线，应该将所有“已使用”曲线通道全部记录下来。
	实现思路：在初始化曲线时，需要提供该曲线所有通道，然后记录不重复的曲线通道
*/
/**
 * @brief 清空曲线列表
 * @note 用于清空曲线和曲线是否第一次显示处理
 */
static struct curve_clean_info
{
	rt_int16_t curve_channel;// 曲线通道地址
	rt_bool_t used;			// 通道使用标记
}curve_clean_list[DWIN_CURVE_CHANNEL_MAX_COUNT] =			//8个元素
{
	//曲线通道清理列表初始化，通道0301~030F
	{DWIN_CURVE_CHANNEL1, RT_FALSE},
	{DWIN_CURVE_CHANNEL2, RT_FALSE},
	{DWIN_CURVE_CHANNEL3, RT_FALSE},
	{DWIN_CURVE_CHANNEL4, RT_FALSE},
	{DWIN_CURVE_CHANNEL5, RT_FALSE},
	{DWIN_CURVE_CHANNEL6, RT_FALSE},
	{DWIN_CURVE_CHANNEL7, RT_FALSE},
	{DWIN_CURVE_CHANNEL8, RT_FALSE},
};

/**
 * @brief 清空曲线命令模板
 * @note  用于清空曲线
 */
static rt_uint8_t clean_curve_command[] = 
{
	0x5A,
	0xA5,
	0x05,
	0x82,
	0x03,		// 曲线通道编号
	0x00,
	0x00,		// 数据个数为0，自然清空了曲线通道
	0x00
};

/**
 * @brief 清空曲线数据的相关索引
 * @note  用于清空
 */
#define CURVE_CHANNEL_COUNT_INDEX	8	//通道数索引
#define CURVE_DATA_START_INDEX		10	//曲线数据起始索引

/**
 * @brief 曲线数据帧模板
 * @param DWIN_DATA_FRAME_MAX_LENGTH	255
 * @note  用于准备显示曲线的数据帧
 */
static rt_uint8_t curve_data_frame[DWIN_DATA_FRAME_MAX_LENGTH] =
{
	//帧头+写指令
	0x5A,
	0xA5,
	0x00,		// 后续字节数	DWIN_DATA_BYTE_COUNT_INDEX
	0x82,
	0x03,
	0x10,
	//曲线数据头
	0x5A,
	0xA5,
	0x00,		// 同时写入的曲线通道个数
	0x00,
};

/**
 * @brief 曲线数据帧相关的索引
 * @note  用于准备显示曲线的数据帧
 */
#define CURVE_CHANNEL_ID_INDEX		0	//通道ID索引
#define CURVE_DATA_COUNT_INDEX		1	//数据量索引
#define CURVE_DATA_OFFSET_INDEX		2	//数据偏移

/**
 * @brief 单曲线发送缓冲区
 * @param DWIN_CURVE_DATA_MAX_COUNT	20
 * @note  用于接收曲线数据
 */
static rt_uint8_t one_curve_data_buff[DWIN_CURVE_DATA_MAX_COUNT * 2 + 2] =
{
	0x00,		// 曲线通道编号00~07
	0x00,		// 本次写入的数据个数
};

/**
 * @brief 曲线列表
 * @note  用于存储多条曲线数据
 */
static curve_data_t curve_list[DWIN_CURVE_MAX_COUNT];//曲线列表，32个元素，用于存储曲线数据

/* 曲线窗口配置 */
static struct curve_window
{
	rt_int16_t curve_index_list[DWIN_CURVE_IN_WINDOW_MAX_COUNT];//曲线下标列表，8个元素
	rt_int16_t curve_count;										//曲线数量
	rt_bool_t first_show;										//首次显示标记
}curve_window_list[DWIN_CURVE_WINDOW_MAX_COUNT];//16个元素

static volatile rt_int16_t current_curve_window_index;			//当前窗口索引
static volatile rt_int16_t last_curve_window_index;				//上一次窗口索引
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 获取并预处理曲线数据
 * @param curve_id 曲线ID
 * @param all 是否获取全部数据
 * @return rt_uint16_t 有效数据点数
 */
static rt_uint16_t get_and_adjust_curve_data(rt_uint16_t curve_id, rt_bool_t all)
{
	curve_data_t *curve = &curve_list[curve_id];//获取曲线id地址
	rt_uint16_t curve_data_count = 0;			//曲线数量计数
	rt_uint16_t *curve_data_list;				//获取曲线数据列表地址
	rt_uint16_t index;
	/* curve_data_list是曲线数据列表指针，曲线数据本身，先把把缓冲区的数据强制类型转换成rt_uint16_t，跳过两字节得到指针 */
	curve_data_list = (rt_uint16_t *) (one_curve_data_buff + CURVE_DATA_OFFSET_INDEX);
	/* 从队列获取数据（带互斥锁保护），关闭接收数据 */
	rt_mutex_take(curve->mutex, RT_WAITING_FOREVER);
	if (all == RT_TRUE)//如果需要获取所有数据，则调用相应函数，否则，仅获取最新数据
	{
		curve_data_count = curve_data_queue_get_all_data(&curve->queue, curve_data_list);//获取所有数据
	}
	else 
	{
		curve_data_count = curve_data_queue_get_last_data(&curve->queue, curve_data_list);//获取最新数据
	}
	rt_mutex_release(curve->mutex);// 释放互斥锁
	
	if (curve_data_count == 0)// 如果没有获取到任何数据，则直接返回
	{
		return curve_data_count;
	}
	/* 填充通道ID和数据量，缓冲区填充通道ID */
	one_curve_data_buff[CURVE_CHANNEL_ID_INDEX] = curve->curve_channel & 0xFF;//截留曲线通道低字节，高字节是相同的在数据模板给出了
	one_curve_data_buff[CURVE_DATA_COUNT_INDEX] = curve_data_count & 0xFF;//截留曲线通道低字节，统计数据量
	/* 数据自适应转换 */
	for (index = 0; index < curve_data_count; index++)
	{
		curve_data_list[index] = curve->adjust_fun(curve_data_list[index]);//适应曲线数据，跳过两字节得到指针
	}
	
	return curve_data_count;// 返回获取到的数据数量
}

/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 曲线显示
 * @param curve_window_id 曲线窗口ID，用于指定哪个窗口的曲线数据需要显示
 * @param all 是否显示所有曲线数据的标志，为真时显示所有数据，为假时按一定规则筛选数据
 */
void curve_show(rt_int16_t curve_window_id, rt_bool_t all)
{
	rt_uint16_t show_curve_data_count = 0;// 显示曲线数据数量计数
	rt_uint16_t show_curve_count = 0;// 显示曲线数量计数
	struct curve_window *curve_window;// 定义曲线窗口指针，用于操作曲线窗口结构体
	rt_uint16_t index;
	rt_uint16_t one_curve_data_count = 0;// 单条曲线数据数量计数
	rt_uint16_t curve_data_offset = CURVE_DATA_START_INDEX;// 曲线数据偏移量，用于确定曲线数据在数据帧中的位置
	
	curve_window = &curve_window_list[curve_window_id];// 曲线窗口列表指针
	show_curve_count = curve_window->curve_count;// 获取当前窗口中曲线的数量

	curve_data_frame[CURVE_CHANNEL_COUNT_INDEX] = show_curve_count & 0xFF;//CURVE_CHANNEL_COUNT_INDEX为8，也就是曲线数据帧模板列表第九个元素，这个元素表明曲线通道数量
	/* 遍历窗口内所有曲线 */
	for (index = 0; index < show_curve_count; index++)
	{
		// 获取并调整单条曲线的数据，根据'all'参数决定是否获取所有数据
		one_curve_data_count = get_and_adjust_curve_data(curve_window->curve_index_list[index], all);
		if (one_curve_data_count <= 0)// 如果当前曲线没有数据，则跳过当前循环
		{
			continue;
		}
		// 到这里说明已经取出一条曲线的相关数据，且，已经构成这条曲线的数据
		// 且所形成的曲线数据字节数为：one_curve_data_count * 2 + 2
		// 需要将上述字节内容，复制到curve_data_frame的相关位置。
		// curve_data_frame的相关位置（偏移量）初始值为：CURVE_DATA_START_INDEX
		// 以后，每增加一条curve的数据，偏移量需要加one_curve_data_count * 2 + 2
		rt_memcpy(curve_data_frame + curve_data_offset, one_curve_data_buff, one_curve_data_count * 2 + 2);
		curve_data_offset += one_curve_data_count * 2 + 2;
		
		show_curve_data_count += one_curve_data_count;// 累加显示的曲线数据数量
	}
	
	if (show_curve_data_count <= 0)// 如果没有曲线数据需要显示，则直接返回
	{
		return;
	}
	
	curve_data_frame[DWIN_DATA_BYTE_COUNT_INDEX] = (curve_data_offset - 3) & 0xFF;
	dwin_serial_send(curve_data_frame, curve_data_offset);
	
/*
	要显示的曲线数据个数 = 0;
	要显示的曲线个数 = 窗口中的曲线个数;
	曲线数据缓存;
	
	for (i = 0; i < 曲线个数; i++)
	{
		本曲线数据个数 = 获取并adjust所有/新增曲线数据();
		if (本曲线数据个数 > 0)
		{
			构建、完善并拼凑迪文曲线命令();
			要显示的曲线数据个数 += 本曲线数据个数;
		}
	}
	
	if (要显示的曲线数据个数 > 0)
		发送迪文曲线串口命令();
		
	手工过程
	将“职责单一”的功能，用函数实现
*/
}

/* 
 * @brief 窗口曲线第一次显示
 * @note  返回为真
*/
rt_bool_t is_curve_window_first_show(rt_int16_t curve_window_id)
{
	return curve_window_list[curve_window_id].first_show;
}

/* 
 * @brief 窗口曲线非第一次显示
*/
void set_curve_window_not_first_show(rt_int16_t curve_window_id)
{
	curve_window_list[curve_window_id].first_show = RT_FALSE;
}

/* 
 * @brief 获取当前曲线窗口id
 * @note  返回当前曲线窗口的id索引
*/
rt_int16_t get_current_curve_window(void)
{
	return current_curve_window_index;
}

/* 
 * @brief 设置当前曲线窗口id
 * @param curve_window_id		曲线窗口id
 * @note  比较大的麻烦是切换到没有曲线窗口的页面，曲线窗口id变为-1，再切换到之前的页面，曲线窗口id如何变回来
*/
void set_current_curve_window(rt_int16_t curve_window_id)
{
	if (current_curve_window_index == curve_window_id)//如果切换的曲线窗口与之前相同，直接返回，不需要重新显示
	{
		return;
	}

	if (last_curve_window_index != -1)//这个判断语句表示上一次所处界面有曲线窗口
	//然后执行内部语句，把索引的上一次曲线窗口的first_show标记为RT_TRUE，即标记为第一次显示
	{
		curve_window_list[last_curve_window_index].first_show = RT_TRUE;
	}

	if (current_curve_window_index != -1)// 如果当前页面有曲线窗口，将当前曲线窗口索引值保存为上一次曲线窗口索引值，方便切换页面再切换换回去时，显示切换页面前的曲线窗口
	{
		last_curve_window_index = current_curve_window_index;
	}
	
	current_curve_window_index = curve_window_id;// 当前曲线窗口id交给当前曲线窗口索引，其它用到current_curve_window_index时就显示曲线窗口id值的曲线
}

/* 
 * @brief 添加曲线数据
 * @param curve_id			曲线id
 * @param data				要添加的数据，来源是上位机发送的数据帧，被分发器分发的数据
 * @note  先判断曲线通道是否已满，已满输出日志并断言，若未满，先互斥锁线锁住曲线列表，不再取数据，之后互斥锁放开曲线列表。
 * 		曲线数据的接收来自can侦听线程，发送来自显示线程，使用互斥锁保证线程安全
*/
void add_curve_data(rt_uint16_t curve_id, rt_uint16_t data)
{
	if (curve_id >= DWIN_CURVE_MAX_COUNT)//id大于最大曲线通道数，也就是曲线已满
	{
		LOG_W("curve index (%d) too large!", curve_id);
		RT_ASSERT(0);
	}
	
	rt_mutex_take(curve_list[curve_id].mutex, RT_WAITING_FOREVER);//使用互斥量让曲线列表不再取数据
	curve_data_queue_add_data(&curve_list[curve_id].queue, data);//曲线数据队列添加数据
	rt_mutex_release(curve_list[curve_id].mutex);//放开互斥量
}

/* 
 * @brief 添加曲线到窗口
 * @param curve_id			曲线id
 * @param curve_window_id	曲线窗口id，由获取当前曲线id或设置当前曲线窗口id
 * @note  分别检查curve_id、curve_window_id是否超限，超限输出日志并断言，未超限则记录目标窗口曲线数量，将 curve_id 添加到该窗口的曲线索引列表中
 * 		增加该窗口的曲线计数，表示成功添加一条曲线
*/
void add_curve_to_window(rt_uint16_t curve_id, rt_uint16_t curve_window_id)
{
	rt_uint16_t curve_count;//记录目标窗口曲线数量
	
	if (curve_id >= DWIN_CURVE_MAX_COUNT)//检查 curve_id 是否超出最大允许值，若超出则记录警告并断言失败。
	{
		LOG_W("curve index (%d) too large!", curve_id);
		RT_ASSERT(0);
	}
	
	if (curve_window_id >= DWIN_CURVE_WINDOW_MAX_COUNT)//检查 curve_window_id 是否超出最大允许值，若超出则同样记录警告并断言失败。
	{
		LOG_W("curve window index (%d) too large!", curve_window_id);
		RT_ASSERT(0);
	}
	
	curve_count = curve_window_list[curve_window_id].curve_count;//记录目标窗口曲线数量
	curve_window_list[curve_window_id].curve_index_list[curve_count] = curve_id;//将 curve_id 添加到该窗口的曲线索引列表中
	++curve_window_list[curve_window_id].curve_count;//增加该窗口的曲线计数，表示成功添加一条曲线
}

/* 
 * @brief 清空曲线
 * @param 无
 * @note  遍历曲线清理列表 curve_clean_list，将标记为“已使用”（used == RT_TRUE）的通道信息打包成清理命令 clean_curve_command，
		并通过串口发送出去，每次发送后延时1毫秒，确保串口有足够时间处理数据。 
*/
void clean_curve(void)
{
	int index;
	//遍历所有通道
	for (index = 0; index < DWIN_CURVE_CHANNEL_MAX_COUNT; index++)
	{
		if (curve_clean_list[index].used == RT_TRUE)//若曲线窗口被使用过，则提取通道号并组成曲线清除命令；
		{
			//下面两个赋值语句是把清空曲线通道列表的通道地址的低字节和高字节分别放入清空曲线指令列表的第五、第六元素，
			//由于curve_clean_list列表存储数据的方式是小端存储，所以实际放入第五、第六字节就是03和xx
			clean_curve_command[CLEAN_CURVE_CAHNNEL_INDEX] = curve_clean_list[index].curve_channel & 0xFF;
			clean_curve_command[CLEAN_CURVE_CAHNNEL_INDEX + 1] = (curve_clean_list[index].curve_channel >> 8) & 0xFF;
#if 0			
			{
				int i;
				char str[128];
				char tmp[16];
				
				rt_sprintf(str, "clean command :");
				for (i = 0; i < sizeof(clean_curve_command); i++)
				{
					rt_sprintf(tmp, " %02X", clean_curve_command[i]);
					strcat(str, tmp);
				}
				LOG_I(str);
			}
#endif
			dwin_serial_send(clean_curve_command, sizeof(clean_curve_command));//串口发送清空曲线命令到迪文屏，清空曲线数据
			// 串口数据的发送，实际上是通过rt_thread提供的底层操作完成的。
			// 有可能是通过线程实现的，也有可能是通过中断实现的；
			// 串口数据发送是耗时的，因此，需要适当的延时。
			rt_thread_mdelay(1);
		}
	}
}
/**
 * @brief 初始化曲线配置
 * @param curve_id 曲线的ID，用于标识特定的曲线
 * @param curve_channel 曲线的通道，用于区分不同的数据通道
 * @param adjust_fun 曲线数据调整函数指针，用于后续对曲线数据的调整
 * @note  mutex用于线程安全
 */
void init_curve(rt_uint16_t curve_id, rt_uint16_t curve_channel, curve_data_adjust_t adjust_fun)
{
	rt_uint16_t index;
	
	if (curve_id >= DWIN_CURVE_MAX_COUNT)//检查曲线ID是否超出最大数量限制，如果超出，记录警告日志并断言失败
	{
		LOG_W("curve index (%d) too large!", curve_id);
		RT_ASSERT(0);
	}
	
	if (!IS_DWIN_CURVE_CHANNEL(curve_channel))// 检查曲线通道是否符合预期的值，如果不符合，记录警告日志并断言失败
	{
		LOG_W("error curve channel : %0x4X", curve_channel);
		RT_ASSERT(0);
	}
	
	for (index = 0; index < DWIN_CURVE_CHANNEL_MAX_COUNT; index++)// 遍历曲线通道列表，寻找匹配的曲线通道
	{
		if (curve_clean_list[index].curve_channel == curve_channel)// 如果找到匹配的通道，将其标记为已使用，并跳出循环
		{
			curve_clean_list[index].used = RT_TRUE;
			break;
		}
	}
	
	curve_data_queue_init(&curve_list[curve_id].queue);//曲线数据队列初始化
	curve_list[curve_id].curve_channel = ((curve_channel & 0x0F) - 1) >> 1;// 设置曲线通道，通道号通过曲线通道配置得到
	curve_list[curve_id].mutex = rt_mutex_create("CURVE", RT_IPC_FLAG_PRIO);//创建一个名为"CURVE"的动态互斥量，创建规则直接按RT-Tread例程就行，用于保护曲线数据的完整性
	
	if (adjust_fun == RT_NULL)//若未提供数据调整函数，则使用默认函数。
	{
		adjust_fun = default_curve_data_adjust;
	}
	//将数据调整函数交给曲线下标列表索引的adjust_fun
	curve_list[curve_id].adjust_fun = adjust_fun;
}

/* 
	@ brief	曲线默认数据 

*/
rt_uint16_t default_curve_data_adjust(rt_uint16_t data)
{
	return data;
}
