#include "FreeRTOS.h"
#include "task.h"
#include "platform_stdlib.h"
#include "lwip/sockets.h"
#include "httpd.h"
#include "httpd_util.h"

extern struct httpd_page *httpd_page_database;
extern struct httpd_conn *httpd_connections;
extern uint8_t httpd_max_conn;

void *httpd_malloc(size_t size)
{
	void *ptr = pvPortMalloc(size);
//	httpd_log_verbose("%p = malloc(%d)", ptr, size);
	return ptr;
}

void httpd_free(void *ptr)
{
	vPortFree(ptr);
//	httpd_log_verbose("free(%p)", ptr);
}

int httpd_page_add(char *path, void (*callback)(struct httpd_conn *conn))
{
	struct httpd_page *page = (struct httpd_page *) httpd_malloc(sizeof(struct httpd_page));

	if(page == NULL)
		return -1;

	memset(page, 0, sizeof(struct httpd_page));
	page->path = path;
	page->callback = callback;

	if(httpd_page_database == NULL) {
		httpd_page_database = page;
	}
	else {
		struct httpd_page *cur_page = httpd_page_database;
		struct httpd_page *last_page = NULL;

		while(cur_page) {
			last_page = cur_page;
			cur_page = cur_page->next;
		}

		last_page->next = page;
	}

	return 0;
}

void httpd_page_remove(struct httpd_page *page)
{
	if(page == httpd_page_database) {
		httpd_page_database = page->next;
		httpd_free(page);
	}
	else {
		struct httpd_page *cur_page = httpd_page_database;
		struct httpd_page *prev_page = NULL;

		while(cur_page) {
			if(cur_page == page) {
				prev_page->next = cur_page->next;
				httpd_free(page);
				break;
			}

			prev_page = cur_page;
			cur_page = cur_page->next;
		}
	}
}

void httpd_page_clear(void)
{
	struct httpd_page *cur_page = httpd_page_database;
	struct httpd_page *tmp_page = NULL;

	while(cur_page) {
		tmp_page = cur_page;
		cur_page = cur_page->next;
		httpd_free(tmp_page);
	}

	httpd_page_database = NULL;
}

struct httpd_conn *httpd_conn_add(int sock)
{
	int i;
	struct httpd_conn *conn = NULL;

	for(i = 0; i < httpd_max_conn; i ++) {
		if(httpd_connections[i].sock == -1) {
			httpd_connections[i].sock = sock;
			conn = &httpd_connections[i];
			break;
		}
	}

	return conn;
}

void httpd_conn_remove(struct httpd_conn *conn)
{
	int i;

	for(i = 0; i < httpd_max_conn; i ++) {
		if(&httpd_connections[i] == conn) {
			if(httpd_connections[i].tls) {
				httpd_tls_close(httpd_connections[i].tls);
				httpd_tls_free(httpd_connections[i].tls);
			}

			if(httpd_connections[i].sock != -1)
				close(httpd_connections[i].sock);

			if(httpd_connections[i].request.header)
				httpd_free(httpd_connections[i].request.header);

			if(httpd_connections[i].response_header)
				httpd_free(httpd_connections[i].response_header);

			memset(&httpd_connections[i], 0, sizeof(struct httpd_conn));
			httpd_connections[i].sock = -1;
			break;
		}
	}
}

void httpd_conn_clear(void)
{
	int i;

	for(i = 0; i < httpd_max_conn; i ++) {
		if(httpd_connections[i].tls) {
			httpd_tls_close(httpd_connections[i].tls);
			httpd_tls_free(httpd_connections[i].tls);
		}

		if(httpd_connections[i].sock != -1)
			close(httpd_connections[i].sock);

		if(httpd_connections[i].request.header)
			httpd_free(httpd_connections[i].request.header);

		if(httpd_connections[i].response_header)
			httpd_free(httpd_connections[i].response_header);

		memset(&httpd_connections[i], 0, sizeof(struct httpd_conn));
		httpd_connections[i].sock = -1;
	}
}

void httpd_conn_close(struct httpd_conn *conn)
{
	httpd_log_verbose("%s(%d)", __FUNCTION__, conn->sock);
	httpd_conn_remove(conn);
}

void httpd_conn_dump_header(struct httpd_conn *conn)
{
	if(conn->request.header) {
		char buf[100];

		if(conn->request.method) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.method, conn->request.method_len);
			printf("\nmethod=[%s]\n", buf);
		}

		if(conn->request.path) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.path, conn->request.path_len);
			printf("\npath=[%s]\n", buf);
		}

		if(conn->request.query) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.query, conn->request.query_len);
			printf("\nquery=[%s]\n", buf);
		}

		if(conn->request.version) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.version, conn->request.version_len);
			printf("\nversion=[%s]\n", buf);
		}

		if(conn->request.host) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.host, conn->request.host_len);
			printf("\nhost=[%s]\n", buf);
		}

		if(conn->request.content_type) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->request.content_type, conn->request.content_type_len);
			printf("\ncontent_type=[%s]\n", buf);
		}

		printf("\ncontent_lenght=%d\n", conn->request.content_len);
	}
}

/* Reserved characters after percent-encoding
 * !	#	$	&	'	(	)	*	+	,	/	:	;	=	?	@	[	]
 * %21	%23	%24	%26	%27	%28	%29	%2A	%2B	%2C	%2F	%3A	%3B	%3D	%3F	%40	%5B	%5D
 */
static char special2char(uint8_t *special)
{
	char ch;
	ch = (special[0] >= 'A') ? ((special[0] & 0xdf) - 'A' + 10): (special[0] - '0');
	ch *= 16;
	ch += (special[1] >= 'A') ? ((special[1] & 0xdf) - 'A' + 10): (special[1] - '0');
	return ch;
}

void httpd_query_remove_special(uint8_t *input, size_t input_len, uint8_t *output, size_t output_len)
{
	size_t len = 0;
	uint8_t *ptr = input;

	while((ptr < (input + input_len)) && (len < output_len)) {
		if(*ptr == '%') {
			output[len] = special2char(ptr + 1);
			ptr += 3;
			len ++;
		}
		else {
			output[len] = *ptr;
			ptr ++;
			len ++;
		}
	}
}

int httpd_write(struct httpd_conn *conn, uint8_t *buf, size_t buf_len)
{
	if(conn->tls)
		return httpd_tls_write(conn->tls, buf, buf_len);

	return write(conn->sock, buf, buf_len);
}

int httpd_read(struct httpd_conn *conn, uint8_t *buf, size_t buf_len)
{
	if(conn->tls)
		return httpd_tls_read(conn->tls, buf, buf_len);

	return read(conn->sock, buf, buf_len);
}
