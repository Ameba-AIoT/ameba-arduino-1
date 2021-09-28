#include <string.h>
#include <stdio.h>
#include <polarssl/net.h>
#include <polarssl/ssl.h>
#include <polarssl/error.h>
#include <polarssl/memory.h>

#include "google_nest.h"

#define PORTNUMBER        443
#define DEBUG_GOOGLENEST    1
#define RESPONSE_SIZE       256


#define GOOGLENEST_LOG(level, fmt, ...) printf("\n\r[GOOGLENEST %s] %s: " fmt "\n", level, __FUNCTION__, ##__VA_ARGS__)

#if DEBUG_GOOGLENEST == 2
#define GOOGLENEST_DEBUG(fmt, ...) GOOGLENEST_LOG("DEBUG", fmt, ##__VA_ARGS__)
#else
#define GOOGLENEST_DEBUG(fmt, ...)
#endif
#if DEBUG_GOOGLENEST
#define GOOGLENEST_ERROR(fmt, ...) GOOGLENEST_LOG("ERROR", fmt, ##__VA_ARGS__)
#else
#define GOOGLENEST_ERROR(fmt, ...)
#endif

typedef struct {
    uint32_t status_code;
	uint32_t data_start;
    unsigned char  url[128];
    unsigned char  uri[128];
} gn_response_result_t;


static void (*data_retrieve_cb)(char *) = NULL;
static int gn_stream_get(googlenest_context *googlenest, gn_response_result_t gn_rsp_result);
static int gn_stream_start(googlenest_context *googlenest, gn_response_result_t gn_rsp_result);
static void* gn_malloc(unsigned int size){
	return (void *) rtw_malloc(size);
}

void gn_free(void *buf){
	rtw_mfree(buf);
}

static int parse_https_response(uint8_t *response, uint32_t response_len, gn_response_result_t *result) {
    uint32_t i, p, q, m;
    uint8_t status[4] = {0};
    uint32_t content_length = 0;
    int ret = 0;
	const uint8_t *location_buf1 = "LOCATION";
    const uint8_t *location_buf2 = "Location";
    const uint32_t location_buf_len = strlen(location_buf1);

    // status code
    i = p = q = m = 0;
    for (; i < response_len; ++i) {
        if (' ' == response[i]) {
            ++m;
            if (1 == m) {//after HTTP/1.1
                p = i;
            } 
			else if (2 == m) {//after status code
                q = i;
                break;
            }
        }
    }
    if (!p || !q || q-p != 4) {//HTTP/1.1 200 OK
        return 0;
    }
	
    memcpy(status, response+p+1, 3);//get the status code
	result->status_code = atoi((char const *)status);
	GOOGLENEST_DEBUG("The status code is %d", result->status_code);

	if (result->status_code == 307){
	    //Relocation
	    p = q = 0;
	    for (i = 0; i < response_len; ++i) {
	        if (response[i] == '\r' && response[i+1] == '\n') {
	            q = i;//the end of the line
	            if (!memcmp(response+p, location_buf1, location_buf_len) ||
	                    !memcmp(response+p, location_buf2, location_buf_len)) {//the line of Location
	                int j1 = p+location_buf_len, j2 = q-1;
	                while ( j1 < q && (*(response+j1) == ':' || *(response+j1) == ' ') ) ++j1;
	                while ( j2 > j1 && *(response+j2) == ' ') --j2;
					int http_len = strlen("https://");
					if(!memcmp(response+j1, "https://", http_len)){
						for (i = j1 + http_len; i < j2; ++i) {
							if ('/' == response[i]) {
								p = i;
								break;
							}
						}
						//result->url = (unsigned char *)gn_malloc(p-j1-http_len);						
						//result->uri = (unsigned char *)gn_malloc(j2-p);
						memset(result->url, 0, strlen(result->url));
						memset(result->uri, 0, strlen(result->uri));
						memcpy(result->url, response+j1+http_len, p-j1-http_len);
						memcpy(result->uri, response+p+1, j2-p);
						GOOGLENEST_DEBUG("The url is %s, the uri is %s", result->url, result->uri);
						ret = 1;
						break;
					}
	            }
	            p = i+2;
	        }
	        if (response[i] == '\r' && response[i+1] == '\n' &&
	                response[i+2] == '\r' && response[i+3] == '\n') {//the end of header
	            p = i+4;//p is the start of the body
	            break;
	        }
	    }
    }
	else if(result->status_code == 200){
		ret = 1;
		for (i = 0; i < response_len; ++i) {
	        if (response[i] == '\r' && response[i+1] == '\n' &&
	                response[i+2] == '\r' && response[i+3] == '\n') {//the end of header
	            p = i+4;//p is the start of the body
	            result->data_start = p;
	            break;
	        }
	    }
	}
	else if(result->status_code == 404){
		GOOGLENEST_DEBUG("\n Not Found: A request made over HTTP instead of HTTPS\n");
		ret = -2;
	}
	else if(result->status_code == 400){
		GOOGLENEST_DEBUG("\n Bad Request: 1. Unable to parse PUT or POST data\n2. Missing PUT or POST data\n3.Attempting to PUT or POST data which is too large\n4.A REST API call that contains invalid child names as part of the path");
		ret = -3;
	}
	else if(result->status_code == 403){
		GOOGLENEST_DEBUG("\n Forbidden: A request that violates your Security and Firebase Rules\n");
		ret = -4;
	}
	else if(result->status_code == 417){
		GOOGLENEST_DEBUG("\n Expectation Failed: A REST API call that doesn't specify a namespace\n");
		ret = -5;
	}
	else{
		GOOGLENEST_DEBUG("\n The request is failed!\n");
		ret = -1;
	}	
    return ret;
}

