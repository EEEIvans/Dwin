/**
 * @file dwin_page_var.h
 * @brief 基于界面的迪文屏变量处理逻辑
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __DWIN_PAGE_VAR_H__
#define __DWIN_PAGE_VAR_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
typedef void (*dwin_page_show_fun)(void);//定义一个函数指针类型
/**
 * @struct one_page_info
 * @brief 当前页面的信息配置
 */
typedef struct one_page_info
{
	rt_uint16_t page_id;			/**< 本界面id */
	rt_uint16_t start_index;		/**< 本界面第一个变量的下标 */
	rt_uint16_t var_address;		/**< 本界面第一个变量的地址（迪文屏地址） */
	rt_uint16_t count;				/**< 本界面显示变量个数 */
	dwin_page_show_fun show_fun;	/**< show_fun是dwin_page_show_fun类型的变量，将指向与其相同参数的函数，这里预设指向的是本界面其它显示处理函数 */
}one_page_info_t;
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
/* 迪文数据显示线程初始化函数 */
void init_dwin_var(rt_uint16_t *var_list, rt_uint16_t var_count, one_page_info_t *page_list, rt_uint16_t page_count);
/* 设置当前曲线窗口id */
void set_current_page_id(rt_uint16_t current_page_id);
/* 获取当前曲线窗口id */
rt_uint16_t get_current_page_id(void);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __DWIN_PAGE_VAR_H__ */
