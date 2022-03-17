#include "FreeRTOS.h"
#include "task.h"
#include "platform_stdlib.h"
#include "lwip/sockets.h"
#include "httpc.h"
#include "httpc_util.h"

#define HTTP_CRLF    "\r\n"

int httpc_request_write_header_start(struct httpc_conn *conn, char *method, char *resource, char *content_type, size_t content_len)
{
	char buf[200];
	int ret = 0;
	size_t header_len = 0;

	// remove previous request header
	if(conn->request_header) {
		httpc_free(conn->request_header);
		conn->request_header = NULL;
	}

	header_len = sprintf(buf, "%s %s HTTP/1.1%sHost: %s:%d%s", method, resource, HTTP_CRLF, conn->host, conn->port, HTTP_CRLF);

	if(content_type)
		header_len += sprintf(buf, "Content-Type: %s%s", content_type, HTTP_CRLF);

	if(content_len)
		header_len += sprintf(buf, "Content-Length: %d%s", content_len, HTTP_CRLF);

	if(conn->user_password)
		header_len += sprintf(buf, "Authorization: Basic %s%s", conn->user_password, HTTP_CRLF);

	conn->request_header = (uint8_t *) httpc_malloc(header_len + 1);

	if(conn->request_header) {
		memset(conn->request_header, 0, header_len + 1);
		sprintf(conn->request_header, "%s %s HTTP/1.1%sHost: %s:%d%s", method, resource, HTTP_CRLF, conn->host, conn->port, HTTP_CRLF);

		if(content_type)
			sprintf(conn->request_header + strlen(conn->request_header), "Content-Type: %s%s", content_type, HTTP_CRLF);

		if(content_len)
			sprintf(conn->request_header + strlen(conn->request_header), "Content-Length: %d%s", content_len, HTTP_CRLF);

		if(conn->user_password)
			sprintf(conn->request_header + strlen(conn->request_header), "Authorization: Basic %s%s", conn->user_password, HTTP_CRLF);
	}
	else {
		httpc_log("ERROR: httpc_malloc");
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

int httpc_request_write_header(struct httpc_conn *conn, char *name, char *value)
{
	char buf[200];
	int ret = 0;
	uint8_t *request_header = NULL;
	size_t header_len = 0;

	if(conn->request_header) {
		header_len = sprintf(buf, "%s: %s%s", name, value, HTTP_CRLF);
		header_len += strlen(conn->request_header);
		request_header = (uint8_t *) httpc_malloc(header_len + 1);

		if(request_header) {
			memset(request_header, 0, header_len + 1);
			sprintf(request_header, "%s", conn->request_header);
			sprintf(request_header + strlen(request_header), "%s: %s%s", name, value, HTTP_CRLF);
			httpc_free(conn->request_header);
			conn->request_header = request_header;
		}
		else {
			httpc_log("ERROR: httpc_malloc");
			ret = -1;
			goto exit;
		}
	}
	else {
		httpc_log("ERROR: no header start");
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

int httpc_request_write_header_finish(struct httpc_conn *conn)
{
	int ret = 0;
	uint8_t *request_header = NULL;
	size_t header_len = 0;

	if(conn->request_header) {
		header_len = strlen(HTTP_CRLF);
		header_len += strlen(conn->request_header);
		request_header = (uint8_t *) httpc_malloc(header_len + 1);

		if(request_header) {
			memset(request_header, 0, header_len + 1);
			sprintf(request_header, "%s", conn->request_header);
			sprintf(request_header + strlen(request_header), "%s", HTTP_CRLF);
			ret = httpc_write(conn, request_header, strlen(request_header));
			httpc_free(request_header);
			httpc_free(conn->request_header);
			conn->request_header = NULL;
		}
		else {
			httpc_log("ERROR: httpc_malloc");
			ret = -1;
			goto exit;
		}
	}
	else {
		httpc_log("ERROR: no header start");
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

int httpc_request_write_data(struct httpc_conn *conn, uint8_t *data, size_t data_len)
{
	return httpc_write(conn, data, data_len);
}

int httpc_response_is_status(struct httpc_conn *conn, char *status)
{
	int ret = 0;

	if((strlen(status) == conn->response.status_len) &&
	   (memcmp(status, conn->response.status, conn->response.status_len) == 0)) {

		ret = 1;
	}

	return ret;
}

int httpc_response_read_header(struct httpc_conn *conn)
{
	int ret = 0;
	uint8_t *tmp_buf = NULL;
	size_t tmp_buf_size = 1024;
	size_t header_len = 0;
	int header_existed = 0;
	int header_end_steps = 0;
	uint8_t data_byte = 0;

	httpc_log_verbose("%s", __FUNCTION__);

	// remove previous response header for keepalive connection
	if(conn->response.header) {
		httpc_free(conn->response.header);
		memset(&conn->response, 0, sizeof(struct http_response));
	}

	tmp_buf = (uint8_t *) httpc_malloc(tmp_buf_size);

	if(tmp_buf == NULL) {
		httpc_log("ERROR: httpc_malloc");
		ret = -1;
		goto exit;
	}
	else {
		memset(tmp_buf, 0, tmp_buf_size);
	}

	header_end_steps = 0;

	while((httpc_read(conn, &data_byte, 1) == 1) && (header_len < tmp_buf_size)) {
		if((data_byte == '\r') && (header_end_steps == 0))
			header_end_steps = 1;
		else if((data_byte == '\n') && (header_end_steps == 1))
			header_end_steps = 2;
		else if((data_byte == '\r') && (header_end_steps == 2))
			header_end_steps = 3;
		else if((data_byte == '\n') && (header_end_steps == 3))
			header_end_steps = 4;
		else
			header_end_steps = 0;

		tmp_buf[header_len] = data_byte;
		header_len ++;

		if(header_end_steps == 4) {
			header_existed = 1;
			tmp_buf[header_len] = 0;
			httpc_log_verbose("header[%d]: %s", header_len, tmp_buf);
			break;
		}
	}

	if(header_existed) {
		conn->response.header = (uint8_t *) httpc_malloc(header_len + 1);
		conn->response.header_len = header_len;

		if(conn->response.header) {
			uint8_t *ptr;

			memset(conn->response.header, 0, header_len + 1);
			memcpy(conn->response.header, tmp_buf, header_len);
			ret = 0;

			// get version
			ptr = conn->response.header;

			if((conn->response.version = (uint8_t *) strstr(ptr, "HTTP/1.1 ")) == NULL)
				conn->response.version = (uint8_t *) strstr(ptr, "HTTP/1.0 ");

			if(conn->response.version) {
				conn->response.version_len = strlen("HTTP/1.x");
			}
			else {
				ret = -1;
				goto exit;
			}

			// get status
			ptr = conn->response.version + conn->response.version_len;

			while(ptr < (conn->response.header + conn->response.header_len)) {
				if(*ptr != ' ') {
					conn->response.status = ptr;
					break;
				}

				ptr ++;
			}

			ptr = conn->response.status + 1;

			while(ptr < (conn->response.header + conn->response.header_len)) {
				if((*ptr == '\r') && (*(ptr + 1) == '\n')) {
					conn->response.status_len = ptr - conn->response.status;
					break;
				}

				ptr ++;
			}

			if((conn->response.status == NULL) ||
			   (conn->response.status && (conn->response.status_len == 0))) {

				ret = -1;
				goto exit;
			}

			// get content-type
			ptr = conn->response.status + conn->response.status_len;

			if(strstr(ptr, "Content-Type:")) {
				ptr = (uint8_t *) strstr(ptr, "Content-Type:") + strlen("Content-Type:");

				while(ptr < (conn->response.header + conn->response.header_len)) {
					if(*ptr != ' ') {
						conn->response.content_type = ptr;
						break;
					}

					ptr ++;
				}

				ptr = conn->response.content_type + 1;

				while(ptr < (conn->response.header + conn->response.header_len)) {
					if((*ptr == '\r') && (*(ptr + 1) == '\n')) {
						conn->response.content_type_len = ptr - conn->response.content_type;
						break;
					}

					ptr ++;
				}

				if(conn->response.content_type && (conn->response.content_type_len == 0)) {
					ret = -1;
					goto exit;
				}
			}

			// get content-length
			ptr = conn->response.status + conn->response.status_len;

			if(strstr(ptr, "Content-Length:")) {
				uint8_t *length_ptr = NULL;
				ptr = (uint8_t *) strstr(ptr, "Content-Length:") + strlen("Content-Length:");

				while(ptr < (conn->response.header + conn->response.header_len)) {
					if(*ptr != ' ') {
						length_ptr = ptr;
						break;
					}

					ptr ++;
				}

				ptr = length_ptr + 1;

				while(ptr < (conn->response.header + conn->response.header_len)) {
					if((*ptr == '\r') && (*(ptr + 1) == '\n')) {
						char length[10];
						memset(length, 0, sizeof(length));
						memcpy(length, length_ptr, ptr - length_ptr);
						conn->response.content_len = atoi(length);
						break;
					}

					ptr ++;
				}

				if(length_ptr && (conn->response.content_len == 0)) {
					ret = -1;
					goto exit;
				}
			}
		}
		else {
			httpc_log("ERROR: httpc_malloc");
			ret = -1;
			goto exit;
		}
	}
	else {
		tmp_buf[header_len] = 0;
		httpc_log("ERROR: parse header[%d]: %s", header_len, tmp_buf);
		ret = -1;
		goto exit;
	}

exit:
	if(ret && conn->response.header) {
		httpc_free(conn->response.header);
		memset(&conn->response, 0, sizeof(struct http_response));
	}

	if(tmp_buf) {
		httpc_free(tmp_buf);
	}

	return ret;
}

int httpc_response_read_data(struct httpc_conn *conn, uint8_t *data, size_t data_len)
{
	return httpc_read(conn, data, data_len);
}

int httpc_response_get_header_field(struct httpc_conn *conn, char *field, char **value)
{
	int ret = 0;
	char field_buf[50];
	uint8_t *ptr = NULL;
	size_t value_len;

	*value = NULL;
	sprintf(field_buf, "%s:", field);

	if(strstr(conn->response.header, field_buf)) {
		ptr = (uint8_t *) strstr(conn->response.header, field_buf) + strlen(field_buf);

		while(ptr < (conn->response.header + conn->response.header_len)) {
			if(*ptr != ' ')
				break;

			ptr ++;
		}

		if(strstr(ptr, HTTP_CRLF)) {
			value_len = (uint8_t *) strstr(ptr, HTTP_CRLF) - ptr;
			*value = (char *) httpc_malloc(value_len + 1);

			if(*value) {
				memset(*value, 0, value_len + 1);
				memcpy(*value, ptr, value_len);
			}
			else {
				httpc_log("ERROR: httpc_malloc");
				ret = -1;
				goto exit;
			}
		}
		else {
			ret = -1;
			goto exit;
		}
	}
	else {
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}