static unsigned int arc4random(void){
	unsigned int res = xTaskGetTickCount();
	static unsigned int seed = 0xDEADB00B;

	seed = ((seed & 0x007F00FF) << 7) ^
		((seed & 0x0F80FF00) >> 8) ^ // be sure to stir those low bits
		(res << 13) ^ (res >> 9);    // using the clock too!

	return seed;
}

static void get_random_bytes(void *buf, size_t len){
	unsigned int ranbuf;
	unsigned int *lp;
	int i, count;
	count = len / sizeof(unsigned int);
	lp = (unsigned int *) buf;

	for(i = 0; i < count; i ++) {
		lp[i] = arc4random();  
		len -= sizeof(unsigned int);
	}

	if(len > 0) {
		ranbuf = arc4random();
		memcpy(&lp[i], &ranbuf, len);
	}
}

static int my_random(void *p_rng, unsigned char *output, size_t output_len){
	get_random_bytes(output, output_len);
	return 0;
}

static char *gn_itoa(int value){
	char *val_str;
	int tmp = value, len = 1;

	while((tmp /= 10) > 0)
		len ++;

	val_str = (char *) gn_malloc(len + 1);
	sprintf(val_str, "%d", value);

	return val_str;
}

void google_retrieve_data_hook_callback(void (*callback)(char *)) {
	data_retrieve_cb = callback;	
}

int gn_connect(googlenest_context *googlenest, char *host, int port){
	int ret;
	ssl_context *ssl = &googlenest->ssl;

	memory_set_own(gn_malloc, gn_free);
	googlenest->socket = -1;
	googlenest->host = host;
	if(port <= 0) 
		port = PORTNUMBER;

//SSL connect
	GOOGLENEST_DEBUG("net_connect to %s:%d", googlenest->host, port);

	if((ret = net_connect(&googlenest->socket, googlenest->host, port)) != 0) {
		GOOGLENEST_ERROR("net_connect ret(%d)", ret);
		goto exit;
	}

	GOOGLENEST_DEBUG("net_connect ok");

//SSL configuration
	GOOGLENEST_DEBUG("do ssl_init");

	if((ret = ssl_init(ssl)) != 0) {
		GOOGLENEST_ERROR("ssl_init ret(%d)", ret);
		goto exit;
	}

	GOOGLENEST_DEBUG("ssl_init ok");

	ssl_set_endpoint(ssl, SSL_IS_CLIENT);
	ssl_set_authmode(ssl, SSL_VERIFY_NONE);
	ssl_set_rng(ssl, my_random, NULL);
	ssl_set_bio(ssl, net_recv, &googlenest->socket, net_send, &googlenest->socket);
	
//SSL handshake
	GOOGLENEST_DEBUG("do ssl_handshake");

	if((ret = ssl_handshake(ssl)) != 0) {
		GOOGLENEST_ERROR("ssl_handshake ret(-0x%x)", -ret);
		goto exit;
	}

	GOOGLENEST_DEBUG("ssl_handshake ok");
	GOOGLENEST_DEBUG("SSL ciphersuite: %s", ssl_get_ciphersuite(ssl));
	return 0;

exit:
#ifdef POLARSSL_ERROR_C
	if(ret != 0) {
		char error_buf[100];
		polarssl_strerror(ret, error_buf, 100);
		GOOGLENEST_ERROR("\n\rLast error was: %d - %s\n", ret, error_buf);
	}
#endif
	if(googlenest->socket != -1) {
		net_close(googlenest->socket);
		googlenest->socket = -1;
        ret = -1 ;
	}
	ssl_free(ssl);
	return ret;
}

