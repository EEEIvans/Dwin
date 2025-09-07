/**
 * @file bll_dwin.c
 * @brief 业务逻辑 - Dwin数据解析
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
/*============================ INCLUDES ======================================*/
#include <board.h>

#include "bll_dwin.h"
#include "bll_can.h"
#include "interface_can.h"
#include "interface_dwin.h"
#include "interface_curve.h"
#include "dispatcher_can_dwin.h"
#include "dwin_page_var.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"bll_dwin"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
#define DWIN_AUTO_LOAD_DATA_SELF_QUALITY	0x500A
#define DWIN_AUTO_LOAD_DATA_SLOPE			0x500B
#define DWIN_AUTO_LOAD_DATA_SELECT_PAGE		0x500C
#define DWIN_AUTO_LOAD_DATA_CURVE_BUTTON	0x500D
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
//自动上传数据列表
static rt_uint8_t auto_upload_data[] =
{
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
	0x00,
};

static rt_uint16_t lasted_curve_window_id;//上一次曲线窗口id
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/*本车质量分发器*/
static void self_quality_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	rt_uint16_t value;
	
	value = (buff[0] << 8) | buff[1];//buff[0]左移8位变成16位二进制数，再与buff[1]按位与，buff[1]高位补0，进行按位或运算
	//拼凑一个16位二进制数，因为是buff[0]左移所以buff[0]的数据变为小端的16位二进制数的高位，仍然需要SWAP_16函数
	// 滑动刻度取值范围为：0~1000
	// 本车质量取值范围为：0~50000
	value *= 50;//滑动刻度值扩大50倍
	set_dwin_var_value(DWIN_DATA_FRAME_QUALITY_INDEX, SWAP_16(value));//设置本车质量的值

	// 按CAN格式发送数据到上位机
	auto_upload_data[4] = (value >> 8) & 0xFF;//得到数据高位
	auto_upload_data[5] = value & 0xFF;//得到数据低位
	
	can_send(0x301, auto_upload_data, sizeof(auto_upload_data));
}
/*道路坡度分发器*/
static void road_slope_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	rt_int16_t value;
	
	value = (buff[0] << 8) | buff[1];//buff[0]左移8位变成16位二进制数，再与buff[1]按位与，buff[1]高位补0，进行按位或运算
	//拼凑一个16位二进制数，因为是buff[0]左移所以buff[0]的数据变为小端的16位二进制数的高位，仍然需要SWAP_16函数
	// 滑动刻度取值范围为：0~1000
	// 道路坡度取值范围为：-90~90
	value = (value - 500) / 500.0 * 90;//滑动刻度值转换成坡度值
	set_dwin_var_value(DWIN_DATA_FRAME_SLOPE_INDEX, SWAP_16(value));//设置道路坡度的值

	// 按CAN格式发送数据到上位机
	auto_upload_data[6] = value & 0xFF;//只使用一个字节存储是因为坡度为-90~90，一个字节足够
	
	can_send(0x301, auto_upload_data, sizeof(auto_upload_data));
}
/*页面选择分发器*/
static void select_page_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	rt_uint16_t value;

	value = (buff[0] << 8) | buff[1];

	set_current_page_id(value);
	if (value == 1)//没有曲线窗口那个页面
	{
		set_current_curve_window(-1);
	}
	else if (value == 0)//
	{
		set_current_curve_window(lasted_curve_window_id);
	}
}
/*曲线选择分发器*/
static void dwin_cruve_selected(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	rt_uint16_t value;

	value = (buff[0] << 8) | buff[1];
	if (value == 1)
	{
		lasted_curve_window_id = CURVE_WINDOW_SELF_SPEED;
		set_current_curve_window(CURVE_WINDOW_SELF_SPEED);
	}
	else if (value == 2)
	{
		lasted_curve_window_id = CURVE_WINDOW_ACC;
		set_current_curve_window(CURVE_WINDOW_ACC);
	}
}
/*本车车速数值调整*/
static rt_uint16_t self_speed_adjust(rt_uint16_t value)
{
	value *= 10;
	value = SWAP_16(value);
	
	return value;
}
/*实际加速度调整*/
static rt_uint16_t real_acc_adjust(rt_uint16_t value)
{
	rt_int16_t data = (rt_uint16_t) value;
	data = (data + 2000) / 3;
	data = data > 1000 ? 1000 : data;
	data = SWAP_16(data);
	
	return (rt_uint16_t) data;
}
/*估计加速度*/
static rt_uint16_t esti_acc_adjust(rt_uint16_t value)
{
	rt_int16_t data = (rt_uint16_t) value;
	data = (data + 2000) / 3;
	data = data > 1000 ? 1000 : data;
	data = SWAP_16(data);
	
	return (rt_uint16_t) data;
}
/*============================ EXTERNAL IMPLEMENTATION =======================*/
void init_bll_dwin(void)
{
	// dwin分发器系统列表，0x101、0x201、0x301用于分发器处理函数进行比对，这些数字实际上是自己规定的，在can数据发送输入规定的帧id就行
	static dwin_dispatcher_t dwin_dispatcher_pool[] =
	{
		{ DWIN_AUTO_LOAD_DATA_SELF_QUALITY, self_quality_parser},
		{ DWIN_AUTO_LOAD_DATA_SLOPE, 		road_slope_parser},
		{ DWIN_AUTO_LOAD_DATA_SELECT_PAGE, 	select_page_parser},
		{ DWIN_AUTO_LOAD_DATA_CURVE_BUTTON, dwin_cruve_selected},
	};

	init_curve(CURVE_SELF_SPEED_INDEX, 	DWIN_CURVE_CHANNEL1, self_speed_adjust);//初始化本车加速度曲线
	init_curve(CURVE_REAL_ACC_INDEX, 	DWIN_CURVE_CHANNEL1, real_acc_adjust);//初始化实际加速度曲线
	init_curve(CURVE_ESTI_ACC_INDEX, 	DWIN_CURVE_CHANNEL2, esti_acc_adjust);//初始化估计加速度曲线
	
	add_curve_to_window(CURVE_SELF_SPEED_INDEX,	CURVE_WINDOW_SELF_SPEED);//添加本车车速曲线到窗口
	add_curve_to_window(CURVE_REAL_ACC_INDEX,	CURVE_WINDOW_ACC);//添加实际加速度曲线窗口
	add_curve_to_window(CURVE_ESTI_ACC_INDEX,	CURVE_WINDOW_ACC);//添加估计加速度曲线窗口
	
	set_current_curve_window(CURVE_WINDOW_SELF_SPEED);//设置当前曲线窗口，
	
	init_dwin_serial();
	init_dwin_dispatcher(dwin_dispatcher_pool, sizeof(dwin_dispatcher_pool) / sizeof(dwin_dispatcher_t));
}
