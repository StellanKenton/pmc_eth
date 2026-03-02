#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lwip_comm.h"
#include "LAN8720.h"
#include "usmart.h"
#include "lcd.h"
#include "sram.h"
#include "malloc.h"
#include "lwip/netif.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "lwipopts.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include <string.h>
#include <errno.h>
#include "SEGGER_RTT.h"

//ALIENTEK ̽����STM32F407������
//LWIP LWIP�޲���ϵͳ��ֲ����
//����֧�֣�www.openedv.com
//�������������ӿƼ����޹�˾


//��LCD����ʾ��ַ��Ϣ����
//�������ȼ�
#define DISPLAYT_TASK_PRIO		8
//�����ջ��С	
#define DISPLAY_STK_SIZE 		128  
//������
TaskHandle_t DISPLAY_Task_Handler;
//������
void display_task(void *pvParameters);

//LED����
//�������ȼ�
#define LED_TASK_PRIO		9
//�����ջ��С	
#define LED_STK_SIZE 		64  
//������
TaskHandle_t LED_Task_Handler;
//������
void led_task(void *pvParameters);

//START����
//�������ȼ�
#define START_TASK_PRIO		10
//�����ջ��С	
#define START_STK_SIZE 		128  
//������
TaskHandle_t START_Task_Handler;
//������
void start_task(void *pvParameters);

//TCP Server����
//�������ȼ�
#define TCP_SERVER_TASK_PRIO		6
//�����ջ��С	
#define TCP_SERVER_STK_SIZE 		512  
//������
TaskHandle_t TCP_SERVER_Task_Handler;
//������
void tcp_server_task(void *pvParameters);
//TCP Server�˿ں�
#define TCP_SERVER_PORT		8080

//��LCD����ʾ��ַ��Ϣ
//mode:1 ��ʾDHCP��ȡ���ĵ�ַ
//	  ���� ��ʾ��̬��ַ
void show_address(u8 mode)
{
	u8 buf[30];
	if(mode==2)
	{
		sprintf((char*)buf,"MAC    :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//��ӡMAC��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"DHCP GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,170,210,16,16,buf); 
		sprintf((char*)buf,"DHCP IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,190,210,16,16,buf); 
	}
	else 
	{
		sprintf((char*)buf,"MAC      :%d.%d.%d.%d.%d.%d",lwipdev.mac[0],lwipdev.mac[1],lwipdev.mac[2],lwipdev.mac[3],lwipdev.mac[4],lwipdev.mac[5]);//��ӡMAC��ַ
		LCD_ShowString(30,130,210,16,16,buf); 
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);						//��ӡ��̬IP��ַ
		LCD_ShowString(30,150,210,16,16,buf); 
		sprintf((char*)buf,"Static GW:%d.%d.%d.%d",lwipdev.gateway[0],lwipdev.gateway[1],lwipdev.gateway[2],lwipdev.gateway[3]);	//��ӡ���ص�ַ
		LCD_ShowString(30,170,210,16,16,buf); 
		sprintf((char*)buf,"Static IP:%d.%d.%d.%d",lwipdev.netmask[0],lwipdev.netmask[1],lwipdev.netmask[2],lwipdev.netmask[3]);	//��ӡ���������ַ
		LCD_ShowString(30,190,210,16,16,buf); 
	}	
}

int main(void)
{
	delay_init(168);       								//��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);		//����NVIC�жϷ���4
	
	// Initialize SEGGER RTT (before UART initialization to ensure RTT is available)
	SEGGER_RTT_Init();
	
	uart_init(115200);    								//���ڲ���������
	usmart_dev.init(84); 								//��ʼ��USMART
	LED_Init();  										//LED��ʼ��
	KEY_Init();  										//������ʼ��
	LCD_Init(); 										//LCD��ʼ��
	FSMC_SRAM_Init();									//��ʼ���ⲿSRAM  
	
	my_mem_init(SRAMIN);								//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMEX);								//��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMCCM);	  							//��ʼ��CCM�ڴ��
	
	POINT_COLOR = RED; 		
  
	while(lwip_comm_init()) //lwip��ʼ��
	{
        printf("LWIP initialization failed! Retrying...\r\n");
		delay_ms(1200);
	}
	printf("LWIP initialization successful!\r\n");

	xTaskCreate((TaskFunction_t )start_task,            //������
			  (const char*    )"start_task",          	//��������
			  (uint16_t       )START_STK_SIZE,        	//�����ջ��С
			  (void*          )NULL,                 	//���ݸ��������Ĳ���
			  (UBaseType_t    )START_TASK_PRIO,       	//�������ȼ�
			  (TaskHandle_t*  )&START_Task_Handler);  	//������              
	vTaskStartScheduler();          					//�����������
}