void gn_close(googlenest_context *googlenest){
	if(googlenest->socket != -1) {
		net_close(googlenest->socket);
		googlenest->socket = -1;
	}

	ssl_free(&googlenest->ssl);
}

static int gn_stream_get(googlenest_context *googlenest, gn_response_result_t gn_rsp_result){
	char *request = NULL;
	int ret, read_size;
	unsigned char *buffer;
	ssl_context *ssl = &googlenest->ssl;

//Send https request
	request = (char *) gn_malloc(strlen("GET /") + strlen(gn_rsp_result.uri) + strlen(" HTTP/1.1\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\n\r\n") + 1);
	sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", gn_rsp_result.uri, googlenest->host);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}
	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);
	buffer = (unsigned char *) gn_malloc(BUFFER_SIZE);
	if(buffer == NULL){
		GOOGLENEST_ERROR("Malloc buffer failed");
		goto exit;
	}
//Receive https response
	while(1) {
		memset(buffer, 0, BUFFER_SIZE);
		if((ret = ssl_read(ssl, buffer, BUFFER_SIZE - 1)) <= 0) {
			ret = 0;
			break;
		}

		read_size = ret;
		
		GOOGLENEST_DEBUG("read %d bytes:\n%s", read_size, buffer);
		memset(&gn_rsp_result, 0, sizeof(gn_rsp_result));
		if((ret = parse_https_response(buffer, read_size, &gn_rsp_result)) == 1){
			if(gn_rsp_result.status_code == 307){
				GOOGLENEST_DEBUG("\nRedirect address is %s, URI is %s", gn_rsp_result.url, gn_rsp_result.uri);
				ret = 1;
				if(request)
					gn_free(request);
				if(buffer)
					gn_free(buffer);
				gn_close(googlenest);
				vTaskDelay(2000);
				printf("\nStart reconnecting for the streaming...\n");
				if(gn_stream_start(googlenest, gn_rsp_result) != 0){
					ret = -1;
					goto exit;
				}
			}
			else if (gn_rsp_result.status_code == 200){
				printf("\r\nStart getting the event!\r\n");
				if(gn_rsp_result.data_start < read_size){					
					GOOGLENEST_DEBUG("data_start: %d, read_size: %d", gn_rsp_result.data_start, read_size);
					unsigned char *data_bak = gn_malloc(read_size - gn_rsp_result.data_start);
					memset(data_bak, 0, (read_size - gn_rsp_result.data_start));
					memcpy(data_bak, buffer + gn_rsp_result.data_start, (read_size - gn_rsp_result.data_start));
					if((memcmp(&buffer[read_size -1], "\r", 1) == 0) || (memcmp(&buffer[read_size -1], "\n", 1) == 0) && (memcmp(&buffer[read_size -3], "}", 1) == 0)){
						memset(buffer, 0, BUFFER_SIZE);
						memcpy(buffer, data_bak, (read_size - gn_rsp_result.data_start));
						gn_free(data_bak);
						if(data_retrieve_cb != NULL)
							data_retrieve_cb(buffer);
					}
					else{	
						memset(buffer, 0, BUFFER_SIZE);
						memcpy(buffer, data_bak, (read_size - gn_rsp_result.data_start));
						gn_free(data_bak);
						if((ret = ssl_read(ssl, buffer + (read_size - gn_rsp_result.data_start), BUFFER_SIZE - (read_size - gn_rsp_result.data_start) - 1)) <= 0) {
							ret = 0;
							break;
						}
						GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
						if(data_retrieve_cb != NULL)
							data_retrieve_cb(buffer);
					}

				}
				while(1) {
					memset(buffer, 0, BUFFER_SIZE);
					if((ret = ssl_read(ssl, buffer, BUFFER_SIZE - 1)) <= 0) {
						ret = 0;
						break;
					}
					GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
					if(data_retrieve_cb != NULL)
						data_retrieve_cb(buffer);
				}	
			}
			else{
				GOOGLENEST_ERROR("The response code is not correct");
				ret = -1;
				goto exit;
			}
		}
		else{			
			GOOGLENEST_ERROR("Parse the google nest response failed");
			ret = -1;
			goto exit;
		}			
		GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, buffer);
	}
