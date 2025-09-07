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
#ifndef __BLL_DWIN_H__
#define __BLL_DWIN_H__

/*============================ INCLUDES ======================================*/
#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================ MACROS ========================================*/
#define CURVE_SELF_SPEED_INDEX		0
#define CURVE_REAL_ACC_INDEX		1
#define CURVE_ESTI_ACC_INDEX		2

#define CURVE_WINDOW_SELF_SPEED		0
#define CURVE_WINDOW_ACC			1
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ PROTOTYPES ====================================*/
void init_bll_dwin(void);
/*============================ INCLUDES ======================================*/

#ifdef __cplusplus
}
#endif

#endif /* __BLL_DWIN_H__ */