//start����
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();      							//�����ٽ���
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); 							//����DHCP����
#endif
	//����LED����
  xTaskCreate((TaskFunction_t )led_task,              	//������
              (const char*    )"led_task",           	//��������
              (uint16_t       )LED_STK_SIZE,        	//�����ջ��С
              (void*          )NULL,                  	//���ݸ��������Ĳ���
              (UBaseType_t    )LED_TASK_PRIO,       	//�������ȼ�
              (TaskHandle_t*  )&LED_Task_Handler);   	//������ 
	
 	//����DISPLAY����
  xTaskCreate((TaskFunction_t )display_task,          	//������
              (const char*    )"display_task",        	//��������
              (uint16_t       )DISPLAY_STK_SIZE,      	//�����ջ��С
              (void*          )NULL,                  	//���ݸ��������Ĳ���
              (UBaseType_t    )DISPLAYT_TASK_PRIO,    	//�������ȼ�
              (TaskHandle_t*  )&DISPLAY_Task_Handler);	//������ 
  vTaskSuspend(START_Task_Handler);						//����ʼ����
  taskEXIT_CRITICAL();             						//�˳��ٽ���								
}

//��ʾ��ַ����Ϣ
void display_task(void *pvParameters)
{
	static u8 tcp_server_started = 0;  // TCP server是否已启动标志
	
	while(1)
	{ 

#if LWIP_DHCP									        //������DHCP��ʱ��
		if(lwipdev.dhcpstatus != 0) 					//����DHCP
		{
			show_address(lwipdev.dhcpstatus );			//��ʾ��ַ��Ϣ
			// 如果IP获取成功(dhcpstatus==2)且TCP server未启动，则启动TCP server
			if(lwipdev.dhcpstatus == 2 && tcp_server_started == 0)
			{
				taskENTER_CRITICAL();
				xTaskCreate((TaskFunction_t )tcp_server_task,            //������
						  (const char*    )"tcp_server_task",          	//��������
						  (uint16_t       )TCP_SERVER_STK_SIZE,        	//�����ջ��С
						  (void*          )NULL,                 	//���ݸ��������Ĳ���
						  (UBaseType_t    )TCP_SERVER_TASK_PRIO,       	//�������ȼ�
						  (TaskHandle_t*  )&TCP_SERVER_Task_Handler);  	//������
				taskEXIT_CRITICAL();
				tcp_server_started = 1;
				printf("TCP Server task started on port %d\r\n", TCP_SERVER_PORT);
			}
			vTaskSuspend(DISPLAY_Task_Handler); 		//��ʾ���ַ��Ϣ�������������
		}
#else
		show_address(0); 						        //��ʾ��̬��ַ
		// 使用静态IP时，直接启动TCP server
		if(tcp_server_started == 0)
		{
			taskENTER_CRITICAL();
			xTaskCreate((TaskFunction_t )tcp_server_task,            //������
					  (const char*    )"tcp_server_task",          	//��������
					  (uint16_t       )TCP_SERVER_STK_SIZE,        	//�����ջ��С
					  (void*          )NULL,                 	//���ݸ��������Ĳ���
					  (UBaseType_t    )TCP_SERVER_TASK_PRIO,       	//�������ȼ�
					  (TaskHandle_t*  )&TCP_SERVER_Task_Handler);  	//������
			taskEXIT_CRITICAL();
			tcp_server_started = 1;
			printf("TCP Server task started on port %d\r\n", TCP_SERVER_PORT);
		}
		vTaskSuspend(DISPLAY_Task_Handler); 			//��ʾ���ַ��Ϣ�������������
#endif //LWIP_DHCP
		vTaskDelay(500);      							//��ʱ500ms
	}
}

//led����
void led_task(void *pvParameters)
{
	while(1)
	{
		LED0 = !LED0;
		vTaskDelay(500);     							 //��ʱ500ms
 	}
}

