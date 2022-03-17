#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <string.h>
#include <websocket/libwsclient.h>
#include <websocket/wsclient_api.h>

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
static const char encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			     "abcdefghijklmnopqrstuvwxyz0123456789+/";

extern int max_data_len;
extern int wsclient_recv_timeout;
extern int wsclient_send_timeout;

/***************Base Framing Protocol for reference*****************/
//
//	0					1					2					3
//	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-------+-+-------------+-------------------------------+
// |F|R|R|R| opcode|M| Payload len |	Extended payload length    |
// |I|S|S|S|  (4)  |A|	   (7)	   |			 (16/64)		   |
// |N|V|V|V|	   |S|			   |   (if payload len==126/127)   |
// | |1|2|3|	   |K|			   |							   |
// +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
// |	 Extended payload length continued, if payload len == 127  |
// + - - - - - - - - - - - - - - - +-------------------------------+
// |							   |Masking-key, if MASK set to 1  |
// +-------------------------------+-------------------------------+
// | Masking-key (continued)	   |		  Payload Data		   |
// +-------------------------------- - - - - - - - - - - - - - - - +
// :					 Payload Data continued ... 			   :
// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |					 Payload Data continued ... 			   |
// +---------------------------------------------------------------+
/*******************************************************************/
static unsigned int ws_arc4random(void)
{
	unsigned int res = xTaskGetTickCount();
	static unsigned int seed = 0xDEADB00B;

	seed = ((seed & 0x007F00FF) << 7) ^
		((seed & 0x0F80FF00) >> 8) ^ // be sure to stir those low bits
		(res << 13) ^ (res >> 9);    // using the clock too!

	return seed;
}

static int ws_b64_encode_string(const char *in, int in_len, char *out, int out_size){
	unsigned char triple[3];
	int i;
	int len;
	int line = 0;
	int done = 0;

	while (in_len) {
		len = 0;
		for (i = 0; i < 3; i++) {
			if (in_len) {
				triple[i] = *in++;
				len++;
				in_len--;
			} else
				triple[i] = 0;
		}

		if (done + 4 >= out_size)
			return -1;

		*out++ = encode[triple[0] >> 2];
		*out++ = encode[((triple[0] & 0x03) << 4) |
					     ((triple[1] & 0xf0) >> 4)];
		*out++ = (len > 1 ? encode[((triple[1] & 0x0f) << 2) |
					     ((triple[2] & 0xc0) >> 6)] : '=');
		*out++ = (len > 2 ? encode[triple[2] & 0x3f] : '=');

		done += 4;
		line += 4;
	}

	if (done + 1 >= out_size)
		return -1;

	*out++ = '\0';

	return done;
}

/***************WebSocket handshake request*****************/
/*	GET /chat HTTP/1.1
/*	Host: server.example.com(:port)
/*	Upgrade: websocket
/*	Connection: Upgrade
/*	Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==
/*	Sec-WebSocket-Protocol: chat, superchat
/*	Sec-WebSocket-Version: 13
/*	Origin: http://example.com
/************************************************************/
static char *ws_handshake_header(char* host, char* path, char*origin, int port, char*extraHeader, int DefaultPort){
	char key_b64[24], hash[16];
	char *header;

	memset(key_b64, 0, 24);
	memset(hash, 0, 16);
	ws_get_random_bytes(hash, 16);
	ws_b64_encode_string(hash, 16, key_b64, 24);//Content of Sec-WebSocket-Key
	
	if(strlen(origin) == 0) {
		if(DefaultPort == 1){
			header = (char *) ws_malloc(strlen("GET /") + strlen(path) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(host) + strlen("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ") 
				+ sizeof(key_b64) + strlen("\r\nSec-WebSocket-Version: 13\r\n")+strlen(extraHeader)+strlen("\r\n")); 

			sprintf(header, "GET /%s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n%s\r\n", 
			path, host, key_b64, extraHeader);
		}
		else{
			header = (char *) ws_malloc(strlen("GET /") + strlen(path) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(host) + strlen(":") + strlen(port) + strlen("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ") 
				+ sizeof(key_b64) + strlen("\r\nSec-WebSocket-Version: 13\r\n")+strlen(extraHeader)+strlen("\r\n")); 

			sprintf(header, "GET /%s HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n%s\r\n", 
			path, host, port, key_b64, extraHeader);
		}
	}
	else{
		if(DefaultPort == 1){
			header = (char *) ws_malloc(strlen("GET /") + strlen(path) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(host) + strlen("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nOrigin: ") + strlen(origin) + 
				strlen("\r\nSec-WebSocket-Key: ") + sizeof(key_b64) + strlen("\r\nSec-WebSocket-Version: 13\r\n")+strlen(extraHeader)+strlen("\r\n")); 

			sprintf(header, "GET /%s HTTP/1.1\r\nHost: %s\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nOrigin: %s\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n%s\r\n", 
			path, host, origin, key_b64, extraHeader);
		}
		else{
			header = (char *) ws_malloc(strlen("GET /") + strlen(path) + strlen(" HTTP/1.1\r\nHost: ") 
				+ strlen(host) + strlen(":") + strlen(port) + strlen("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nOrigin: ") + strlen(origin) + 
				strlen("\r\nSec-WebSocket-Key: ") + sizeof(key_b64) + strlen("\r\nSec-WebSocket-Version: 13\r\n")+strlen(extraHeader)+strlen("\r\n")); 

			sprintf(header, "GET /%s HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nOrigin: %s\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n%s\r\n", 
			path, host, port, origin, key_b64, extraHeader);
		}
	}

	return header;
}

