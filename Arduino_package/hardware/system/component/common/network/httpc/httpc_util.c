#include "FreeRTOS.h"
#include "task.h"
#include "platform_stdlib.h"
#include "lwip/sockets.h"
#include "httpc.h"
#include "httpc_util.h"

void *httpc_malloc(size_t size)
{
	void *ptr = pvPortMalloc(size);
//	httpc_log_verbose("%p = malloc(%d)", ptr, size);
	return ptr;
}

void httpc_free(void *ptr)
{
	vPortFree(ptr);
//	httpc_log_verbose("free(%p)", ptr);
}

void httpc_conn_dump_header(struct httpc_conn *conn)
{
	if(conn->response.header) {
		char buf[100];

		if(conn->response.version) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->response.version, conn->response.version_len);
			printf("\nversion=[%s]\n", buf);
		}

		if(conn->response.status) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->response.status, conn->response.status_len);
			printf("\nstatus=[%s]\n", buf);
		}

		if(conn->response.content_type) {
			memset(buf, 0, sizeof(buf));
			memcpy(buf, conn->response.content_type, conn->response.content_type_len);
			printf("\ncontent_type=[%s]\n", buf);
		}

		printf("\ncontent_lenght=%d\n", conn->response.content_len);
	}
}

int httpc_write(struct httpc_conn *conn, uint8_t *buf, size_t buf_len)
{
	if(conn->tls)
		return httpc_tls_write(conn->tls, buf, buf_len);

	return write(conn->sock, buf, buf_len);
}

int httpc_read(struct httpc_conn *conn, uint8_t *buf, size_t buf_len)
{
	if(conn->tls)
		return httpc_tls_read(conn->tls, buf, buf_len);

	return read(conn->sock, buf, buf_len);
}
