#include "drv_lwip.h"
#include "main.h"
#include "lwip/sockets.h"
#include "bcmu_tcp.h"

struct test_settings {
  struct sockaddr_storage addr;
  int start_client;
  int loop_cnt;
};

TaskHandle_t   ServerSocket_Handler;									
static uint8_t RecvData_Socket1[1500];
static uint8_t SendData_Socket1[1500];
//线程优先级
#define LWIP_PROTOCOL_TASK_PRIO		2
//默认堆栈大小
#define LWIP_PROTOCOL_STA_SIZE		768
#define MAX_DATA_NUM			1400	//最大单包数据



void sockets_conn_server(void *arg)
{
  int s, ret;
	uint32_t tmp32_SendLen = 0;
	
  s = (int)arg;

  while (1) {
    ret = lwip_recv(s,RecvData_Socket1, 1,  0);
    if (ret > 0) 
		{
			ret = lwip_read(s, &RecvData_Socket1[1], MAX_DATA_NUM);
			if(((uint16_t)RecvData_Socket1[4]*256+RecvData_Socket1[5]) == (ret-5))
			{
					tmp32_SendLen = TCP_Protocol(RecvData_Socket1,SendData_Socket1);
					ret = lwip_write(s,SendData_Socket1,tmp32_SendLen);
			}
			
			if(ret == -1)
			{
				break;
			}
    } 
		else 
		{
      if (ret == -1) 
			{
        int err = errno;
        if (err == ECONNRESET) 
				{
          break;
        }
        if (err == ENOTCONN) 
				{
          break;
        }
      }
    }
  }
  ret = lwip_close(s);
	vTaskDelete(ServerSocket_Handler);


}

 void	sockets_accept_server(void *arg)
{
  int slisten;
  portBASE_TYPE xResult;
  int ret;
	struct sockaddr_storage aclient;
	socklen_t aclient_len = sizeof(aclient);
  struct sockaddr_storage addr;
  struct test_settings *settings = (struct test_settings *)mem_malloc(sizeof(struct test_settings));

  memset(settings, 0, sizeof(struct test_settings));
  settings->addr.ss_family = (sa_family_t)0x02;//TCPv4
  ((struct sockaddr_in *)(&settings->addr))->sin_port = PP_HTONS(502);
	//创建SOCKET
  slisten = lwip_socket(AF_INET, SOCK_STREAM, 0);
	
  memcpy(&addr, &settings->addr, sizeof(struct sockaddr_storage));
	//绑定SOCKET到IP和端口
  ret = lwip_bind(slisten, (struct sockaddr *)&addr, sizeof(addr));
	//开启SOCKET监听
  ret = lwip_listen(slisten, 0);
	
	while(1)
	{
		//函数等待连接
		ret = lwip_accept(slisten,(struct sockaddr *)&aclient, &aclient_len);
		
		if(ret>0)//有连接,创建线程
		{
			if(ret > 5)
			{
				lwip_close(ret);//只连接5个 socket
			}
			else if(ret ==1)
			{
				xResult = xTaskCreate( sockets_conn_server,"sockets_conn_server",  LWIP_PROTOCOL_STA_SIZE,(void*)ret, LWIP_PROTOCOL_TASK_PRIO,&ServerSocket_Handler);
			}
			(void)xResult;
		}
	}
}