void ws_get_random_bytes(void *buf, size_t len)
{
	unsigned int ranbuf;
	unsigned int *lp;
	int i, count;
	count = len / sizeof(unsigned int);
	lp = (unsigned int *) buf;

	for(i = 0; i < count; i ++) {
		lp[i] = ws_arc4random();  
		len -= sizeof(unsigned int);
	}

	if(len > 0) {
		ranbuf = ws_arc4random();
		memcpy(&lp[i], &ranbuf, len);
	}
}

void* ws_malloc(size_t size){
	return pvPortMalloc(size);
}

void ws_free(void *buf){
	vPortFree(buf);
}

int ws_random(void *p_rng, unsigned char *output, size_t output_len){
	rtw_get_random_bytes(output, output_len);
	return 0;
}

void ws_client_close(wsclient_context *wsclient)
{
	if(wsclient->sockfd != -1) {
		closesocket(wsclient->sockfd);
		wsclient->sockfd = -1;
	}

	wsclient->readyState = CLOSED;
	wsclient->use_ssl = 0;
	wsclient->port = 0;
	wsclient->tx_len = 0;
	wsclient->rx_len = 0;

	if(wsclient->txbuf){
		ws_free(wsclient->txbuf);
		wsclient->txbuf = NULL;
	}		
	if(wsclient->rxbuf){
		ws_free(wsclient->rxbuf);
		wsclient->rxbuf = NULL;
	}
	if(wsclient->receivedData){
		ws_free(wsclient->receivedData);
		wsclient->receivedData = NULL;
	}	
	if(wsclient->tls){
		ws_free(wsclient->tls);
		wsclient->tls = NULL;
	}
	if(wsclient->extraHeader){
		ws_free(wsclient->extraHeader);
		wsclient->extraHeader = NULL;
	}
	memset(wsclient->host, 0, 128);
	memset(wsclient->path, 0, 128);
	memset(wsclient->origin, 0, 128);
/* remove by frankie  20190109 :
     we can not free wsclient here(when dispatching this api in ws_poll,it returns void,upper layer thread continues polling wsclient  readyState,
    it's value will be unstable if memory has been free)
	if(wsclient)
		ws_free(wsclient);
		wsclient = NULL;
	}
*/

}

int ws_client_read(wsclient_context *wsclient, unsigned char *data, size_t data_len){
	int ret = 0;
	ret = recv(wsclient->sockfd, data, data_len, 0);
	int err = 0;
	int err_len = sizeof(err);
	
	if (ret < 0) {
		getsockopt(wsclient->sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len);
	}
	if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
		return 0;
	}
	else if(ret < 0)
		return -1;
	else 
		return ret;
}

int ws_client_send(wsclient_context *wsclient, unsigned char *data, size_t data_len){
	int ret = 0;
	ret = send(wsclient->sockfd, data, data_len, 0);
	int err = 0;
	int err_len = sizeof(err);
	
	if (ret < 0) {
		getsockopt(wsclient->sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len);
	}
	if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
		return 0;
	}
	else if(ret < 0)
		return -1;
	else 
		return ret;
}