exit:
	if(buffer)
		gn_free(buffer);
	if(gn_rsp_result.url)
		gn_free(gn_rsp_result.url);
	if(gn_rsp_result.uri)
		gn_free(gn_rsp_result.uri);
	if(request)
		gn_free(request);
	gn_close(googlenest);
	return ret;
}

static int gn_stream_start(googlenest_context *googlenest, gn_response_result_t gn_rsp_result){
	int ret;
	
//start reconnect
	memset(googlenest, 0, sizeof(googlenest_context));
	if(gn_connect(googlenest, gn_rsp_result.url, PORTNUMBER) == 0) {
		printf("\r\n Reconnection is OK!\r\n");
		if(gn_stream_get(googlenest, gn_rsp_result) != 0){
			GOOGLENEST_ERROR("\nThe streaming is failed\n");
			gn_close(googlenest);
			ret = -1;
		}
		else
			ret = 0;
	}
	else{
		printf("\r\nReconnection is fail! Please try again!\r\n");
		gn_close(googlenest);
		ret = -1;
	}
	return ret;
}

int gn_stream(googlenest_context *googlenest, char *uri){
	char *request = NULL;
	int ret, read_size;
	int i = 0;
	unsigned char *buffer;
	ssl_context *ssl = &googlenest->ssl;
	gn_response_result_t gn_rsp_result;

//Send http request
	request = (char *) gn_malloc(strlen("GET /") + strlen(uri) + strlen(" HTTP/1.1\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\nAccept: text/event-stream\r\n\r\n") + 1);
	sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\nAccept: text/event-stream\r\n\r\n", uri, googlenest->host);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}

	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);
		
//Receive https response
	buffer = (unsigned char *) gn_malloc(BUFFER_SIZE);
	if(buffer == NULL){
		GOOGLENEST_ERROR("Malloc buffer failed");
		goto exit;
	}

	while(1) {
		memset(buffer, 0, sizeof(buffer));

		if((ret = ssl_read(ssl, buffer, (BUFFER_SIZE - 1))) <= 0) {
			ret = 0;
			break;
		}
		read_size = ret;
		GOOGLENEST_DEBUG("read %d bytes:\n%s", read_size, buffer);
		memset(&gn_rsp_result, 0, sizeof(gn_rsp_result));
		if((ret = parse_https_response(buffer, read_size, &gn_rsp_result)) == 1){
			if(gn_rsp_result.status_code == 307){
				GOOGLENEST_DEBUG("\nRedirect address is %s, URI is %s", gn_rsp_result.url, gn_rsp_result.uri);
				ret = 1;
				if(request) 
					gn_free(request);
				if(buffer) 
					gn_free(buffer);
				gn_close(googlenest);
				vTaskDelay(2000);
				printf("\nStart reconnecting for the streaming...\n");
				if(gn_stream_start(googlenest, gn_rsp_result) != 0){
					ret = -1;
					goto exit;
				}
			}
			else if (gn_rsp_result.status_code == 200){
				printf("\r\nStart getting the event!\r\n");
				
				if(gn_rsp_result.data_start < read_size){					
					GOOGLENEST_DEBUG("data_start: %d, read_size: %d", gn_rsp_result.data_start, read_size);
					unsigned char *data_bak = gn_malloc(read_size - gn_rsp_result.data_start);
					memset(data_bak, 0, (read_size - gn_rsp_result.data_start));
					memcpy(data_bak, buffer + gn_rsp_result.data_start, (read_size - gn_rsp_result.data_start));
					if(memcmp(&buffer[read_size -1], "}", 1) == 0){
						memset(buffer, 0, BUFFER_SIZE);
						memcpy(buffer, data_bak, (read_size - gn_rsp_result.data_start));
						gn_free(data_bak);
						if(data_retrieve_cb != NULL)
							data_retrieve_cb(buffer);
					}
					else{	
						memset(buffer, 0, BUFFER_SIZE);
						memcpy(buffer, data_bak, (read_size - gn_rsp_result.data_start));
						gn_free(data_bak);
						if((ret = ssl_read(ssl, buffer + (read_size - gn_rsp_result.data_start), BUFFER_SIZE - (read_size - gn_rsp_result.data_start) - 1)) <= 0) {
							ret = 0;
							break;
						}
						GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
						if(data_retrieve_cb != NULL)
							data_retrieve_cb(buffer);
					}
						
				}
				while(1) {
					memset(buffer, 0, BUFFER_SIZE);
					if((ret = ssl_read(ssl, buffer, BUFFER_SIZE - 1)) <= 0) {
						ret = 0;
						break;
					}
					GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
					if(data_retrieve_cb != NULL)
						data_retrieve_cb(buffer);
				}	
			}
			else{
				GOOGLENEST_ERROR("The response code is not correct");
				ret = -1;
				goto exit;
			}
		}
		else{			
			GOOGLENEST_ERROR("Parse the google nest response failed");
			ret = -1;
			goto exit;
		}			
	}
