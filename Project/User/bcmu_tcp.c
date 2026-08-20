#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "lwip/sockets.h"
#include "bcmu_tcp.h"

/* Task priority */
#define LWIP_PROTOCOL_TASK_PRIO     2
/* Max connections */
#define LWIP_MAX_CONNECT            MEMP_NUM_NETCONN
/* Default stack size */
#define LWIP_PROTOCOL_STA_SIZE      1024
#define MAX_DATA_NUM                1400    /* Max single packet data size */

TaskHandle_t ServerSocket_Handler[LWIP_MAX_CONNECT];
uint8_t RecvData_Socket[LWIP_MAX_CONNECT][2000];
uint8_t SendData_Socket[LWIP_MAX_CONNECT][2000];

/* Static stacks and TCBs for per-connection tasks */
static StackType_t ServerSocketTaskStack[LWIP_MAX_CONNECT][LWIP_PROTOCOL_STA_SIZE];
static StaticTask_t ServerSocketTaskTCB[LWIP_MAX_CONNECT];

void sockets_conn_server(void *arg)
{
    int s, ret;
    s = (int)arg;

    struct timeval tv = {5, 0}; /* 5s timeout */
    if(lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))){
        DDL_Printf(PRINT_LEVEL_ERROR, "setsockopt failed: %s\n", strerror(errno));
    }
    if(lwip_setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))){
        DDL_Printf(PRINT_LEVEL_ERROR, "setsockopt failed: %s\n", strerror(errno));
    }

    while (1)
    {
        /* Read full data at once to avoid holding pbufs too long */
        ret = lwip_recv(s, RecvData_Socket[s], MAX_DATA_NUM, 0);
        if (ret > 0)
        {
            /* Echo received data back (link verification only) */
            memcpy(SendData_Socket[s], RecvData_Socket[s], ret);
            ret = lwip_write(s, SendData_Socket[s], ret);
            if(ret == -1)
            {
                int err = errno;
                /* Exit on real errors; continue on EAGAIN/EWOULDBLOCK */
                if (err != EAGAIN && err != EWOULDBLOCK)
                {
                    break;
                }
            }
        }
        else if (ret == 0)
        {
            /* Peer closed connection */
            break;
        }
        else /* ret == -1 */
        {
            int err = errno;
            /* EAGAIN is a timeout, keep waiting; real connection errors exit */
            if (err == ECONNRESET || err == ENOTCONN || err == EAGAIN || err == EBADF || err == EPIPE || err == ETIMEDOUT)
            {
                break;
            }
        }
    }

    /* Drain remaining data in receive buffer to avoid pbuf leak */
    int drain_ret;
    uint8_t drain_buf[128];
    struct timeval drain_tv = {0, 0}; /* non-blocking */
    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &drain_tv, sizeof(drain_tv));

    do {
        drain_ret = lwip_recv(s, drain_buf, sizeof(drain_buf), MSG_DONTWAIT);
    } while (drain_ret > 0);

    /* SO_LINGER = 0: force immediate close so the netconn is released right away */
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    lwip_setsockopt(s, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));

    /* Shutdown read/write directions */
    lwip_shutdown(s, SHUT_RDWR);

    /* Close socket; with SO_LINGER=0 the netconn is released immediately */
    lwip_close(s);
    ServerSocket_Handler[s] = NULL;
    vTaskDelete(NULL);

    for (;;); /* defensive: should never reach here */
}

/* Connection sequence counter, reserved for fd-reuse ABA protection */
static volatile uint32_t conn_seq = 0;

void sockets_accept_server(void *arg)
{
    int ret;
    struct sockaddr_storage aclient;
    socklen_t aclient_len = sizeof(aclient);

    /* Create server socket */
    int slisten = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (slisten < 0) {
        return;
    }

    /* Initialize server sockaddr structure */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(502);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind the server socket */
    if (lwip_bind(slisten, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return;
    }

    /* Start listening for incoming connections */
    if (lwip_listen(slisten, LWIP_MAX_CONNECT) < 0) {
        return;
    }

    while(1)
    {
        ret = lwip_accept(slisten,(struct sockaddr *)&aclient, &aclient_len);

        if(ret > 0)
        {
            /* Bound check: fd must be in [0, LWIP_MAX_CONNECT) */
            if(ret >= LWIP_MAX_CONNECT)
            {
                lwip_close(ret);
                continue;
            }

            {
                /* Poll until the old task really exits: vTaskDelete only marks deletion,
                 * actual TCB cleanup is done by the idle task asynchronously.
                 * Confirm eDeleted via eTaskGetState instead of a fixed delay. */
                TaskHandle_t old_handler = ServerSocket_Handler[ret];
                if (old_handler != NULL) {
                    int wait_ticks = 0;
                    while (eTaskGetState(old_handler) != eDeleted && wait_ticks < 200) {
                        vTaskDelay(pdMS_TO_TICKS(5));
                        wait_ticks++;
                    }
                    if (eTaskGetState(old_handler) != eDeleted) {
                        /* Old task timed out, reject this connection to protect the TCB from concurrent reuse */
                        lwip_close(ret);
                        continue;
                    }
                }
            }

            conn_seq++;
            (void)conn_seq; /* reserved: ABA detection/debug */

            /* Old task TCB confirmed released, safely reuse the static slot */
            ServerSocket_Handler[ret] = xTaskCreateStatic(
                sockets_conn_server,
                "sockets_conn_server",
                LWIP_PROTOCOL_STA_SIZE,
                (void*)ret,
                LWIP_PROTOCOL_TASK_PRIO,
                ServerSocketTaskStack[ret],
                &ServerSocketTaskTCB[ret]
            );

            if(ServerSocket_Handler[ret] == NULL) {
                lwip_close(ret);
            }
        }
    }
}