void ws_sendData(uint8_t type, size_t message_size, uint8_t* message, int useMask, wsclient_context *wsclient) {

	if (wsclient->readyState == CLOSING || wsclient->readyState == CLOSED){ 
		return; 
	}

	uint8_t masking_key[4];
	uint8_t* header;
	int header_len;

	if(useMask){
		ws_get_random_bytes(masking_key, 4);
		//WSCLIENT_DEBUG("The mask key is %2x, %2x, %2x, %2x\n",masking_key[0], masking_key[1], masking_key[2], masking_key[3]);
	}
	
	header_len = (2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (useMask ? 4 : 0));
	header = malloc(header_len);
	header[0] = 0x80 | type;
	if (message_size < 126) {
		header[1] = (message_size & 0xff) | (useMask ? 0x80 : 0);
		if (useMask) {
			header[2] = masking_key[0];
			header[3] = masking_key[1];
			header[4] = masking_key[2];
			header[5] = masking_key[3];
		}
	}
	else if (message_size < 65536) {
		header[1] = 126 | (useMask ? 0x80 : 0);
		header[2] = (message_size >> 8) & 0xff;
		header[3] = (message_size >> 0) & 0xff;
		if (useMask) {
			header[4] = masking_key[0];
			header[5] = masking_key[1];
			header[6] = masking_key[2];
			header[7] = masking_key[3];
		}
	}
	else { 
		header[1] = 127 | (useMask ? 0x80 : 0);
		header[2] = (message_size >> 56) & 0xff;
		header[3] = (message_size >> 48) & 0xff;
		header[4] = (message_size >> 40) & 0xff;
		header[5] = (message_size >> 32) & 0xff;
		header[6] = (message_size >> 24) & 0xff;
		header[7] = (message_size >> 16) & 0xff;
		header[8] = (message_size >>  8) & 0xff;
		header[9] = (message_size >>  0) & 0xff;
		if (useMask) {
			header[10] = masking_key[0];
			header[11] = masking_key[1];
			header[12] = masking_key[2];
			header[13] = masking_key[3];
		}
	}
	
	memset(wsclient->txbuf, 0, max_data_len + 16);
	memcpy(wsclient->txbuf, header, header_len);
	memcpy(wsclient->txbuf + header_len, message, message_size);
	wsclient->tx_len = header_len + message_size;
	free(header);

	if (useMask) {
		for (size_t i = 0; i != message_size; i++) {
			wsclient->txbuf[header_len + i] ^= masking_key[i&0x3]; 
		}
	}
	return;
}

int ws_hostname_connect(wsclient_context *wsclient) {
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	char sport[5];
	static int opt = 1, option = 1;
	memset(sport, 0, 5);
	sprintf(sport, "%d", wsclient->port);
	if ((wsclient->sockfd = getaddrinfo(wsclient->host, sport, &hints, &result)) != 0){
		WSCLIENT_ERROR("ERROR: Getaddrinfo failed: %s\n", wsclient->sockfd);
		return -1;
	}
	for(p = result; p != NULL; p = p->ai_next){
		wsclient->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (wsclient->sockfd == INVALID_SOCKET) { 
			continue; 
		}
		if((setsockopt(wsclient->sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof(opt))) < 0){
			WSCLIENT_ERROR("ERROR: Setting socket option keepalive failed!\n");
			return -1;
		} 
		if((setsockopt(wsclient->sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option))) < 0){
			WSCLIENT_ERROR("ERROR: Setting socket option SO_REUSEADDR failed!\n");
			return -1;
		} 

		if(wsclient_recv_timeout != 0)
			if (ws_setsockopt_rcvtimeo(wsclient, wsclient_recv_timeout) < 0) {
				WSCLIENT_ERROR("ERROR: set SO_RCVTIMEO fail\n");
				return -1;
			}
		if(wsclient_send_timeout != 0)
			if (ws_setsockopt_sndtimeo(wsclient, wsclient_send_timeout) < 0) {
				WSCLIENT_ERROR("ERROR: set SO_SNDTIMEO fail\n");
				return -1;
			}
		
		if (connect(wsclient->sockfd, p->ai_addr, p->ai_addrlen) != SOCKET_ERROR) {
			break;
		}
		closesocket(wsclient->sockfd);
		wsclient->sockfd = INVALID_SOCKET;
	}
	freeaddrinfo(result);
	return wsclient->sockfd;
}

int ws_client_handshake(wsclient_context *wsclient)
{
	int ret = 0;
	int DefaultPort = 0;

	if(((wsclient->use_ssl == 1) && (wsclient->port == 443))
		|| ((wsclient->use_ssl == 0) && (wsclient->port == 80)))
		DefaultPort = 1;

	char *header = ws_handshake_header(wsclient->host, wsclient->path, wsclient->origin, wsclient->port, wsclient->extraHeader, DefaultPort);
	WSCLIENT_DEBUG("The header is %s\r\n which size is %d\r\n", header, strlen(header));

	ret = wsclient->fun_ops.client_send(wsclient, header, strlen(header));
	ws_free(header);
	return ret;
}

