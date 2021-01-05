#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <string.h>
#include <websocket/wsclient_api.h>

void (*ws_receive_cb)(wsclient_context *, int) = NULL;

static void ws_dispatchBinary(wsclient_context *wsclient, int rlen) {
	int data_start = 0;//the lenth of data already read
	while (1) {
		struct wsheader_type ws;
		int rxbuf_len;
		rxbuf_len = rlen - data_start;//lenth of data not read
		if (rxbuf_len < 2) { 
			return;
		}
		const uint8_t * data = (uint8_t *) &wsclient->rxbuf[data_start]; // peek, but don't consume
		ws.fin = (data[0] & 0x80) == 0x80;
		ws.opcode = (data[0] & 0x0f);
		ws.mask = (data[1] & 0x80) == 0x80;
		ws.N0 = (data[1] & 0x7f);
		ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 8 : 0) + (ws.mask? 4 : 0);
		if (rxbuf_len < ws.header_size) { 
			return;
		}
		int i = 0;
		if (ws.N0 < 126) {
			ws.N = ws.N0;
			i = 2;
		}
		else if (ws.N0 == 126) {
			ws.N = 0;
			ws.N |= ((uint64_t) data[2]) << 8;
			ws.N |= ((uint64_t) data[3]) << 0;
			i = 4;
		}
		else if (ws.N0 == 127) {
			ws.N = 0;
			ws.N |= ((uint64_t) data[2]) << 56;
			ws.N |= ((uint64_t) data[3]) << 48;
			ws.N |= ((uint64_t) data[4]) << 40;
			ws.N |= ((uint64_t) data[5]) << 32;
			ws.N |= ((uint64_t) data[6]) << 24;
			ws.N |= ((uint64_t) data[7]) << 16;
			ws.N |= ((uint64_t) data[8]) << 8;
			ws.N |= ((uint64_t) data[9]) << 0;
			i = 10;
		}
		if (ws.mask) {
			ws.masking_key[0] = ((uint8_t) data[i+0]) << 0;
			ws.masking_key[1] = ((uint8_t) data[i+1]) << 0;
			ws.masking_key[2] = ((uint8_t) data[i+2]) << 0;
			ws.masking_key[3] = ((uint8_t) data[i+3]) << 0;
		}
		else {
			ws.masking_key[0] = 0;
			ws.masking_key[1] = 0;
			ws.masking_key[2] = 0;
			ws.masking_key[3] = 0;
		}
		if (rxbuf_len < ws.header_size+ws.N) { 
			WSCLIENT_ERROR("ERROR: Didn't get the whole message: length exceeded\n"); 
			return; /* Need: ws.header_size+ws.N - rxbuf.size() */ 
		}

		// We got a whole message, now do something with it:
		if (ws.opcode == TEXT_FRAME || ws.opcode == BINARY_FRAME || ws.opcode == CONTINUATION) 
		{
			if (ws.mask) {
				for (size_t i = 0; i != ws.N; ++i) { 
					wsclient->rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; 
				} 
			}
			memset(wsclient->receivedData, 0, MAX_DATA_LEN);
			memcpy(wsclient->receivedData, (wsclient->rxbuf + data_start + ws.header_size), ws.N);
			
			if (ws.fin != 0) {
				if(ws_receive_cb != NULL)
					ws_receive_cb(wsclient, ws.N);				
			}
		}
		else if (ws.opcode == PING) {
			if (ws.mask) { 
				for (size_t i = 0; i != ws.N; ++i) { 
					wsclient->rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; 
				}
			}
			WSCLIENT_DEBUG("Get PING from server\n");
			uint8_t pong_Frame[6] = {0x8a, 0x80, 0x00, 0x00, 0x00, 0x00};
			int ret = wsclient->fun_ops.client_send(wsclient, pong_Frame, 6);
			if(ret < 0){
				WSCLIENT_ERROR("ERROR: Replay PING failed\n"); 
				wsclient->fun_ops.client_close(wsclient); 
			}
		}
		else if (ws.opcode == PONG) { }
		else if (ws.opcode == CLOSE) { 
			ws_close(wsclient); 
		}
		else { 
			WSCLIENT_ERROR("ERROR: Got unexpected WebSocket message.\n"); 
			wsclient->fun_ops.client_close(wsclient); 
		}
		data_start = data_start + ws.header_size + ws.N;
	}
}

int wss_set_fun_ops(wsclient_context *wsclient){
	struct ws_fun_ops *ws_fun = &wsclient->fun_ops;
	if(wsclient->use_ssl == 1){
		ws_fun->hostname_connect = &wss_hostname_connect;
		ws_fun->client_close = &wss_client_close;
		ws_fun->client_read = &wss_client_read;
		ws_fun->client_send = &wss_client_send;
	}
	else{
		ws_fun->hostname_connect = &ws_hostname_connect;
		ws_fun->client_close = &ws_client_close;
		ws_fun->client_read = &ws_client_read;
		ws_fun->client_send = &ws_client_send;
	}
	return 0;
}

