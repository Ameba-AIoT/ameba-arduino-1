#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include <lwip/sockets.h>

#include "http.h"
#include "xml.h"
#include "cloud_updater.h"

#define CLOUD_ENTRANCE  "176.34.62.248"
#define CLOUD_PORT      80
#define CLOUD_SSL_PORT  443

#define DEBUG_HTTP          0

static unsigned int atoh(char *str)
{
	int i;
	unsigned int value = 0;

	for(i = 0; i < strlen(str); i ++) {
		if((str[i] >= '0') && (str[i] <= '9'))
			value = value * 16 + (str[i] - '0');
		else if((str[i] >= 'a') && (str[i] <= 'f'))
			value = value * 16 + (str[i] - 'a' + 10);
		else if((str[i] >= 'A') && (str[i] <= 'F'))
			value = value * 16 + (str[i] - 'A' + 10);
		else {
			value = 0;
			break;
		}
	}

	return value;
}

#if DEBUG_HTTP
static void dump_msg(char *title, char *request, int length)
{
	int i;

	taskENTER_CRITICAL();
	printf("\n\r%s\n\r", title);
	
	for(i = 0; i < length; i ++) {
		if(request[i] == '\r')
			printf("\\r ");
		else if(request[i] == '\n')
			printf("\\n ");
		else
			printf("%c", request[i]);
	}

	printf("\n\r");
	taskEXIT_CRITICAL();
}
#endif

static void* updater_malloc(unsigned int size)
{
	return pvPortMalloc(size);
}

static void updater_mfree(void *buf)
{
	vPortFree(buf);
}