//TCP Server����
void tcp_server_task(void *pvParameters)
{
	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len;
	int ret;
	char recv_buf[1024];
	int recv_len;
	
	// 创建TCP socket
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(server_sock < 0)
	{
		printf("TCP Server: Failed to create socket, err=%d\r\n", server_sock);
		vTaskDelete(NULL);
		return;
	}
	
	// 设置socket选项，允许地址重用
	int opt = 1;
	setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	// 设置SO_KEEPALIVE，检测死连接
	opt = 1;
	setsockopt(server_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
	
	// 绑定地址和端口
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
	server_addr.sin_port = htons(TCP_SERVER_PORT);
	
	ret = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret < 0)
	{
		printf("TCP Server: Failed to bind socket, err=%d\r\n", ret);
		close(server_sock);
		vTaskDelete(NULL);
		return;
	}
	
	// 开始监听
	ret = listen(server_sock, 5);  // 最多5个连接在队列中等待
	if(ret < 0)
	{
		printf("TCP Server: Failed to listen, err=%d\r\n", ret);
		close(server_sock);
		vTaskDelete(NULL);
		return;
	}
	
	printf("TCP Server: Listening on port %d\r\n", TCP_SERVER_PORT);
	
	while(1)
	{
		// 等待客户端连接
		client_addr_len = sizeof(client_addr);
		client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
		
		if(client_sock < 0)
		{
			printf("TCP Server: Failed to accept connection, err=%d\r\n", client_sock);
			vTaskDelay(100);  // 等待100ms后重试
			continue;
		}
		
		// 启用TCP Keep-Alive，检测死连接
		opt = 1;
		setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
		
		// 先不设置非阻塞模式，使用阻塞模式但设置超时
		int timeout_ms = 100;  // 100ms超时
		setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));
		setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout_ms, sizeof(timeout_ms));
		
		// 打印客户端连接信息
		char ip_str[16];
		inet_ntoa_r(client_addr.sin_addr, ip_str, sizeof(ip_str));
		printf("TCP Server: Client connected from %s:%d (socket=%d)\r\n", ip_str, ntohs(client_addr.sin_port), client_sock);
		
		// 连接建立后稍微延时，让TCP握手完全完成
		vTaskDelay(50);
		
		// 接收和发送数据循环
		int loop_count = 0;
		while(1)
		{
			// 检查串口是否有数据需要转发
			if(USART_RX_STA & 0x8000)  // 串口接收完成标志
			{
				u16 uart_len = USART_RX_STA & 0x3FFF;  // 获取串口接收的数据长度
				if(uart_len > 0)
				{
					// 通过TCP发送串口数据
					ret = send(client_sock, (char*)USART_RX_BUF, uart_len, 0);
					if(ret < 0)
					{
						printf("TCP Server: Failed to send UART data, err=%d\r\n", ret);
						break;  // 退出内层循环，关闭连接
					}
					else
					{
						printf("TCP Server: Forwarded %d bytes from UART to TCP\r\n", ret);
					}
					
					// 清空串口接收状态
					USART_RX_STA = 0;
				}
			}
			
			// 接收TCP数据（非阻塞）
			recv_len = recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);
			
			if(recv_len > 0)
			{
				// 确保字符串以null结尾
				recv_buf[recv_len] = '\0';
				
				// 在串口打印接收到的数据
				printf("TCP Server: Received %d bytes: ", recv_len);
				// 打印原始数据（十六进制和ASCII）
				for(int i = 0; i < recv_len; i++)
				{
					if(recv_buf[i] >= 32 && recv_buf[i] <= 126)  // 可打印字符
					{
						printf("%c", recv_buf[i]);
					}
					else
					{
						printf("\\x%02X", (unsigned char)recv_buf[i]);
					}
				}
				printf("\r\n");
				
				// 原样回复数据
				ret = send(client_sock, recv_buf, recv_len, 0);
				if(ret < 0)
				{
					printf("TCP Server: send error, err=%d\r\n", ret);
					break;  // 退出内层循环，关闭连接
				}
				else if(ret != recv_len)
				{
					printf("TCP Server: Partial send, sent %d of %d bytes\r\n", ret, recv_len);
				}
				else
				{
					printf("TCP Server: Sent %d bytes back\r\n", ret);
				}
			}
			else if(recv_len == 0)
			{
				// 对端正常关闭连接
				printf("TCP Server: Client closed connection\r\n");
				break;  // 退出内层循环，关闭连接
			}
			else  // recv_len < 0
			{
				// recv超时或暂时没有数据，这是正常的
				// 继续循环等待数据
			}
			
			// 每隔1000次循环打印一次心跳，确认连接正常
			loop_count++;
			if(loop_count % 1000 == 0)
			{
				printf("TCP Server: Connection alive, loop=%d\r\n", loop_count);
			}
			
			// 短暂延时，避免CPU占用过高
			vTaskDelay(10);
		}
		
		// 连接关闭流程：先shutdown再close
		printf("TCP Server: Closing connection...\r\n");
		shutdown(client_sock, SHUT_RDWR);
		vTaskDelay(50);  // 延时让LwIP处理shutdown
		close(client_sock);
		
		// 连接关闭后，额外延时确保LwIP完全清理资源
		printf("TCP Server: Connection closed, waiting for cleanup...\r\n");
		vTaskDelay(200);  // 增加延时到200ms，确保LwIP完全清理TIME_WAIT状态
	}
}