int ws_set_fun_ops(wsclient_context *wsclient){
	struct ws_fun_ops *ws_fun = &wsclient->fun_ops;

	if(wsclient->use_ssl == 0){
		ws_fun->hostname_connect = &ws_hostname_connect;
		ws_fun->client_close = &ws_client_close;
		ws_fun->client_read = &ws_client_read;
		ws_fun->client_send = &ws_client_send;
		return 0;
	}
	else{
		WSCLIENT_ERROR("ERROR: Didn't define the USING_SSL\n"); 
		return -1;
	}
}

void ws_close(wsclient_context *wsclient) {
	if(wsclient->readyState == CLOSING || wsclient->readyState == CLOSED)
		return;
	wsclient->readyState = CLOSING;
	uint8_t pong_Frame[6] = {0x88, 0x80, 0x00, 0x00, 0x00, 0x00};
	wsclient->fun_ops.client_send(wsclient, pong_Frame, 6);
	if (wsclient->readyState == CLOSING)
		wsclient->fun_ops.client_close(wsclient);
	printf("\r\n\r\n\r\n>>>>>>>>>>>>>>>Closing the Connection with websocket server<<<<<<<<<<<<<<<<<<\r\n\r\n\r\n");
}

readyStateValues ws_getReadyState(wsclient_context *wsclient){
	return wsclient->readyState;
}

void ws_dispatch(void (*callback)(wsclient_context *, int)) {
	 ws_receive_cb = callback;	
}

void ws_poll(int timeout, wsclient_context *wsclient) { // timeout in milliseconds
	int total_rlen = 0;
	int ret = 0;

	if (wsclient->readyState == CLOSED) {
		if (timeout > 0) {
			struct timeval tv = { timeout/1000, (timeout%1000) * 1000 };
			select(0, NULL, NULL, NULL, &tv);
		}
		return;
	}
	if (timeout != 0) {
		fd_set rfds;
		fd_set wfds;
		struct timeval tv = { timeout/1000, (timeout%1000) * 1000 };
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(wsclient->sockfd, &rfds);
		if (sizeof(wsclient->txbuf)) { 
			FD_SET(wsclient->sockfd, &wfds); 
		}
		select(wsclient->sockfd + 1, &rfds, &wfds, 0, timeout > 0 ? &tv : 0);
	}

	while (1) {
		ret = wsclient->fun_ops.client_read(wsclient, &wsclient->rxbuf[total_rlen], (MAX_DATA_LEN - total_rlen));
		if(ret == 0)
			break;
		else if(ret < 0){
			wsclient->fun_ops.client_close(wsclient);
			WSCLIENT_ERROR("ERROR: Read data failed!\n");
			break;
		}
		else{		
			WSCLIENT_DEBUG("\r\nreceiving the message: %s, length : %d\r\n", wsclient->rxbuf[total_rlen], ret);
			total_rlen += ret;
		}
	}
	
	ws_dispatchBinary(wsclient, total_rlen);

	if(wsclient->tx_len > 0){
		ret = wsclient->fun_ops.client_send(wsclient, wsclient->txbuf, wsclient->tx_len);
		if (ret == 0) {
		}
		else if(ret < 0){ 
			wsclient->fun_ops.client_close(wsclient);
			WSCLIENT_ERROR("ERROR: Send data faild!\n");
			return;
		}
		else
			WSCLIENT_DEBUG("Send %d bytes data to websocket server\n", ret);

		memset(wsclient->txbuf, 0, MAX_DATA_LEN + 16);
		wsclient->tx_len = 0;
	}
	
	if (wsclient->readyState == CLOSING)
		wsclient->fun_ops.client_close(wsclient);
}

void ws_sendPing(wsclient_context *wsclient) {
	ws_sendData(PING, 0, NULL, 0, wsclient);
}

void ws_sendBinary(uint8_t* message, int message_len, int use_mask, wsclient_context *wsclient) {
	if(message_len > MAX_DATA_LEN){
		WSCLIENT_ERROR("ERROR: The length of data exceeded the MAX_DATA_LEN\n");
		return;
	}
	ws_sendData(BINARY_FRAME, message_len, (uint8_t*)message, use_mask, wsclient);
}

void ws_send(char* message, int message_len, int use_mask, wsclient_context *wsclient) {
	WSCLIENT_DEBUG("Send data: %s\n", message);
	if(message_len > MAX_DATA_LEN){
		WSCLIENT_ERROR("ERROR: The length of data exceeded the MAX_DATA_LEN\n");
		return;
	}
	ws_sendData(TEXT_FRAME, message_len, (uint8_t*)message, use_mask, wsclient);
}