exit:
	if(buffer)
		gn_free(buffer);
	if(gn_rsp_result.url)
		gn_free(gn_rsp_result.url);
	if(gn_rsp_result.uri)
		gn_free(gn_rsp_result.uri);
	if(request) 
		gn_free(request);
	gn_close(googlenest);
	return ret;
}

int gn_put(googlenest_context *googlenest, char *uri, char *content){
	char *request = NULL;
	int ret;
	unsigned char buffer[RESPONSE_SIZE];
	ssl_context *ssl = &googlenest->ssl;

//Send https request
	char *len_str = gn_itoa((int)strlen(content));
	request = (char *) gn_malloc(strlen("PUT /") + strlen(uri) + strlen(" HTTP/1.1\r\nConnection:close\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: ") + strlen(len_str) + strlen("\r\nCache-Control: no-cache\r\n\r\n") + strlen(content) + strlen("\r\n") + 1);
	sprintf(request, "PUT /%s HTTP/1.1\r\nConnection:close\r\nHost: %s\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: %s\r\nCache-Control: no-cache\r\n\r\n%s\r\n", uri, googlenest->host, len_str, content);
	gn_free(len_str);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}

	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);

//Receive https response
	while(1) {
		memset(buffer, 0, sizeof(buffer));

		if((ret = ssl_read(ssl, buffer, sizeof(buffer) - 1)) <= 0) {
			ret = 0;
			break;
		}
		GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
	}

exit:
	if(request) 
		gn_free(request);
	gn_close(googlenest);
	return ret;
}

int gn_post(googlenest_context *googlenest, char *uri, char *content, unsigned char *out_buffer, size_t out_len){
	char *request = NULL;
	int ret, read_size;
	unsigned char buffer[RESPONSE_SIZE];
	ssl_context *ssl = &googlenest->ssl;

//Send https request
	char *len_str = gn_itoa((int)strlen(content));
	request = (char *) gn_malloc(strlen("POST /") + strlen(uri) + strlen(" HTTP/1.1\r\nConnection:close\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: ") + strlen(len_str) + strlen("\r\nCache-Control: no-cache\r\n\r\n") + strlen(content) + strlen("\r\n") + 1);
	sprintf(request, "POST /%s HTTP/1.1\r\nConnection:close\r\nHost: %s\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: %s\r\nCache-Control: no-cache\r\n\r\n%s\r\n", uri, googlenest->host, len_str, content);
	gn_free(len_str);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}
	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);
	
//Receive https response
	while(1) {
		memset(buffer, 0, sizeof(buffer));

		if((ret = ssl_read(ssl, buffer, sizeof(buffer) - 1)) <= 0) {
			ret = 0;
			break;
		}

		read_size = ret;
		GOOGLENEST_DEBUG("read %d bytes:\n%s", read_size, (char *) buffer);

		// Update the last message
		memset(out_buffer, 0, out_len);
		memcpy(out_buffer, buffer, (read_size > (out_len - 1))? (out_len - 1) : read_size);
	}

