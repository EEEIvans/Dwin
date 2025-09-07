/**
 * @file dwin_page_var.c
 * @brief 基于界面的迪文屏变量处理逻辑
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
/*============================ INCLUDES ======================================*/
#include <board.h>

#include "interface_dwin.h"
#include "interface_curve.h"
#include "dwin_page_var.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"dwin_page_var"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
#define DWIN_VAR_SHOW_THREAD_STACK_SIZE		1024	//线程栈大小
#define DWIN_VAR_SHOW_THREAD_PRO			20		//线程优先级
#define DWIN_VAR_SHOW_THREAD_SECTION		20		//线程时间片
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/**
 * @struct dwin_var_info
 * @brief 把迪文屏变量组合成一个数组并统计变量个数，迪文屏所有页面也组合成一个数组并统计界面个数
 */
typedef struct dwin_var_info
{
	rt_uint16_t *var_list;			/**< 所有迪文屏显示变量数据指针 */
	rt_uint16_t var_count;			/**< 变量个数 */
	
	one_page_info_t *page_list;		/**< 当前迪文屏界面配置列表的指针 */
	rt_uint16_t page_count;			/**< 界面个数 */
}dwin_var_info_t;

static dwin_var_info_t dwin_var;//定义dwin_var_info_t结构体类型的变量dwin_var
static volatile rt_uint16_t page_id;	/**< 正在显示的界面id，默认值为0 */
// 迪文屏命令帧模板（写操作）
static rt_uint8_t dwin_command_buff[DWIN_DATA_FRAME_MAX_LENGTH] =
{
	0x5A, 0xA5,			/**< 迪文数据头部 */
	0x00,				/**< 字节个数 */
	DWIN_COMMAND_WRITE,	/**< 写指令 */
	0x00, 0x00,			/**< 变量地址 */
};
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 准备正在显示的页面的迪文屏数据帧
 * @param one_page 当前页面配置
 * @return rt_int16_t 生成的数据帧长度
 * @note 函数填好迪文屏写命令帧模板并添加数据；返回值是数据帧长度，用于串口发送
 */
static rt_int16_t prepared_dwin_data_frame(const one_page_info_t *one_page)
{
	dwin_data_frame_format_t *dwin_data_frame_format;//定义一个dwin_data_frame_format_t结构体类型的指针变量dwin_data_frame_format
	
	dwin_data_frame_format = (dwin_data_frame_format_t *) dwin_command_buff;//将dwin_command_buff列表转换成指针，dwin_data_frame_format指向列表首元素
	dwin_data_frame_format->command = DWIN_COMMAND_WRITE;//配置写命令
	dwin_data_frame_format->var_address = SWAP_16(one_page->var_address);//转换迪文数据存放地址的字节顺序。迪文数据采用大端存储模式，而C语言采用小端存储模式，所以需要转换字节顺序
	dwin_data_frame_format->byte_count = (rt_uint8_t) (one_page->count * 2 + 3);//计算数据长度。因为count记录的是迪文变量个数，而迪文数据是两字节的，byte_count是迪文数据帧统计包括写指令在内的所有字节个数，所以byte_count是count的2倍加3
	// 拷贝变量数据到发送缓冲区
	rt_memcpy((rt_uint8_t *) (dwin_command_buff + DWIN_WRITE_DATA_OFFSET), 
		(rt_uint8_t *) (dwin_var.var_list + one_page->start_index), one_page->count * 2);//拷贝数据目的地址是dwin_command_buff后DWIN_WRITE_DATA_OFFSET个字节开始，即dwin_command_buff+6
																						//源地址是var_list列表后start_index，也就是后第一个变量下标值，即6，即源地址从var_list列表的第七个字节开始拷贝
																						//迪文屏变量占2字节，写入的字节数是迪文变量总数的2倍，所以count要乘2
	return one_page->count * 2 + 6;//返回数据帧长度，也就是数据帧所有字节个数
}
/**
 * @brief 显示当前曲线窗口的曲线
 */