static char *updater_get_link(char *repository, char *fpath, unsigned int *size, unsigned int *checksum, char **version)
{
	int server_socket;
	struct sockaddr_in server_addr;
	char *header, *xml_request, *default_ns, *link = NULL;
	char response_buf[512];
	int read_size;
	struct xml_node *request_root, *repository_node, *fpath_node;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(CLOUD_ENTRANCE);
	server_addr.sin_port = ntohs(CLOUD_PORT);

	if(connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		return NULL;

	//Build XML Request Document Tree
	default_ns = (char *) updater_malloc(strlen("http://") + strlen(CLOUD_ENTRANCE) + strlen("/Updater") + 1);
	sprintf(default_ns, "http://%s/Updater", CLOUD_ENTRANCE);
	request_root = xml_new_element(NULL, "GetFirmwareLink", default_ns);
	repository_node = xml_new_element(NULL, "Repository", NULL);
	fpath_node = xml_new_element(NULL, "FilePath", NULL);
	xml_add_child(request_root, repository_node);
	xml_add_child(request_root, fpath_node);
	xml_add_child(repository_node, xml_new_text(repository));
	xml_add_child(fpath_node, xml_new_text(fpath));
	xml_request = xml_dump_tree(request_root);
	xml_delete_tree(request_root);

	//Write HTTP
	header = http_post_header(CLOUD_ENTRANCE, "/cgi-bin/updater", "text/xml", (int) strlen(xml_request));
#if DEBUG_HTTP
	dump_msg("\nsend http header:", header, strlen(header));
	dump_msg("send xml request:", xml_request, strlen(xml_request));
#endif
	write(server_socket, header, strlen(header));
	write(server_socket, xml_request, strlen(xml_request));
	http_free(header);
	xml_free(xml_request);

	//Read HTTP
	while((read_size = read(server_socket, response_buf, sizeof(response_buf))) > 0) {
		struct xml_node *response_root;
		struct xml_node_set *node_set;
#if DEBUG_HTTP
		char *http_header, *http_body;

		if((http_header = http_response_header(response_buf, read_size)) != NULL) {
			dump_msg("\nrecv http header:", http_header, strlen(http_header));
			http_free(http_header);
		}

		if((http_body = http_response_body(response_buf, read_size)) != NULL) {
			dump_msg("recv http body:", http_body, strlen(http_body));
			http_free(http_body);
		}
#endif
		if((response_root = xml_parse_doc(response_buf, read_size, NULL, "GetFirmwareLinkResponse", default_ns)) != NULL) {
			node_set = xml_find_path(response_root, "/GetFirmwareLinkResponse/Firmware/Link");

			if(node_set->count) {
				struct xml_node *link_text = xml_text_child(node_set->node[0]);

				if(link_text) {
					link = (char *) updater_malloc(strlen(link_text->text) + 1);
					strcpy(link, link_text->text);
				}
			}

			xml_delete_set(node_set);

			node_set = xml_find_path(response_root, "/GetFirmwareLinkResponse/Firmware/Size");

			if(node_set->count) {
				struct xml_node *size_text = xml_text_child(node_set->node[0]);

				if(size_text)
					*size = atoi(size_text->text);
			}

			xml_delete_set(node_set);

			node_set = xml_find_path(response_root, "/GetFirmwareLinkResponse/Firmware/Checksum");

			if(node_set->count) {
				struct xml_node *checksum_text = xml_text_child(node_set->node[0]);

				if(checksum_text)
					*checksum = atoh(checksum_text->text);
			}

			xml_delete_set(node_set);

			node_set = xml_find_path(response_root, "/GetFirmwareLinkResponse/Firmware/Version");

			if(node_set->count) {
				struct xml_node *version_text = xml_text_child(node_set->node[0]);

				if(version_text) {
					*version = (char *) updater_malloc(strlen(version_text->text) + 1);
					strcpy(*version, version_text->text);
				}
			}

			xml_delete_set(node_set);

			xml_delete_tree(response_root);
			break;
		}
	}

	updater_mfree(default_ns);
	close(server_socket);

	return link;
}

int updater_init_ctx(struct updater_ctx *ctx, char *repository, char *fpath)
{
	int ret = -1;

	memset(ctx, 0, sizeof(struct updater_ctx));
	ctx->socket = -1;
	ctx->link = updater_get_link(repository, fpath, &(ctx->size), &(ctx->checksum), &(ctx->version));

	if(ctx->link)
		ret = 0;

	return ret;
}

void updater_free_ctx(struct updater_ctx *ctx)
{
	if(ctx->link)
		updater_mfree(ctx->link);

	if(ctx->socket != -1)
		close(ctx->socket);

	if(ctx->version)
		updater_mfree(ctx->version);
}

int updater_read_bytes(struct updater_ctx *ctx, unsigned char *buf, int size)
{
	int read_bytes = 0;
	char host[] = "255.255.255.255";
	char *host_front = strstr(ctx->link, "http://") + strlen("http://");
	char *host_rear = strstr(host_front + 1, "/");
	char *res  = host_rear;

	if((host_rear > host_front) && ((host_rear - host_front) <= strlen(host))) {
		memset(host, 0, sizeof(host));
		memcpy(host, host_front, host_rear - host_front);
	}

	if(ctx->socket == -1) {
		struct sockaddr_in addr;
		ctx->socket = socket(AF_INET, SOCK_STREAM, 0);
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(host);
		addr.sin_port = ntohs(80);

		if(connect(ctx->socket, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
			int header_end = 0;
			char data;
			char *header = http_get_header(host, res);
#if DEBUG_HTTP
			dump_msg("\nsend http header:", header, strlen(header));
#endif
			write(ctx->socket, header, strlen(header));
			http_free(header);

			//Remove http response header
			while(read(ctx->socket, &data, 1) == 1) {
				if((header_end == 0) && (data == '\r'))
					header_end = 1;
				else if((header_end == 1) && (data == '\n'))
					header_end = 2;
				else if((header_end == 2) && (data == '\r'))
					header_end = 3;
				else if((header_end == 3) && (data == '\n'))
					break;
				else
					header_end = 0;
			}
		}
		else {
			close(ctx->socket);
			ctx->socket = -1;
		}
	}

	if(ctx->socket != -1) {
		read_bytes = read(ctx->socket, buf, size);
		ctx->bytes += read_bytes;
	}

	return read_bytes;
}

void updater_test()
{
	struct updater_ctx ctx;

	if(updater_init_ctx(&ctx, "IOT", "Project.bin") == 0) {
		unsigned int checksum = 0;

		printf("\n\rFirmware link: %s, size = %d bytes, checksum = 0x%08x, version = %s\n", 
			ctx.link, ctx.size, ctx.checksum, ctx.version);

		while(ctx.bytes < ctx.size) {
			unsigned char buf[512];
			int read_bytes = 0;

			if((read_bytes = updater_read_bytes(&ctx, buf, sizeof(buf))) > 0) {
				int i;

				for(i = 0; i < read_bytes; i ++)
					checksum += buf[i];

				printf("\rGet firmware %d/%d bytes      ", ctx.bytes, ctx.size);
			}
			else
				break;
		}

		printf("\n\rctx checksum = %08x, computed checksum = %08x\n", ctx.checksum, checksum);
		updater_free_ctx(&ctx);
	}
}
