/**
 * @file dispatcher_can_dwin.c
 * @brief 代码功能简介
 * @author Lee
 * @version 1.0.0
 * @date 2025-04-15
 *
 * @Copyright (c) 2023, PLKJ Development Team, All rights reserved.
 *
 */
/*============================ INCLUDES ======================================*/
#include <string.h>
#include <board.h>

#include "interface_dwin.h"
#include "dispatcher_can_dwin.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"dispatcher_can_dwin"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/* CAN分发器注册表 */
static struct
{
	can_dispatcher_t *list;	//分发器列表指针
	rt_size_t count;		// 注册的分发器数量
}can_dispatcher_tab;
/* 迪文屏自动加载分发器注册表 */
static struct
{
	dwin_dispatcher_t *list;// 分发器列表指针
	rt_size_t count;		// 注册的分发器数量
}dwin_auto_load_dispatcher_tab;
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 初始化CAN数据分发器系统，得到分发器列表指针，记录分发器数量
 * @param list  分发器配置列表指针
 * @param count 分发器数量
 */
void init_can_dispatcher(can_dispatcher_t *list, rt_size_t count)
{
	
	can_dispatcher_tab.list = list;		//赋值运算符右侧的list来自传入的参数，这条语句相当于什么也没做，但起到了清晰代码的作用
	can_dispatcher_tab.count = count;	//赋值运算符右侧的count来自传入的参数，这条语句相当于什么也没做，但起到了清晰代码的作用
}
/**
 * @brief 初始化迪文屏自动加载分发器系统，得到分发器列表指针，记录分发器数量
 * @param list  分发器配置列表指针
 * @param count 分发器数量
 */
void init_dwin_dispatcher(dwin_dispatcher_t *list, rt_size_t count)
{
	dwin_auto_load_dispatcher_tab.list = list;	//赋值运算符右侧的list来自传入的参数，这条语句相当于什么也没做，但起到了清晰代码的作用
	dwin_auto_load_dispatcher_tab.count = count;//赋值运算符右侧的count来自传入的参数，这条语句相当于什么也没做，但起到了清晰代码的作用
}
/**
 * @brief CAN数据解析路由函数
 * @param id    CAN消息ID,被调用时用的是can侦听线程接收到的数据的id
 * @param buff  数据缓冲区指针，被调用时用的是can侦听线程接收到的数据缓冲区指针
 * @param size  数据长度，被调用时用的是can侦听线程接收到的长度
 */
void can_data_parser(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size)
{
	int index;
	// 遍历所有注册的分发器
	for (index = 0; index < can_dispatcher_tab.count; index++)
	{
		if (can_dispatcher_tab.list[index].id == id)// 逻辑业务层的分发器列表的元素会给出帧id，找到匹配ID，执行对应的处理钩子
		{
			can_dispatcher_tab.list[index].hook(id, buff, size);//hook的参数是can_data_parser传入的参数，也就是can侦听线程接收到的数据的id、*buff、size
																//与hook参数相同的函数是三个分发器函数，所以分发器得到了
			return;
		}
	}
	// 未找到匹配处理器的日志
	LOG_I("CAN data (%04X) parser not found!", id);
}
/**
 * @brief 迪文屏自动加载数据解析路由函数
 * @param address 迪文屏寄存器地址
 * @param buff    数据缓冲区指针
 * @param size    数据长度
 */
void dwin_auto_load_data_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	int index;
	address = SWAP_16(address);// 迪文屏使用大端字节序，需要转换地址
	// 遍历所有注册的分发器
	for (index = 0; index < dwin_auto_load_dispatcher_tab.count; index++)
	{
		if (dwin_auto_load_dispatcher_tab.list[index].address == address)// 逻辑业务层的分发器列表的元素会给出帧address,找到匹配地址，执行处理钩子
		{
			dwin_auto_load_dispatcher_tab.list[index].hook(address, buff, size);//从 can_dispatcher_tab 的结构体数组中选取索引为 index 的元素。
																				//调用该元素中的函数指针hook，传入的参数为 id、*buff 和 size。
			return;
		}
	}
	// 未找到匹配处理器的日志
	LOG_I("Dwin auto load data (%04X) parser not found!", address);
}
/**
 * @brief 默认CAN数据处理函数（调试用）
 * 
 * 以十六进制格式打印接收到的CAN数据
 */
void default_can_data_parser(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size)
{
	char str[512] = {0};// 初始化一个字符数组，用于存储格式化后的CAN数据信息
	char tmp[8];// 临时字符数组，用于存储每个数据字节的格式化字符串
	int i;
	
	rt_sprintf(str, "CAN data (%04X) :", id);// 将CAN数据的ID格式化到字符串中
	for (i = 0; i < size; i++)
	{
		rt_sprintf(tmp, " %02X", buff[i]);// 格式化当前数据字节为两位的十六进制字符串
		strcat(str, tmp);//字符串连接
	}
	
	LOG_I(str);// 打印格式化后的CAN数据信息
}
/**
 * @brief 默认迪文屏数据处理函数（调试用）
 * 
 * 以十六进制格式打印接收到的迪文屏数据
 */
void default_dwin_auto_load_data_parser(rt_uint16_t address, rt_uint8_t *buff, rt_size_t size)
{
	char str[512] = {0};// 初始化一个字符数组，用于存储格式化后的CAN数据信息
	char tmp[8];// 临时字符数组，用于存储每个数据字节的格式化字符串
	int i;
	
	rt_sprintf(str, "Dwin auto load data (%04X) :", address);// 将CAN数据的ID格式化到字符串中
	for (i = 0; i < size * 2; i++)// 注意：迪文屏数据按字处理，也就是两字节，需要x2
	{
		rt_sprintf(tmp, " %02X", buff[i]);// 格式化当前数据字节为两位的十六进制字符串
		strcat(str, tmp);//字符串连接
	}
	
	LOG_I(str);// 打印格式化后的CAN数据信息
}
