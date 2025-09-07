/**
 * @file interface_can.c
 * @brief CAN接口功能实现：CAN初始化、CAN数据侦听线程、CAN数据发送
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

#include "interface_can.h"
#include "dispatcher_can_dwin.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"interface_can"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*
	CAN设备
	非阻塞侦听线程：用完成量（简化设计、避免忙等待）
*/
static struct 
{
	rt_device_t device;//CAN设备
	struct rt_completion cpt;//接收完成量
}interface_can;
/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/**
 * @brief CAN接收回调函数（中断上下文）
 * @param dev 触发中断的设备
 * @param size 接收到的数据长度
 * @return rt_err_t 操作状态码
 * @note 当CAN接收到数据时，由中断调用此函数通知主线程
 */
static rt_err_t can_rx_callback(rt_device_t dev, rt_size_t size)
{
	rt_completion_done(&interface_can.cpt);//释放完成量
	return RT_EOK;
}
/**
 * @brief CAN数据接收处理函数
 * @param parameter 线程参数（未使用）
 * @note 持续监听CAN总线，接收数据并交给分发器处理
 */
static void can_rx_dealer(void *parameter)
{
	static struct rt_can_msg can_receive_msg;//can接收数据消息原型
	while (1)//数据接收处理线程要一直运行，所以用while(1)
	{
		can_receive_msg.hdr_index = -1;//不过滤硬件参数表,也就是要处理所有数据
		rt_completion_wait(&interface_can.cpt, RT_WAITING_FOREVER);//等待can数据接收完成
		rt_device_read(interface_can.device, 0, &can_receive_msg, sizeof(struct rt_can_msg));//读取CAN数据帧 
		// 将CAN数据帧中的数据，交给can数据分发器处理函数处理
		can_data_parser(can_receive_msg.id, can_receive_msg.data, can_receive_msg.len);
#if 0		
		{
			int i;
			// 回环测试
			can_send(can_receive_msg.id, can_receive_msg.data, can_receive_msg.len);
			rt_kprintf("Received CAN data frame:(%04X) ->", can_receive_msg.id);
			for (i = 0; i < can_receive_msg.len; i++)
			{
				rt_kprintf(" %02X", can_receive_msg.data[i]);
			}
		}
#endif
	}
}

/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 初始化CAN总线接口
 * @note 1.查找设备 2.配置波特率 3.设置接收回调 4.创建接收线程
 */
void init_can(void)
{
	rt_err_t res;//res作为rt_err_t类型的变量会被多次使用，进行控制设备、打开设备成功与否的判断
	rt_thread_t thread;//迪文接收线程变量
	//查找CAN设备
	interface_can.device = rt_device_find(INTERFACE_CFG_CAN_NAME);//INTERFACE_CFG_CAN_NAME是设备名称的宏
	if (interface_can.device == RT_NULL)//当查找设备失败时，断言挂起线程
	{
		LOG_E("%s not found!", INTERFACE_CFG_CAN_NAME);
		RT_ASSERT(0);
	}
	//打开CAN设备
	res = rt_device_open(interface_can.device, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);/* 以中断接收及发送方式打开 CAN 设备 */
	if (res != RT_EOK)//当打开CAN设备失败时，断言挂起线程
	{
		LOG_E("CAN open failure!");
		RT_ASSERT(0);
	}
	//控制CAN设备
	res = rt_device_control(interface_can.device, RT_CAN_CMD_SET_BAUD, (void *)CAN500kBaud);/* 设置 CAN 通信的波特率为 500kbit/s*/
	if (res != RT_EOK)//当控制CAN设备失败时，断言挂起线程
	{
		LOG_E("CAN config failure!");
		RT_ASSERT(0);
	}
	//设置异步接收回调的核心函数。设备接收到数据时，通过回调函数主动通知应用程序，实现异步处理机制，回调函数被动响应节省了CPU资源
	rt_device_set_rx_indicate(interface_can.device, can_rx_callback);//can_rx_callback是回调函数
	//完成量初始化
	rt_completion_init(&interface_can.cpt);
	//创建线程
	thread = rt_thread_create("CAN_RX", can_rx_dealer, RT_NULL, //can_rx_dealer是can数据接收处理函数
	INTERFACE_CFG_CAN_THREAD_SIZE,//INTERFACE_CFG_CAN_THREAD_SIZE是接收数据缓冲区大小
			INTERFACE_CFG_CAN_THREAD_PRO,//INTERFACE_CFG_CAN_THREAD_PRO是线程优先级
			INTERFACE_CFG_CAN_THREAD_CPU_SECTION);//INTERFACE_CFG_CAN_THREAD_CPU_SECTION是占用时间片段
	if (thread == RT_NULL)//当创建CAN侦听线程失败时，断言挂起线程
	{
		LOG_E("CAN listener thread failure!");
		RT_ASSERT(0);
	}
	//启动线程
	rt_thread_startup(thread);
}
/**
 * @brief 发送CAN数据帧
 * @param id 消息ID
 * @param buff 数据缓冲区指针
 * @param size 数据长度
 * @return rt_err_t 发送状态（RT_EOK成功，RT_ERROR失败）
 */
rt_err_t can_send(rt_uint32_t id, rt_uint8_t *buff, rt_size_t size)
{
	static struct rt_can_msg can_msg;//rt-thread消息原型
	int i;
	rt_uint16_t len;//len用来得到实际发送数据的长度
	rt_uint16_t length = sizeof(struct rt_can_msg);//lenth用来代替要发送的消息原型所占字节数
	//rt-thread消息原型配置
	can_msg.id = id;
	can_msg.ide = RT_CAN_STDID;     /**< 标准格式 */
	can_msg.rtr = RT_CAN_DTR;       /**< 数据帧 */
	can_msg.len = size;//传入的size长度交给消息原型
	/* 复制数据到消息结构体 */
	for (i = 0; i < size; i++)//用循环把缓冲区数据一个个放到消息原型里，size比较大的时候用memcpy更合适
	{
		can_msg.data[i] = buff[i];
	}
	/* 发送数据，返回值得到发送数据的长度*/
	len = rt_device_write(interface_can.device, 0, &can_msg, length);
	/* 检查发送完整性 */
	if (len != length)//如果实际发送数据与消息原型的长度不相同，打印错误信息
	{
		LOG_I("can data %d send failure (%d / %d)!", id, len, length);
		return RT_ERROR;
	}
	
	return RT_EOK;
}