int ws_check_handshake(wsclient_context *wsclient)
{
	int i, status, ret;
	char line[256];
	for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { 
		ret = wsclient->fun_ops.client_read(wsclient, line+i, 1);
		if (ret == 0) 
			return -1; 
	}
	line[i] = 0;
	if (i == 255) { 
		return -1; 
	}
	WSCLIENT_DEBUG("The response's first line is : %s\n", line);
	if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101) {
		WSCLIENT_ERROR("ERROR: Got bad status connecting to %s\n", line); 
		return -1; 
	}
	
	while (1) {
		for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { 
			ret = wsclient->fun_ops.client_read(wsclient, line+i, 1);
			if (ret == 0) 
				return -1; 
		}
		if (line[0] == '\r' && line[1] == '\n') { 
			break; 
		}
	}
	return 0;
}

#ifdef USING_SSL
int wss_hostname_connect(wsclient_context *wsclient){
	int ret;
	static int opt = 1, option = 1;
//ssl connect
	wsclient->sockfd = -1;
	wsclient->tls = (void *)wss_tls_connect(&wsclient->sockfd, wsclient->host, wsclient->port);

	if(wsclient->tls == NULL){
		WSCLIENT_ERROR("ERROR: ssl connect failed ret(%d)\n", ret);
		goto exit;
	}
//set sock
	if((ret = setsockopt(wsclient->sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char *)&opt, sizeof(opt))) < 0){
		WSCLIENT_ERROR("ERROR: setsockopt SO_KEEPALIVE failed ret(%d)\n", ret);
		goto exit;
	}
	if((ret = setsockopt(wsclient->sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&option, sizeof(option))) < 0){
		WSCLIENT_ERROR("ERROR: setsockopt SO_REUSEADDR failed ret(%d)\n", ret);
		goto exit;
	}

	if(wsclient_recv_timeout != 0)
		if (ws_setsockopt_rcvtimeo(wsclient, wsclient_recv_timeout) < 0) {
			WSCLIENT_ERROR("ERROR: set SO_RCVTIMEO fail\n");
			return -1;
		}
	if(wsclient_send_timeout != 0)
		if (ws_setsockopt_sndtimeo(wsclient, wsclient_send_timeout) < 0) {
			WSCLIENT_ERROR("ERROR: set SO_SNDTIMEO fail\n");
			return -1;
		}

//ssl handshake
	if((ret = wss_tls_handshake(wsclient->tls)) !=0 ){
		WSCLIENT_ERROR("ERROR: ssl handshake failed ret(%d)\n", ret);
		goto exit;
	}
	return 0;
exit:
	wss_tls_close(wsclient->tls, &wsclient->sockfd);
	return -1;
}

void wss_client_close(wsclient_context *wsclient)
{
	wss_tls_close(wsclient->tls, &wsclient->sockfd);

	wsclient->readyState = CLOSED;
	wsclient->use_ssl = 0;
	wsclient->port = 0;
	wsclient->tx_len = 0;

	if(wsclient->txbuf){
		ws_free(wsclient->txbuf);
		wsclient->txbuf = NULL;
	}
		
	if(wsclient->rxbuf){
		ws_free(wsclient->rxbuf);
		wsclient->rxbuf = NULL;
	}
	if(wsclient->receivedData){
		ws_free(wsclient->receivedData);
		wsclient->receivedData = NULL;
	}	
	if(wsclient->extraHeader){
		ws_free(wsclient->extraHeader);
		wsclient->extraHeader = NULL;
	}
	memset(wsclient->host, 0, 128);
	memset(wsclient->path, 0, 128);
	memset(wsclient->origin, 0, 128);
/* remove by frankie  20190109 :
     we can not free wsclient here(when dispatching this api in ws_poll,it returns void,upper layer thread continues polling wsclient  readyState,
    it's value will be unstable if memory has been free)
	if(wsclient)
		ws_free(wsclient);
		wsclient = NULL;
	}
*/


}

int wss_client_read(wsclient_context *wsclient, unsigned char *data, size_t data_len){
	int ret = 0;

	ret = wss_tls_read(wsclient->tls, data, data_len);
	int err = 0;
	int err_len = sizeof(err);
	
	if (ret < 0) {
		getsockopt(wsclient->sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len);
	}
	if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
		return 0;
	}
	else if(ret < 0){
		WSCLIENT_DEBUG("ssl_read failed, return: %d", ret);
		return -1;
	}
	else 
		return ret;
}

int wss_client_send(wsclient_context *wsclient, unsigned char *data, size_t data_len){
	int ret = 0;

	ret = wss_tls_write(wsclient->tls, data, data_len);
	int err = 0;
	int err_len = sizeof(err);
	
	if (ret < 0) {
		getsockopt(wsclient->sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len);
	}
	if(ret < 0 && (err == EWOULDBLOCK || err == EAGAIN || err == ENOMEM)) {
		return 0;
	}
	else if(ret < 0){
		WSCLIENT_DEBUG("ssl_write failed, return: %d", ret);
		return -1;
	}
	else 
		return ret;
}
#endif