exit:
	if(request)
		gn_free(request);
	gn_close(googlenest);
	return ret;
}

int gn_patch(googlenest_context *googlenest, char *uri, char *content){
	char *request = NULL;
	int ret;
	unsigned char buffer[RESPONSE_SIZE];
	ssl_context *ssl = &googlenest->ssl;

//Send https request
	char *len_str = gn_itoa((int)strlen(content));
	request = (char *) gn_malloc(strlen("PATCH /") + strlen(uri) + strlen(" HTTP/1.1\r\nConnection:close\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: ") + strlen(len_str) + strlen("\r\nCache-Control: no-cache\r\n\r\n") + strlen(content) + strlen("\r\n") + 1);
	sprintf(request, "PATCH /%s HTTP/1.1\r\nConnection:close\r\nHost: %s\r\nContent-Type:application/json;charset=utf-8\r\nContent-Length: %s\r\nCache-Control: no-cache\r\n\r\n%s\r\n", uri, googlenest->host, len_str, content);
	gn_free(len_str);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}
	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);

//Receive https response
	while(1) {
		memset(buffer, 0, sizeof(buffer));

		if((ret = ssl_read(ssl, buffer, sizeof(buffer) - 1)) <= 0) {
			ret = 0;
			break;
		}
		GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
	}

exit:
	if(request)
		gn_free(request);
	return ret;
}

int gn_get(googlenest_context *googlenest, char *uri, unsigned char *out_buffer, size_t out_len){
	char *request = NULL;
	int ret, read_size;
	unsigned char buffer[BUFFER_SIZE];
	ssl_context *ssl = &googlenest->ssl;

//Send https request
	request = (char *) gn_malloc(strlen("GET /") + strlen(uri) + strlen(" HTTP/1.1\r\nConnection:close\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\n\r\n") + 1);
	sprintf(request, "GET /%s HTTP/1.1\r\nConnection:close\r\nHost: %s\r\n\r\n", uri, googlenest->host);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}
	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);

//Receive https response
	while(1) {
		memset(buffer, 0, sizeof(buffer));
		if((ret = ssl_read(ssl, buffer, sizeof(buffer) - 1)) <= 0) {
			ret = 0;
			break;
		}

		read_size = ret;
		GOOGLENEST_DEBUG("read %d bytes:\n%s", read_size, (char *) buffer);

		// Update the last message
		memset(out_buffer, 0, out_len);
		memcpy(out_buffer, buffer, (read_size > (out_len - 1))? (out_len - 1) : read_size);
	}

exit:
	if(request) 
		gn_free(request);
	gn_close(googlenest);
	return ret;
}

int gn_delete(googlenest_context *googlenest, char *uri)
{
	char *request = NULL;
	int ret;
	unsigned char buffer[RESPONSE_SIZE];
	ssl_context *ssl = &googlenest->ssl;

//Send http request
	request = (char *) gn_malloc(strlen("DELETE /") + strlen(uri) + strlen(" HTTP/1.1\r\nConnection:close\r\nHost: ") + strlen(googlenest->host) +
	          strlen("\r\nContent-Type:application/json;charset=utf-8\r\nCache-Control: no-cache\r\n\r\n") + 1);
	sprintf(request, "DELETE /%s HTTP/1.1\r\nConnection:close\r\nHost: %s\r\nContent-Type:application/json;charset=utf-8\r\nCache-Control: no-cache\r\n\r\n", uri, googlenest->host);

	while((ret = ssl_write(ssl, request, strlen(request))) <= 0) {
		if(ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE) {
			GOOGLENEST_ERROR("ssl_write ret(%d)", ret);
			goto exit;
		}
	}
	GOOGLENEST_DEBUG("write %d bytes:\n%s", ret, request);
	
//Receive https response
	while(1) {
		memset(buffer, 0, sizeof(buffer));

		if((ret = ssl_read(ssl, buffer, sizeof(buffer) - 1)) <= 0) {
			ret = 0;
			break;
		}

		GOOGLENEST_DEBUG("read %d bytes:\n%s", ret, (char *) buffer);
	}

exit:
	if(request) 
		gn_free(request);
	gn_close(googlenest);
	return ret;
}
