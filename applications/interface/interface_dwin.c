/**
 * @file interface_dwin.c
 * @brief 迪文屏串口输入输出
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
#include "dispatcher_can_dwin.h"

#define DBG_LEVEL	DBG_LOG
#define DBG_TAG		"interface_dwin"
#include <rtdbg.h>

/*============================ MACROS ========================================*/
#define INTERFACE_DWIN_SERIAL_NAME					"uart3"		//串口名称
#define INTERFACE_DWIN_SERIAL_THREAD_STACK_SIZE		1024		//线程堆栈大小
#define INTERFACE_DWIN_SERIAL_THREAD_PRO			20			//优先级
#define INTERFACE_DWIN_SERIAL_THREAD_SEC			20			//所占用时间片段
/*============================ TYPES =========================================*/
/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
/*
	串口设备
	非阻塞侦听线程：用完成量（简化设计、避免忙等待）
*/
static struct 
{
	rt_device_t device;//串口设备
	struct rt_completion cpt;//接收完成量
}interface_dwin_serial;

/*============================ PROTOTYPES ====================================*/
/*============================ INTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 串口接收回调（中断上下文）
 * @param dev 串口设备
 * @param size 接收到的数据长度
 */
static rt_err_t dwin_serial_rx_callback(rt_device_t deveice, rt_size_t size)
{
	rt_completion_done(&interface_dwin_serial.cpt);//释放完成量
	return RT_EOK;
}
/**
 * @brief 迪文屏数据帧收集器
 * @param data_frame 接收到的原始数据指针
 * @param size 数据长度
 * @note 处理数据分片，组装完整协议帧
 */
static void collect_dwin_data_frame(rt_uint8_t *data_frame, rt_uint32_t size)
{
	static rt_uint8_t buffer[DWIN_DATA_FRAME_MAX_LENGTH + 1];//迪文数据最长为255，加1是为了避免出错
	static rt_uint32_t len = 0;//len用于记录传入的数据长度，初始值为0
	
	rt_uint16_t address;//迪文屏数据有固定存放地址，也就是接收到数据的第五、第六字节
	rt_uint8_t data_count;//迪文屏实际的变量数据从第七字节开始
	/* 数据追加 */
	memcpy(buffer + len, data_frame, size);//第一参数是拷贝目的地址，第二参数是拷贝来源地址，第三参数是拷贝字节数
	len += size;//len的值随每一次传入数据而变化
	/* 检查帧完整性 */
	if (len < 3)//当数据小于3字节，直接返回，再进入函数重新接收数据
	{
		return;
	}
	
	if (((rt_uint32_t) buffer[2]) + 3 == len)//迪文数据帧的第三个数据就是后续接收的数据量，可以利用这一点
	{
		// 得到完整数据帧，可以进行下一步处理
		// 将“自动上传数据”交给分发器处理。
#if 0
		{
			// 其实，现在的处理方案是存在问题的。需要先直接输出所接收到的数据，进行观察：
			int i;
			
			rt_kprintf("\nReceive auto upload data from DWIN: %d\n", len);
			for (i = 0; i < len; i++)
			{
				rt_kprintf("%02X ", buffer[i]);
			}
			rt_kprintf("\n");
			// 通过上面的验证发现，上传数据有可能被分割成不同的片段，使得程序无法完整处理所有上传数据。
		}
#endif
		//解析和交给分发器的数据都应该用完整的数据，也就是从buffer缓冲区，而不是data_frame。buffer缓冲区是拼凑好的数据帧
		/* 解析协议帧 */
		address = *((rt_uint16_t *) (buffer + DWIN_DATA_FRAME_ADDRESS_INDEX));//获取buffer缓冲区首地址后DWIN_DATA_FRAME_ADDRESS_INDEX个元素的地址，也就是迪文变量地址
		data_count = *((rt_uint8_t *) (buffer + DWIN_DATA_COUNT_INDEX));//获取buffer缓冲区首地址后DWIN_DATA_INDEX个元素的地址，也就是读字节个数
		/* 交给自动上传数据分发器处理处理函数处理 */
		dwin_auto_load_data_parser(address, buffer + DWIN_DATA_OFFSET_INDEX, data_count);
		len = 0;//len归为0重置缓冲区，进行下一帧处理
	}
}
/**
 * @brief 串口接收处理函数
 * @param arg 未使用
 * @note 持续监听串口数据，进行协议帧组装
 */