static void show_current_curve_window(void)
{
	rt_int16_t current_curve_window_id;
	
	current_curve_window_id = get_current_curve_window();//从get_current_curve_window的返回值得到当前曲线窗口的id值
	if (current_curve_window_id == -1)//无活动曲线窗口
	{
		return;
	}
	
	if (is_curve_window_first_show(current_curve_window_id))
	{
		//首次显示：清除旧数据并显示全部数据;标记不是第一次显示
		clean_curve();
		curve_show(current_curve_window_id, RT_TRUE);
		set_curve_window_not_first_show(current_curve_window_id);
	}
	else
	{
		//非首次显示：显示新数据
		curve_show(current_curve_window_id, RT_FALSE);
	}
}
/**
 * @brief 迪文变量显示线程处理函数
 * 
 * 定期刷新当前活动页面的显示内容
 */
static void dwin_var_show_dealer(void *arg)
{
	int i;
	int len;

	while (1)
	{
		for (i = 0; i < dwin_var.page_count; i++)//遍历所有页面
		{
			if (page_id == dwin_var.page_list[i].page_id)//如果迪文页面配置结构体的页面列表的页面id值等于当前页面id
			
				// 1. 准备并发送变量数据帧
				len = prepared_dwin_data_frame(&dwin_var.page_list[i]);//&dwin_var.page_list由init_dwin_var传入的参数赋值得到，所以prepared_dwin_data_frame函数的返回值就是数据帧长度
				dwin_serial_send(dwin_command_buff, len);//串口发送迪文命令帧,第一参数就是数据帧缓冲区，第二参数是数据帧长度
				// 2. 执行页面特有的显示逻辑，本项目没有体现
				dwin_var.page_list[i].show_fun();
				
				// 显示当前曲线窗口
				show_current_curve_window();
			}
		}
	}
}
/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 初始化迪文屏变量的页面显示配置
 * @note 所有的参数会在业务逻辑层bll.c得到，第一参数dwin_var_list就是数据帧列表的指针，第三参数dwin_pages就是页面列表的指针，第二、第四参数可以通过第一、第三参数计算得到
 */
void init_dwin_var(rt_uint16_t *var_list, rt_uint16_t var_count, one_page_info_t *page_list, rt_uint16_t page_count)
{
	rt_thread_t thread;
	//dwin_var就是dwin_var_info_t结构体类型的变量dwin_var
	dwin_var.var_list = var_list;//传入的var_list来自
	//赋值符右边的var_list是传入的参数，在业务逻辑层得到分发器分发的数据，左边的var_list在dwin_var结构体内，也就是把传入的var_list保存到dwin_var结构体中
	dwin_var.var_count = var_count;//传入的var_count保存到dwin_var结构体中
	
	dwin_var.page_list = page_list;//传入的page_list保存到dwin_var结构体中
	dwin_var.page_count = page_count;//传入的page_count保存到dwin_var结构体中
	//创建页面显示线程
	thread = rt_thread_create("DWIN_SHOW", dwin_var_show_dealer, RT_NULL,
			DWIN_VAR_SHOW_THREAD_STACK_SIZE,
			DWIN_VAR_SHOW_THREAD_PRO + 1,//优先级加1让迪文屏显示的优先级低于迪文接收线程、CAN接收线程。这样显示才不会出问题
			DWIN_VAR_SHOW_THREAD_SECTION);
	if (thread == RT_NULL)//如果线程为空，创建线程失败，输出日志并断言
	{
		LOG_E("DWIN SHOW thread failure!");
		RT_ASSERT(0);
	}
	//启动线程
	rt_thread_startup(thread);
}
/**
 * @brief 设置当前活动页面ID
 * @param current_page_id 要设置的页面ID
 * @note 在bll_dwin.c中会通过按键返回变量的值得到current_page_id
 */
void set_current_page_id(rt_uint16_t current_page_id)
{
	page_id = current_page_id;
}
/**
 * @brief 获取当前活动页面ID
 * @return 当前页面ID
 */
rt_uint16_t get_current_page_id(void)
{
	return page_id;//返回默认值0
}
