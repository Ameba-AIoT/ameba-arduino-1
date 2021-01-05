#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <platform/platform_stdlib.h>

#define PING_IP		"192.168.0.1"
#define PING_TO		1000
#define PING_ID		0xABCD
#define BUF_SIZE	200
#define STACKSIZE	1024

static unsigned short ping_seq = 0;
static int infinite_loop, ping_count, data_size, ping_interval, ping_call;
static char ping_ip[16];

static void generate_ping_echo(unsigned char *buf, int size)
{
	int i;
	struct icmp_echo_hdr *pecho;

	for(i = 0; i < size; i ++) {
		buf[sizeof(struct icmp_echo_hdr) + i] = (unsigned char) i;
	}

	pecho = (struct icmp_echo_hdr *) buf;
	ICMPH_TYPE_SET(pecho, ICMP_ECHO);
	ICMPH_CODE_SET(pecho, 0);
	pecho->chksum = 0;
	pecho->id = PING_ID;
	pecho->seqno = htons(++ ping_seq);

	//Checksum includes icmp header and data. Need to calculate after fill up icmp header
	pecho->chksum = inet_chksum(pecho, sizeof(struct icmp_echo_hdr) + size);
}

void ping_test(void *param)
{
	int i, ping_socket;
	int pint_timeout = PING_TO;
	struct sockaddr_in to_addr, from_addr;
	int from_addr_len = sizeof(struct sockaddr_in);
	int ping_size, reply_size;
	unsigned char ping_buf[BUF_SIZE], reply_buf[BUF_SIZE];
	unsigned int ping_time, reply_time, total_time = 0, received_count = 0;
	struct ip_hdr *iphdr;
	struct icmp_echo_hdr *pecho;

	//Ping size = icmp header(8 bytes) + data size
	ping_size = sizeof(struct icmp_echo_hdr) + data_size;
	printf("\n\r[%s] PING %s %d(%d) bytes of data", __FUNCTION__, ping_ip, data_size, sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr) + data_size);

	for(i = 0; (i < ping_count) || (infinite_loop == 1); i ++) {
		ping_socket = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
		setsockopt(ping_socket, SOL_SOCKET, SO_RCVTIMEO, &pint_timeout, sizeof(pint_timeout));

		to_addr.sin_len = sizeof(to_addr);
		to_addr.sin_family = AF_INET;
		to_addr.sin_addr.s_addr = inet_addr(ping_ip);

		generate_ping_echo(ping_buf, data_size);
		sendto(ping_socket, ping_buf, ping_size, 0, (struct sockaddr *) &to_addr, sizeof(to_addr));
		
		ping_time = xTaskGetTickCount();
		if((reply_size = recvfrom(ping_socket, reply_buf, sizeof(reply_buf), 0, (struct sockaddr *) &from_addr, (socklen_t *) &from_addr_len))
			>= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {

			reply_time = xTaskGetTickCount();
			iphdr = (struct ip_hdr *)reply_buf;
			pecho = (struct icmp_echo_hdr *)(reply_buf + (IPH_HL(iphdr) * 4));

			if((pecho->id == PING_ID) && (pecho->seqno == htons(ping_seq))) {
				printf("\n\r[%s] %d bytes from %s: icmp_seq=%d time=%d ms", __FUNCTION__, reply_size - sizeof(struct ip_hdr), inet_ntoa(from_addr.sin_addr), htons(pecho->seqno), (reply_time - ping_time) * portTICK_RATE_MS);
				received_count++;
				total_time += (reply_time - ping_time) * portTICK_RATE_MS;
			}
		}
		else
			printf("\n\r[%s] Request timeout for icmp_seq %d", __FUNCTION__, ping_seq);

		close(ping_socket);
		vTaskDelay(ping_interval * configTICK_RATE_HZ);
	}
	printf("\n\r[%s] %d packets transmitted, %d received, %d%% packet loss, average %d ms", __FUNCTION__, ping_count, received_count, (ping_count-received_count)*100/ping_count, total_time/received_count);
	if(!ping_call)
		vTaskDelete(NULL);
}

void do_ping_call(char *ip, int loop, int count)
{
	ping_call = 1;
	ping_seq = 0;
	data_size = 120;
	ping_interval = 1;
	infinite_loop = loop;
	ping_count = count;
	strcpy(ping_ip, ip);

	ping_test(NULL);
}

void do_ping_test(char *ip, int size, int count, int interval)
{
	if((sizeof(struct icmp_echo_hdr) + size) > BUF_SIZE) {
		printf("\n\r%s BUF_SIZE(%d) is too small", __FUNCTION__, BUF_SIZE);
		return;
	}

	if(ip == NULL)
		strcpy(ping_ip, PING_IP);
	else
		strcpy(ping_ip, ip);

	ping_call = 0;
	ping_seq = 0;
	data_size = size;
	ping_interval = interval;

	if(count == 0) {
		infinite_loop = 1;
		ping_count = 0;
	}
	else {
		infinite_loop = 0;
		ping_count = count;
	}

	if(xTaskCreate(ping_test, ((char const*)"ping_test"), STACKSIZE, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