static void dwin_serail_rx_dealer(void *arg)
{
	static rt_uint8_t rx_buffer[BSP_UART3_RX_BUFSIZE + 1];//创建用于设置串口处理接收到的数据的缓冲区
	//BSP_UART3_RX_BUFSIZE是缓冲区大小，加1是考虑到如果是字符串需要预留结束位标志
	rt_uint16_t len;//len代表实际发送的数据长度
	while (1)//数据接收处理线程要一直运行，所以用while(1)
	{
		//阻塞等待串口数据接收完成
		rt_completion_wait(&interface_dwin_serial.cpt, RT_WAITING_FOREVER);
		/* 读取串口数据帧 */
		len = rt_device_read(interface_dwin_serial.device, 0, rx_buffer, BSP_UART3_RX_BUFSIZE);
		// 接收迪文屏串口数据，这是“自动上传”类型的数据，需要进一步处理。
		collect_dwin_data_frame(rx_buffer, len);
	}
}

/*============================ EXTERNAL IMPLEMENTATION =======================*/
/**
 * @brief 初始化迪文屏串口
 * @note 1.配置串口参数 2.设置接收回调 3.创建接收线程
 */
void init_dwin_serial(void)
{
	struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;/* RT-Thread提供的默认初始化配置参数,参数包含波特率、数据位、停止位、校验方式等 */
	rt_err_t result;//result作为rt_err_t类型的变量会被多次使用，进行控制设备、打开设备成功与否的判断
	rt_thread_t thread;//串口接收线程变量
	//查找设备
	interface_dwin_serial.device = rt_device_find(INTERFACE_DWIN_SERIAL_NAME);
	RT_ASSERT(interface_dwin_serial.device != RT_NULL);//断言设备非空
	//控制设备
	result = rt_device_control(interface_dwin_serial.device, RT_DEVICE_CTRL_CONFIG, &config);
	RT_ASSERT(result == RT_EOK);//断言控制设备成功
	//打开设备
	result = rt_device_open(interface_dwin_serial.device, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
	RT_ASSERT(result == RT_EOK);//断言打开设备成功
	//完成量初始化
	rt_completion_init(&interface_dwin_serial.cpt);
	//设置接收回调函数
	result = rt_device_set_rx_indicate(interface_dwin_serial.device, dwin_serial_rx_callback);//dwin_serial_rx_callback是回调函数
	RT_ASSERT(result == RT_EOK);//断言接收回调函数成功
	//创建线程
	thread = rt_thread_create("DWIN_RX", dwin_serail_rx_dealer, RT_NULL,//serial_rx_dealer是数据接收处理函数 
	INTERFACE_DWIN_SERIAL_THREAD_STACK_SIZE,//INTERFACE_DWIN_SERIAL_THREAD_STACK_SIZE是缓冲区大小
			INTERFACE_DWIN_SERIAL_THREAD_PRO,//INTERFACE_DWIN_SERIAL_THREAD_PRO是优先级
			INTERFACE_DWIN_SERIAL_THREAD_SEC);//INTERFACE_DWIN_SERIAL_THREAD_SEC是占有时间片段
	RT_ASSERT(thread != RT_NULL);//断言线程创建成功
	//启动线程
	rt_thread_startup(thread);
}
/**
 * @brief 发送数据到迪文屏
 * @param buff 数据缓冲区指针
 * @param size 数据长度
 */
void dwin_serial_send(rt_uint8_t *buff, rt_uint32_t size)
{
	// UART3发送的串口数据，会被迪文屏所接收！
	rt_device_write(interface_dwin_serial.device, 0, buff, size);
}
