/**
 * @file bll_can.h
 * @brief 业务逻辑 - CAN数据解析
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
#ifndef __BLL_CAN_H__
#define __BLL_CAN_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
#define DWIN_DATA_FRAME_RUN_PROGRESS_INDEX	0       //运行进程数据在列表的索引值
#define DWIN_DATA_FRAME_SELF_ACC_INDEX		1       //本车加速度在列表的索引值
#define DWIN_DATA_FRAME_STEERING_INDEX		2       //方向盘转角在列表的索引值
#define DWIN_DATA_FRAME_YAW_INDEX			3       //横摆角速度在列表的索引值
#define DWIN_DATA_FRAME_TORQUE_INDEX		4       //发动机扭矩在列表的索引值
#define DWIN_DATA_FRAME_SELF_SPEED_INDEX	5       //本车车速在列表的索引值
#define DWIN_DATA_FRAME_QUALITY_INDEX		6       //本车质量在列表的索引值
#define DWIN_DATA_FRAME_SLOPE_INDEX			7       //道路坡度在列表的索引值
#define DWIN_DATA_FRAME_LIGHT_INDEX			8       //指示灯状态在列表的索引值
#define DWIN_DATA_FRAME_GEAR_INDEX			9       //挡位状态在列表的索引值

// 迪文屏变量地址定义
#define DWIN_DATA_RUN_PROGRESS_ADDRESS		0x5000  //运行进程变量地址
#define DWIN_DATA_SELF_ACC_ADDRESS			0x5001  //本车加速度变量地址
#define DWIN_DATA_STEERING_ADDRESS			0x5002  //方向盘转角变量地址
#define DWIN_DATA_YAW_ADDRESS				0x5003  //横摆角速度变量地址
#define DWIN_DATA_TORQUE_ADDRESS			0x5004  //发动机扭矩变量地址
#define DWIN_DATA_SELF_SPEED_ADDRESS		0x5005  //本车车速变量地址
#define DWIN_DATA_QUALITY_ADDRESS			0x5006  //本车质量变量地址
#define DWIN_DATA_SLOPE_ADDRESS				0x5007  //道路坡度变量地址
#define DWIN_DATA_LIGHT_ADDRESS				0x5008  //指示灯变量地址
#define DWIN_DATA_GEAR_ADDRESS				0x5009  //挡位状态变量地址
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
void init_bll_can(void);

void set_dwin_var_value(rt_uint16_t var_index, rt_uint16_t value);
rt_uint16_t get_dwin_var_value(rt_uint16_t var_index);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __BLL_CAN_H__ */