int ws_connect_url(wsclient_context *wsclient) {
	int ret;

	ret = wsclient->fun_ops.hostname_connect(wsclient);
	if (ret == -1) {
		wsclient->fun_ops.client_close(wsclient);
		return -1;
	}
	else{	
		ret = ws_client_handshake(wsclient);
		if(ret <= 0){
			WSCLIENT_ERROR("ERROR: Sending handshake failed\n");
			wsclient->fun_ops.client_close(wsclient);
			return -1;
		}
		else{
			ret = ws_check_handshake(wsclient);
			if(ret == 0)
				WSCLIENT_DEBUG("Connected with websocket server!\n");
			else{
				WSCLIENT_ERROR("ERROR: Response header is wrong\n");
				wsclient->fun_ops.client_close(wsclient);
				return -1;
			}
		}
	}

    int flag = 1;	
    ret = setsockopt(wsclient->sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)); // Disable Nagle's algorithm
    if(ret == 0){
		ret = fcntl(wsclient->sockfd, F_SETFL, O_NONBLOCK);
		if(ret == 0){
			wsclient->readyState = OPEN;			
			printf("\r\n\r\n\r\n>>>>>>>>>>>>>>>Connected to websocket server<<<<<<<<<<<<<<<<<<\r\n\r\n\r\n");
    		return wsclient->sockfd;
    	}
		else{
			wsclient->fun_ops.client_close(wsclient);
			return -1;
		}
	}
	else {
		wsclient->fun_ops.client_close(wsclient);
		WSCLIENT_ERROR("ERROR: Failed to set socket option\n");
		return -1;
	}
}

wsclient_context *create_wsclient(char *url, int port, char *path, char* origin){

	wsclient_context *wsclient = (wsclient_context *)ws_malloc(sizeof(wsclient_context));	
	if(wsclient == NULL){
		WSCLIENT_ERROR("ERROR: Malloc(%d bytes) failed\n", sizeof(wsclient_context));
		ws_free(wsclient);
		return NULL;
	}
	memset(wsclient, 0, sizeof(wsclient_context));

	wsclient->port = port;
	if (strlen(origin) >= 200) {
		WSCLIENT_ERROR("ERROR: Origin size exceeded\n");
		return NULL;
	}
	else if(origin == NULL)
		memset(wsclient->origin, 0, 200);
	else{
		memset(wsclient->origin, 0, 200);
		memcpy(wsclient->origin, origin, strlen(origin));
	}

	if (strlen(url) >= 128) {
		WSCLIENT_ERROR("ERROR: Url size exceeded\n");
		return NULL;
	}
	memset(wsclient->host, 0, 128);
	memset(wsclient->path, 0, 128);
	if(path)
		memcpy(wsclient->path, path, strlen(path));

	if (!strncmp(url, "wss://", strlen("wss://")) || !strncmp(url, "WSS://", strlen("WSS://"))){
		memcpy(wsclient->host, (url + strlen("wss://")), (strlen(url) - strlen("wss://")));
		wsclient->use_ssl = 1;
		if(wsclient->port <= 0)
			wsclient->port = 443;
	}
	else if(!strncmp(url, "ws://", strlen("ws://")) || !strncmp(url, "WS://", strlen("WS://"))){
		memcpy(wsclient->host, (url + strlen("ws://")), (strlen(url) - strlen("ws://")));
		wsclient->use_ssl = 0;
		if(wsclient->port <= 0)
			wsclient->port = 80;
	}
	else {
        WSCLIENT_ERROR("ERROR: Url format is wrong: %s\n", url);
		ws_free(wsclient);
        return NULL;
    }
	
	wsclient->readyState = CLOSED;
	wsclient->sockfd = -1;
	wsclient->tx_len = 0;
	wsclient->ssl = NULL;
	
	wsclient->txbuf = (uint8_t *)ws_malloc(MAX_DATA_LEN + 16);
	wsclient->rxbuf = (uint8_t *)ws_malloc(MAX_DATA_LEN + 16);
	wsclient->receivedData = (uint8_t *)ws_malloc(MAX_DATA_LEN);

	if(!wsclient->txbuf || !wsclient->rxbuf || !wsclient->receivedData)
	{
  		WSCLIENT_ERROR("ERROR: Malloc tx rx buffer memory fail\n");
		ws_client_close(wsclient);
  		return NULL;
  	}
	memset(wsclient->txbuf, 0, MAX_DATA_LEN + 16);
	memset(wsclient->rxbuf, 0, MAX_DATA_LEN + 16);
	memset(wsclient->receivedData, 0, MAX_DATA_LEN);

	if(wsclient_set_fun_ops(wsclient) < 0){
		WSCLIENT_ERROR("ERROR: Init function failed\n");
		ws_client_close(wsclient);
		return NULL;
	}

	return wsclient;
}
