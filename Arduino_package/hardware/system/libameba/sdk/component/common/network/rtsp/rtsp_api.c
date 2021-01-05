#include "FreeRTOS.h"
#include "task.h"
#include "platform/platform_stdlib.h"
#include "rtsp/rtsp_api.h"

#include "sockets.h"
#include "wifi_conf.h" //for wifi_is_ready_to_transceive
#include "wifi_util.h" //for getting wifi mode info
#include "lwip/netif.h" //for LwIP_GetIP

extern struct netif xnetif[NET_IF_NUM];
extern uint8_t* LwIP_GetIP(struct netif *pnetif);

#define RTSP_CTX_ID_BASE	0
static u32 rtsp_ctx_id_bitmap = 0;
_mutex rtsp_ctx_id_bitmap_lock = NULL;
unsigned portBASE_TYPE rtsp_service_priority = RTSP_SERVICE_PRIORITY;

/* clock usage */
static u32 rtsp_tick = 0;
static u32 old_depend_clock_tick = 0;
static u32 rtsp_clock_hz = 0;

static u8 *plid_string=NULL;
static u8 *sps_string=NULL;
static u8 *pps_string=NULL;

unsigned char h264_flag_adjust = 0;//for h264 time delay
unsigned char aac_flag_adjust = 0;//for aac time delay
//init a clock with input clock frequency
int rtsp_clock_init(u32 clock_hz)
{
        rtsp_clock_hz = clock_hz;
        rtsp_tick = old_depend_clock_tick = rtw_get_current_time();
        return 0;
}

void set_prefilter_packet(struct rtsp_context *rtsp_ctx,u32 num)
{
        rtsp_ctx->pre_filter_packet = num;
}

void time_sync_disable(void)
{
        h264_flag_adjust = 0;
        aac_flag_adjust = 0;
        printf("time_sync_disable\r\n");
}

void time_sync_enable(void)
{
        h264_flag_adjust = 1;
        aac_flag_adjust  = 1;
        printf("time_sync_enable\r\n");
}

void rtsp_clock_deinit()
{
        rtsp_clock_hz = 0;
}

u32 rtsp_get_current_tick()
{
        u32 current_depend_clock_tick;
        u32 delta_depend_clock_tick;
        if(!rtsp_clock_hz)
        {
            RTSP_ERROR("\n\rrtsp clock not init! Cannot get correct tick!");
            return 0;
        }
        current_depend_clock_tick = rtw_get_current_time();
        delta_depend_clock_tick = current_depend_clock_tick - old_depend_clock_tick;
        old_depend_clock_tick = current_depend_clock_tick;
        rtsp_tick += (delta_depend_clock_tick * rtsp_clock_hz) / RTSP_DEPEND_CLK_HZ;
        return rtsp_tick;
}

int rtsp_get_number(int number_base, u32 *number_bitmap, _mutex *bitmap_lock)
{
	int number = -1;
    int i;
	rtw_mutex_get(bitmap_lock);
	for(i = 0; i < 32; i ++)
	{
	    if(!((1<<i)&*number_bitmap))
			break;
	}
	if(i == 32)
	{
	    RTSP_ERROR("\n\rno more bitmap available!");
		rtw_mutex_put(bitmap_lock);
		return number;
	}
	*number_bitmap |= 1<<i;
	number = number_base + i;
	rtw_mutex_put(bitmap_lock);
	return number;
}

void rtsp_put_number(int number, int number_base, u32 *number_bitmap, _mutex *bitmap_lock)
{
    int i = number - number_base;
	rtw_mutex_get(bitmap_lock);
	*number_bitmap &= !(1<<i);
	rtw_mutex_put(bitmap_lock);
}

int rtsp_parse_stream_media_type(struct codec_info *codec)
{
    switch(codec->codec_id)
	{
		case(AV_CODEC_ID_MJPEG):
		case(AV_CODEC_ID_H264):
        case(AV_CODEC_ID_MP4V_ES):
			return AVMEDIA_TYPE_VIDEO;
		case(AV_CODEC_ID_PCMU):
		case(AV_CODEC_ID_PCMA):
        case(AV_CODEC_ID_MP4A_LATM):
		    return AVMEDIA_TYPE_AUDIO;
		default:
		    return AVMEDIA_TYPE_UNKNOWN;
	}    
}

int rtsp_connect_ctx_init(struct rtsp_context *rtsp_ctx)
{
        struct connect_context *connect_ctx = &rtsp_ctx->connect_ctx;
	connect_ctx->server_ip = LwIP_GetIP(&xnetif[0]);
	connect_ctx->server_port = rtsp_ctx->id + DEF_RTSP_PORT;
	connect_ctx->socket_id = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_ctx->socket_id < 0)
	{
		RTSP_ERROR("\n\rrtsp server socket create failed!");
		return -EINVAL;
	}
	return 0;
}

void rtsp_transport_init(struct rtsp_context *rtsp_ctx)
{
        struct rtsp_transport *transport = &rtsp_ctx->transport[0];
        for(int i=0; i<RTSP_MAX_STREAM_NUM; i++){
                transport += i;
                transport->serverport_low = rtsp_ctx->id * 2 + RTP_SERVER_PORT_BASE;
                transport->serverport_high = rtsp_ctx->id * 2 + RTP_SERVER_PORT_BASE + 1;
                transport->port_low = rtsp_ctx->id * 2 + RTP_PORT_BASE;
                transport->port_high = rtsp_ctx->id * 2 + RTP_PORT_BASE + 1;
                transport->clientport_low = rtsp_ctx->id * 2 + RTP_CLIENT_PORT_BASE;
                transport->clientport_high = rtsp_ctx->id * 2 + RTP_CLIENT_PORT_BASE + 1;
                transport->isRtp = 1;
                transport->isTcp = 0;
                transport->castMode = UNICAST_UDP_MODE;
                transport->ttl = 0;                        
        }
}

void rtsp_session_init(struct rtsp_context *rtsp_ctx)
{
	struct rtsp_session *session = &rtsp_ctx->session;
        session->id = 0;
        session->version = 0;
        session->start_time = 0;
        session->end_time = 0;
        session->name = "ameba";
        session->user = "-";
}

void rtsp_stream_context_init(struct rtsp_context *rtsp_ctx, struct stream_context *stream_ctx)
{
    stream_ctx->parent = rtsp_ctx;
	stream_ctx->stream_id = -1;
	INIT_LIST_HEAD(&stream_ctx->input_queue);
	INIT_LIST_HEAD(&stream_ctx->output_queue);
	rtw_mutex_init(&stream_ctx->input_lock);
	rtw_mutex_init(&stream_ctx->output_lock);	
	stream_ctx->codec = NULL;
	stream_ctx->media_type = AVMEDIA_TYPE_UNKNOWN;
	stream_ctx->framerate = 0;
	//initialize rtp statistics
	memset(&stream_ctx->statistics, 0, sizeof(struct rtp_statistics));
}

void rtsp_stream_context_clear(struct stream_context *stream_ctx)
{
        stream_ctx->parent = NULL;
	stream_ctx->stream_id = -1;
	INIT_LIST_HEAD(&stream_ctx->input_queue);
	INIT_LIST_HEAD(&stream_ctx->output_queue);
	rtw_mutex_free(&stream_ctx->input_lock);
	rtw_mutex_free(&stream_ctx->output_lock);	
	stream_ctx->codec = NULL;
	stream_ctx->media_type = AVMEDIA_TYPE_UNKNOWN;
	stream_ctx->framerate = 0;
	//initialize rtp statistics
	memset(&stream_ctx->statistics, 0, sizeof(struct rtp_statistics));	
}

void rtp_stream_statistics_sync(struct stream_context *stream_ctx)
{
    //video & audio stream should be differentiated
        memset(&stream_ctx->statistics, 0, sizeof(struct rtp_statistics));
        stream_ctx->statistics.do_start_check = 1;
	switch(stream_ctx->media_type)
	{
	        case(AVMEDIA_TYPE_VIDEO):
		        stream_ctx->statistics.rtp_tick_inc = stream_ctx->codec->clock_rate / stream_ctx->framerate;
			stream_ctx->statistics.delay_threshold = RTSP_DEPEND_CLK_HZ / stream_ctx->framerate;
			break;
		case(AVMEDIA_TYPE_AUDIO):
                        switch(stream_ctx->codec->codec_id)
                        {
                            case(AV_CODEC_ID_PCMU):
                            case(AV_CODEC_ID_PCMA):
                                stream_ctx->statistics.rtp_tick_inc = 160;
                                stream_ctx->statistics.delay_threshold = /*(160*RTSP_DEPEND_CLK_HZ)/stream_ctx->codec->clock_rate*/20;
                                break;
                            case(AV_CODEC_ID_MP4A_LATM):
                                stream_ctx->statistics.rtp_tick_inc = 1024;
                                stream_ctx->statistics.delay_threshold = (1024*RTSP_DEPEND_CLK_HZ)/stream_ctx->samplerate;
                                break;
			}
                        break;
		default:
		    RTSP_ERROR("\n\rstream media type unsupported!");
		    return;
	}
}

struct rtsp_context *rtsp_context_create(u8 nb_streams)
{
	int i;
    struct rtsp_context *rtsp_ctx = malloc(sizeof(struct rtsp_context));
	if(rtsp_ctx == NULL)
	{
	    RTSP_ERROR("\n\rallocate rtsp context failed");
		return NULL;
	}
        memset(rtsp_ctx, 0, sizeof(struct rtsp_context));
    rtsp_ctx->response = malloc(RTSP_RESPONSE_BUF_SIZE);
	if(rtsp_ctx->response == NULL)
	{
	    RTSP_ERROR("\n\rallocate rtsp response failed");
		free(rtsp_ctx);
		return NULL;
	}
	rtsp_ctx->connect_ctx.remote_ip = malloc(4);
	if(rtsp_ctx->connect_ctx.remote_ip == NULL)
	{
		RTSP_ERROR("\n\rallocate remote ip memory failed");
		free(rtsp_ctx->response);
		free(rtsp_ctx);
		return NULL;
	}
        if(rtsp_ctx_id_bitmap_lock == NULL)
        {
                rtw_mutex_init(&rtsp_ctx_id_bitmap_lock);
        }
	rtsp_ctx->id = rtsp_get_number(RTSP_CTX_ID_BASE, &rtsp_ctx_id_bitmap, &rtsp_ctx_id_bitmap_lock);
	rtsp_ctx->allow_stream = 0;
	rtsp_ctx->state = RTSP_INIT;
	rtsp_transport_init(rtsp_ctx);
	rtsp_session_init(rtsp_ctx);	
	rtsp_ctx->is_rtsp_start = 0;
	rtw_init_sema(&rtsp_ctx->start_rtsp_sema, 0);	
	rtsp_ctx->is_rtp_start = 0;
	rtw_init_sema(&rtsp_ctx->start_rtp_sema, 0);
	rtsp_ctx->rtp_service_handle = NULL;	
        rtsp_ctx->pre_filter_packet = 12;//filter the unstable timestamp
        h264_flag_adjust = 1;//enable h264 time delay
        aac_flag_adjust = 1;//enable aac time delay
#ifdef SUPPORT_RTCP
    rtsp_ctx->is_rtcp_start = 0;
	rtw_init_sema(&rtsp_ctx->start_rtcp_sema, 0);
    rtsp_ctx->rtcp_service_handle = NULL;
#endif
        rtsp_ctx->nb_streams_setup = 0;
	if(nb_streams > RTSP_MAX_STREAM_NUM)
	{
		RTSP_ERROR("\n\rnumber of streams exceed MAX!");
    	nb_streams = RTSP_MAX_STREAM_NUM;
	}
	rtsp_ctx->nb_streams = nb_streams;
	rtsp_ctx->stream_ctx = malloc(nb_streams * sizeof(struct stream_context));	
	if(rtsp_ctx->stream_ctx == NULL)
	{
	    RTSP_ERROR("\n\rallocate rtsp stream context failed");
		free(rtsp_ctx->connect_ctx.remote_ip);
		free(rtsp_ctx->response);
		free(rtsp_ctx);
		return NULL;		
	}
	for(i = 0; i < nb_streams; i++)
	{
            rtsp_ctx->rtpseq[i] = 0;
	    rtsp_stream_context_init(rtsp_ctx, &rtsp_ctx->stream_ctx[i]);
	}
        return rtsp_ctx;
}

void rtsp_context_free(struct rtsp_context *rtsp_ctx)
{
	int i;
	for(i = 0; i < rtsp_ctx->nb_streams; i++)
		rtsp_stream_context_clear(&rtsp_ctx->stream_ctx[i]);
        free(rtsp_ctx->stream_ctx);
	free(rtsp_ctx->response);
	free(rtsp_ctx->connect_ctx.remote_ip);	
	rtw_free_sema(&rtsp_ctx->start_rtp_sema);
	rtw_free_sema(&rtsp_ctx->start_rtsp_sema);	
#ifdef SUPPORT_RTCP
        rtw_free_sema(&rtsp_ctx->start_rtcp_sema);
#endif	
        rtsp_put_number(rtsp_ctx->id, RTSP_CTX_ID_BASE, &rtsp_ctx_id_bitmap, &rtsp_ctx_id_bitmap_lock);
        if(rtsp_ctx_id_bitmap == 0)
                rtw_mutex_free(&rtsp_ctx_id_bitmap_lock);
	free(rtsp_ctx);        
}

int rtsp_get_request_len(u8 *request)
{
    u8 * ptr = request;
	int end = 0;
	int len = 0;
	while(*ptr != 0)
	{
		len ++;
		if((*ptr == '\r') && (end == 0))
		{
			end = 1;
		}else if((*ptr == '\n') && (end == 1))
			{
				end = 2;
			}else if((*ptr == '\r') && (end == 2))
				{
					end = 3;
				}else if((*ptr == '\n') && (end ==3))
					{
						break;
					}else{
						end = 0;
					}
		ptr++;
	}
	if(*ptr == 0)
	{
		return len;
	}
	*(request + len) = '\0';
	return len;
}

u8 * rtsp_parse_header_line(struct rtsp_context *rtsp_ctx, u8 *header)
{
        u8 *ptr = header;
	int len = 0;
	int end = 0;
	u8 method[16] = {0};
	
	while((*ptr != ' ')&&(*ptr != '\0'))
	{
	    ptr ++;
            len ++;
	}
	if(*ptr == '\0')
	{
	    rtsp_ctx->request_type = 0;
	    RTSP_ERROR("\n\rinvalid request!");
	    return ptr;		
	}
	memcpy(method, header, len);
	method[len] = '\0';
	
	if(!strcmp(method, "OPTIONS"))
	{
		rtsp_ctx->request_type = REQUEST_OPTIONS;
	}else if(!strcmp(method, "DESCRIBE"))
		{
			rtsp_ctx->request_type = REQUEST_DESCRIBE;
		}else if(!strcmp(method, "SETUP"))
			{
				rtsp_ctx->request_type = REQUEST_SETUP;
			}else if(!strcmp(method, "TEARDOWN"))
				{
					rtsp_ctx->request_type = REQUEST_TEARDOWN;
				}else if(!strcmp(method, "PLAY"))
					{
						rtsp_ctx->request_type = REQUEST_PLAY;
					}else if(!strcmp(method, "PAUSE"))
						{
							rtsp_ctx->request_type = REQUEST_PAUSE;
						}else if(!strcmp(method, "GET_PARAMETER")){
                                                        rtsp_ctx->request_type = REQUEST_GET_PARAM;
                                                  }else{
                                                                  rtsp_ctx->request_type = 0; //unknown cmd type
                                                  }
	
//turn on to parse URL	
#if 0
//to do...
#else
    while(*ptr != '\0')
	{
		ptr++;
	    if((*ptr == '\r')&&(end == 0))
		{
			end = 1;
		}else if((*ptr == '\n')&&(end == 1))
		{
		    ptr++;
			return ptr;
		}else
		{
		    end = 0;
		}
	}
	return ptr;
#endif	
}

u8 * rtsp_parse_body_line(struct rtsp_context *rtsp_ctx, u8 *body)
{
        u8 *start, *end, *mov;
	start = end = body;
	int offset = 0;
	u8 temp[64] = {0};
	//return if we parse to the end of the message body(\r\n\r\n)
	if((*end == '\r') || (*end == '\n'))
	{
	    while(*end != '\0')
		    end++;
	    return end;
	}
	//parse field
	while((*end != ' ')&&(*end != '\0'))
	{
	    end++;
	}
	offset = end - start;
	memcpy(temp, start, offset - 1); //ignore ':'
	temp[offset - 1] = '\0';
	if(!strcmp(temp, "CSeq")) //parse CSeq
	{
	    memset(temp, 0, 64);
		end++; //skip ' '
		start = end;
		while((*end != '\n')&&(*end != '\0'))
		{
			while((*end != ';') && (*end != '\r'))
			{
				end++;
			}
			offset = end - start;
			memcpy(temp, start, offset);
			temp[offset] = '\0';	
			rtsp_ctx->CSeq = atoi(temp);
			end++; //skip '\r' or ';'
			break;
		}
	}else if(!strcmp(temp, "Transport")) //parse Transport
	    {
		    memset(temp, 0, 64);
                    end++;
                    start = end;
                    while((*end != '\n')&&(*end != '\0'))
                    {
                            while((*end != ';') && (*end != '\r'))
                            {
                                    end++;
                            }
                            offset = end - start;
                            memcpy(temp, start, offset);
                            temp[offset] = '\0';
                            if(!strncmp(temp, "RTP", 3))
                            {
                              rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].isRtp = 1;
                                    rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].isTcp = 0;//default udp to be lower transport protocol
                                    
                            }else if(!strncmp(temp, "TCP", 3))
                                    {
                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].isTcp = 1;
                                    }else if(!strncmp(temp, "unicast", 7))
                                            {
                                                    if(rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].isTcp){
                                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode = UNICAST_TCP_MODE;
                                                    }else{
                                                        rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode = UNICAST_UDP_MODE;
                                                    }
                                            }else if(!strncmp(temp, "multicast", 9))
                                                    {
                                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode = MULTICAST_MODE;
                                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].isTcp = 0;
                                                    }else if(!strncmp(temp, "ttl=", 4))
                                                            {
                                                                    start = start + 4;
                                                                    mov = start;
                                                                    while((*mov != ';')&&(*mov != '\r'))
                                                                            mov++;
                                                                    offset = mov - start;
                                                                    memset(temp, 0, 64);
                                                                    memcpy(temp, start, offset);
                                                                    temp[offset] = '\0';
                                                                    rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].ttl = atoi(temp);
                                                            }else if(!strncmp(temp, "client_port=", 12))
                                                                    {
                                                                            start = start + 12;
                                                                            mov = start;
                                                                            while(*mov != '-')
                                                                                    mov++;
                                                                            offset = mov - start;
                                                                            memset(temp, 0, 64);
                                                                            memcpy(temp, start, offset);
                                                                            temp[offset] = '\0';
                                                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_low = atoi(temp);
                                                                            mov++;
                                                                            start = mov;
                                                                            while((*mov != ';') && (*mov != '\r'))
                                                                                    mov++;
                                                                            offset = mov - start;
                                                                            memset(temp, 0, 64);
                                                                            memcpy(temp, start, offset);
                                                                            temp[offset] = '\0';
                                                                            rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_high = atoi(temp);

                                                                    }
                            memset(temp, 0, 64);
                            end++;
                            start = end;
                    }
		}else{
			    while((*end != '\n') && (*end != '\0'))
				    end++;
		     }
        if(*end == '\0')
	    return end;
	else{
		end++; 
		return end;
	}
}

void rtsp_parse_request(struct rtsp_context *rtsp_ctx, u8* request)
{
	if(*request == '\0')
	{
		rtsp_ctx->request_type = 0;
		RTSP_ERROR("\n\rinvalid request!");
		return;
	}
    u8 *pstart = request;
	u8 *pbody = NULL;
	int len = rtsp_get_request_len(request);
	pbody = rtsp_parse_header_line(rtsp_ctx, pstart);
	while(*pbody != '\0')
	{
	    pbody = rtsp_parse_body_line(rtsp_ctx, pbody);
	}
}

static void new_session_id(u32 *session_id)
{
    u32 rand = rtw_get_current_time();
	if(rand < 10000000)
		rand = rand + 10000000;
	*session_id = rand;
}

/* -------------------------------- start of sdp ----------------------------------------*/

static u8 *data_to_hex(u8 *buff, u8 *src, int s, int lowercase)
{
    int i;
    static const char hex_table_uc[16] = { '0', '1', '2', '3',
                                        '4', '5', '6', '7',
                                        '8', '9', 'A', 'B',
                                        'C', 'D', 'E', 'F' };
    static const char hex_table_lc[16] = { '0', '1', '2', '3',
                                        '4', '5', '6', '7',
                                        '8', '9', 'a', 'b',
                                        'c', 'd', 'e', 'f' };
    const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;
    for(i = 0; i < s; i++) {
        buff[i * 2]     = hex_table[src[i] >> 4];
        buff[i * 2 + 1] = hex_table[src[i] & 0xF];
    }

    return buff;
}

static u8 *extradata2config(void *extra)
{
   u8 *config;

   if(strlen(extra) > 1024)
   {
        RTSP_PRINTF("\n\rtoo much extra data!");
        return NULL;
   }
   config = malloc(10 + strlen(extra)*2);
   if (config == NULL) {
       RTSP_PRINTF("\n\rallocate config memory failed");
       return NULL;
   }
   memcpy(config, "; config=", 9);
   data_to_hex(config + 9, extra, strlen(extra), 1);
   config[9 + strlen(extra) * 2] = 0;

   return config;
}

void set_profile_lv_string(char *plid){//called before rtsp_open()
	plid_string = plid;
	return;
}
void set_sps_string(char * sps){//called before rtsp_open()
	sps_string = sps;
	return;
}
void set_pps_string(char * pps){//called before rtsp_open()
	pps_string = pps;
	return;
}

extern u8 base64_sps[128];
extern u8 base64_pps[64];
extern u8 plid[4];
static void create_sdp_a_string(u8 *string, struct stream_context *s, void* extra)
{
	char spspps_string[256];
    u8 * config = NULL;
    if(extra != NULL)
    {
        config = extradata2config(extra);
    }
    
    switch(s->codec->codec_id)
	{
		case(AV_CODEC_ID_MJPEG):
				sprintf(string, "a=rtpmap:%d JPEG/%d" CRLF             \
							  "a=control:streamid=%d" CRLF            \
							  "a=framerate:%d" CRLF \
							  , s->codec->pt, s->codec->clock_rate, s->stream_id, s->framerate); 
				break;	
		case(AV_CODEC_ID_H264):
                                if(base64_sps[0]!=0 && base64_sps[0]!=0)
                                {
                                    set_profile_lv_string(plid);
                                    set_sps_string(base64_sps);
                                    set_pps_string(base64_pps);
                                }
				if(plid_string!=NULL){
					strcat(spspps_string, ";profile-level-id=");
					strcat(spspps_string, plid_string);
				}
				if(sps_string!=NULL){
					strcat(spspps_string, ";sprop-parameter-sets=");
					strcat(spspps_string, sps_string);
					if(pps_string!=NULL){
						strcat(spspps_string, ",");
						strcat(spspps_string, pps_string);
					}
				}
		        sprintf(string, "a=rtpmap:%d H264/%d" CRLF \
						"a=control:streamid=%d" CRLF \
                                                  "a=fmtp:%d packetization-mode=0%s%s" CRLF \
                                                    , (s->codec->pt + s->stream_id), s->codec->clock_rate, s->stream_id, (s->codec->pt + s->stream_id), config? config:"", spspps_string);
				break;
		case(AV_CODEC_ID_PCMU):
				sprintf(string, "a=rtpmap:%d PCMU/%d" CRLF             \
								"a=ptime:20" CRLF						\
							  "a=control:streamid=%d" CRLF            \
							  , s->codec->pt, s->samplerate, s->stream_id); 
				break;
		case(AV_CODEC_ID_PCMA):
				sprintf(string, "a=rtpmap:%d PCMA/%d" CRLF             \
								"a=ptime:20" CRLF						\
							  "a=control:streamid=%d" CRLF            \
							  , s->codec->pt, s->samplerate, s->stream_id); 
                                break;
                case(AV_CODEC_ID_MP4A_LATM):
                                sprintf(string, "a=rtpmap:%d mpeg4-generic/%d/%d" CRLF     \
                                                          "a=fmtp:%d streamtype=5; profile-level-id=15; mode=AAC-hbr%s; sizeLength=13; indexLength=3; indexDeltaLength=3; constantDuration=1024; Profile=1"  CRLF         \
                                                            "a=control:streamid=%d" CRLF \
														/*	  "a=type:broadcast"  CRLF \*/
                                                            , (s->codec->pt + s->stream_id), s->samplerate, s->channel,(s->codec->pt + s->stream_id), config? config:"", s->stream_id);  
                                break;
                case(AV_CODEC_ID_MP4V_ES):
                                sprintf(string, "a=rtpmap:%d MPEG4-ES/%d" CRLF     \
                                                          "a=control:streamid=%d" CRLF \
                                                          "a=fmtp:%d profile-level-id=1%s"  CRLF         \
                                                            , (s->codec->pt + s->stream_id), s->codec->clock_rate, s->stream_id, (s->codec->pt + s->stream_id), config? config:"");  
                                break;
		default:
		                break;
	}
    free(config);
}

static int get_frequency_index(int samplerate)
{
	uint32_t freq_idx_map[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
	for(int i=0;i<sizeof(freq_idx_map)/sizeof(freq_idx_map[0]);i++){
		if(samplerate==freq_idx_map[i])
			return i;
	}
	return 0xf;		// 15: frequency is written explictly 
}

void rtsp_create_sdp(struct rtsp_context *rtsp_ctx, u8* sdp_buf, int max_len)
{
    int i;
	struct stream_context *stream;
	u8 attr_buf[MAX_SDP_SIZE];
    //sdp session level
	u8 *unicast_addr, *connection_addr;
        u8 *extra = NULL;
        int offset = 0;
	u8 nettype[] = "IN";
	u8 addrtype[] = "IP4";
	unicast_addr = rtsp_ctx->connect_ctx.server_ip;
	connection_addr = rtsp_ctx->connect_ctx.remote_ip;
	/* fill Protocol Version -- only have Version 0 for now*/
	sprintf(sdp_buf, "v=0" CRLF);
	sdp_fill_o_field(sdp_buf, max_len, rtsp_ctx->session.user, rtsp_ctx->session.id, rtsp_ctx->session.version, nettype, addrtype, unicast_addr);
	sdp_fill_s_field(sdp_buf, max_len, rtsp_ctx->session.name);
	sdp_fill_c_field(sdp_buf, max_len, nettype, addrtype, connection_addr, /*rtsp_ctx->transport.ttl*/0);//should change ttl if not unicast
	sdp_fill_t_field(sdp_buf, max_len, rtsp_ctx->session.start_time, rtsp_ctx->session.end_time);
	//sdp media level
	for(i = 0; i < rtsp_ctx->nb_streams; i++)
	{
     	stream = &rtsp_ctx->stream_ctx[i];
		if(stream->stream_id >= 0)
		{		
			if(stream->codec->pt >= RTP_PT_DYN_BASE)
				sdp_fill_m_field(sdp_buf, max_len, stream->media_type, 0, (stream->codec->pt + stream->stream_id));
			else
				sdp_fill_m_field(sdp_buf, max_len, stream->media_type, 0, stream->codec->pt);
			if(stream->codec->codec_id == AV_CODEC_ID_MP4A_LATM)
			{
				uint16_t config;
				int freq_idx = get_frequency_index(stream->samplerate);
				config = (0x2 << 11)|(freq_idx << 7)|(stream->channel << 3)|0x0;
				
				extra = malloc(3);
				memset(extra, 0, 3);
				//form AAC generic config: object type (5 bit) frequency index (4 bit) channel config (4 bit) + 3 bit zero
				/* 44.1khz LLC LC stereo -- config = 0x1210*/
				/* 16khz LLC LC stereo -- config = 0x1410*/
				if((freq_idx<=12) && (stream->channel!=0)){
					*extra = (config>>8)&0xFF;
					*(extra + 1) = config&0xFF;
				}else{
					*extra = 0x14;
					*(extra + 1) = 0x10;
				}

				*(extra + 2) = 0x00;// '\0'
			}
			memset(attr_buf, 0, MAX_SDP_SIZE);
			create_sdp_a_string(attr_buf, stream, extra);
			if(extra!=NULL){
				free(extra);
				extra=NULL;
			}
			sdp_strcat(sdp_buf, max_len, attr_buf);
		}
	}
}
/* -------------------------------- END of sdp ------------------------------------------*/

void rtsp_cmd_options(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                             "CSeq: %d" CRLF \
								 "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER" CRLF \
								 CRLF, rtsp_ctx->CSeq);
}

void rtsp_cmd_getparm(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
    sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                            "CSeq: %d" CRLF \
                                      "Session: %d:timeout=60" CRLF \
                                        CRLF, rtsp_ctx->CSeq,rtsp_ctx->session.id);
}

void rtsp_cmd_describe(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	u8 sdp_buffer[MAX_SDP_SIZE] = {0};
	int sdp_content_len = 0;
	new_session_id(&rtsp_ctx->session.id);
	rtsp_create_sdp(rtsp_ctx, sdp_buffer, MAX_SDP_SIZE - 1);
	sdp_content_len = strlen(sdp_buffer);
	sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                             "CSeq: %d" CRLF \
								 "Content-Type: application/sdp" CRLF \
								 "Content-Base: rtsp://%d.%d.%d.%d:%d/test.sdp" CRLF \
								 "Content-Length: %d" CRLF \
								 CRLF \
								 "%s", rtsp_ctx->CSeq, (u8)rtsp_ctx->connect_ctx.server_ip[0], (u8)rtsp_ctx->connect_ctx.server_ip[1], (u8)rtsp_ctx->connect_ctx.server_ip[2], (u8)rtsp_ctx->connect_ctx.server_ip[3], rtsp_ctx->connect_ctx.server_port, sdp_content_len, sdp_buffer);
}

void rtsp_cmd_setup(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	char *castmode;
	switch(rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode)
	{
	     case(UNICAST_TCP_MODE):
		 case(UNICAST_UDP_MODE):
		     castmode = "unicast";
			 break;
	     case(MULTICAST_MODE):
		     castmode = "multicast";
		     break;
		 default:
		     castmode = "unknown";
	}
	if(rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode == UNICAST_UDP_MODE)
	{
	     sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
		                             "CSeq: %d" CRLF \
									 "Session: %d;timeout=60" CRLF \
									 "Transport: RTP/AVP/UDP;%s;client_port=%d-%d;server_port=%d-%d" CRLF \
									CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id, castmode, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_low, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_high, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].serverport_low, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].serverport_high);
	}else if(rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode == UNICAST_TCP_MODE)
		{
	         sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
		                             "CSeq: %d" CRLF \
									 "Session: %d" CRLF \
									 "Transport: RTP/AVP/TCP;%s;client_port=%d-%d;server_port=%d-%d" CRLF \
									CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id, castmode, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_low, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].clientport_high, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].serverport_low, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].serverport_high);	
		}else if(rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].castMode == MULTICAST_MODE)
			{
			    sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
											"CSeq: %d" CRLF \
											"Session: %d" CRLF \
											"Transport: RTP/AVP/UDP;%s;port=%d-%d;ttl=%d" CRLF \
											CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id, castmode, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].port_low, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].port_high, rtsp_ctx->transport[rtsp_ctx->nb_streams_setup].ttl);
			}
}

void rtsp_cmd_play(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                             "CSeq: %d" CRLF \
								 "Session: %d" CRLF \
								 CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id);
}

void rtsp_cmd_pause(struct rtsp_context *rtsp_ctx)
{
        memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                             "CSeq: %d" CRLF \
				     "Session: %d" CRLF \
				     CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id);
}

void rtsp_cmd_teardown(struct rtsp_context *rtsp_ctx)
{
        memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	sprintf(rtsp_ctx->response, "RTSP/1.0 200 OK" CRLF \
	                            "CSeq: %d" CRLF \
				    "Session: %d" CRLF \
				    CRLF, rtsp_ctx->CSeq, rtsp_ctx->session.id);
}

void rtsp_cmd_error(struct rtsp_context *rtsp_ctx)
{
    memset(rtsp_ctx->response, 0, RTSP_RESPONSE_BUF_SIZE);
	sprintf(rtsp_ctx->response, "RTSP/1.0 400" CRLF \
	                             "CSeq: %d" CRLF \
                                    CRLF, rtsp_ctx->CSeq);
}

void rtsp_enable_stream(struct rtsp_context *rtsp_ctx)
{
    rtsp_ctx->allow_stream = 1;
}

void rtsp_disable_stream(struct rtsp_context *rtsp_ctx)
{
    rtsp_ctx->allow_stream = 0;
}

int rtsp_is_stream_enabled(struct rtsp_context *rtsp_ctx)
{
    return rtsp_ctx->allow_stream;
}

void rtsp_enable_service(struct rtsp_context *rtsp_ctx)
{
    rtsp_ctx->is_rtsp_start = 1;
}

void rtsp_disable_service(struct rtsp_context *rtsp_ctx)
{	
    rtsp_ctx->is_rtsp_start = 0;
}

int rtsp_is_service_enabled(struct rtsp_context *rtsp_ctx)
{
    return rtsp_ctx->is_rtsp_start;
}

void rtsp_close_service(struct rtsp_context *rtsp_ctx)
{
    rtsp_disable_stream(rtsp_ctx);
    rtsp_disable_service(rtsp_ctx);
}

void show_result_statistics(struct rtsp_context *rtsp_ctx)
{
	int i = 0;
	struct stream_context *stream = NULL;
	for(i = 0; i < rtsp_ctx->nb_streams_setup; i++){
		stream = &rtsp_ctx->stream_ctx[i];
                if((stream->statistics.sent_packet == 0) && (stream->statistics.drop_packet == 0))
                  printf("\n\rch = %d sf:%d df:%d l:0%%",i,stream->statistics.sent_packet, stream->statistics.drop_packet);
                else
                  printf("\n\rch = %d sf:%d df:%d l:%d%%",i,stream->statistics.sent_packet, stream->statistics.drop_packet,\
                         (stream->statistics.drop_packet*100)/(stream->statistics.sent_packet+stream->statistics.drop_packet));
	}
}

#define WLAN0_NAME "wlan0"
void rtsp_start_service(struct rtsp_context *rtsp_ctx)
{
    u32 start_time, current_time;
    u8 *request;
	int mode = 0;
	struct sockaddr_in server_addr, client_addr;
	int client_socket;
	socklen_t client_addr_len = sizeof(struct sockaddr_in);
	
	fd_set server_read_fds, client_read_fds;
	struct timeval s_listen_timeout, c_listen_timeout;
	int ok;
	int opt = 1;
        u32 keep_alive_time = 0;

	
	request = malloc(RTSP_REQUEST_BUF_SIZE);
	if(request == NULL)
	{
	    RTSP_ERROR("\n\rallocate request buffer failed");
	    return;
	}
Redo:	
	start_time = rtw_get_current_time();
	wext_get_mode(WLAN0_NAME, &mode);
	//printf("\n\rwlan mode:%d", mode);
        while(rtsp_is_service_enabled(rtsp_ctx))
        {
            vTaskDelay(1000);
            if(!rtsp_is_service_enabled(rtsp_ctx))
            {
                    RTSP_ERROR("\n\rrtsp service disabled while waiting for wifi Tx/Rx ready...");
                    return;
            }
            current_time = rtw_get_current_time();
            if((current_time - start_time) > 60000)
            {
                    RTSP_ERROR("\n\rwifi Tx/Rx not ready...rtsp service cannot stream");
                    rtsp_disable_stream(rtsp_ctx);
                    return;
            }
            if(rltk_wlan_running(0)>0){
                wext_get_mode(WLAN0_NAME, &mode);
                if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) >= 0 && (mode == IW_MODE_INFRA)){
                  printf("connect successful sta mode\r\n");
                  break;
                }
                if(wifi_is_ready_to_transceive(RTW_AP_INTERFACE) >= 0 && (mode == IW_MODE_MASTER)){
                  printf("connect successful ap mode\r\n");
                  break;
                }
            }
        }
#if 0
	switch(mode)
	{
		case(IW_MODE_MASTER)://AP mode
			while(wifi_is_ready_to_transceive(RTW_AP_INTERFACE) < 0)
			{
					vTaskDelay(1000);
					if(!rtsp_is_service_enabled(rtsp_ctx))
					{
						RTSP_ERROR("\n\rrtsp service disabled while waiting for wifi Tx/Rx ready...");
						return;
					}
					current_time = rtw_get_current_time();
					if((current_time - start_time) > 60000)
					{
						RTSP_ERROR("\n\rwifi Tx/Rx not ready...rtsp service cannot stream");
						rtsp_disable_stream(rtsp_ctx);
						return;
					}
			}
			break;
		case(IW_MODE_INFRA)://STA mode
		case(IW_MODE_AUTO)://when ameba doesn't join bss
			while(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) < 0)
			{
					vTaskDelay(1000);
					if(!rtsp_is_service_enabled(rtsp_ctx))
					{
						RTSP_ERROR("\n\rrtsp service disabled while waiting for wifi Tx/Rx ready...");
						return;
					}
					current_time = rtw_get_current_time();
					if((current_time - start_time) > 60000)
					{
						RTSP_ERROR("\n\rwifi Tx/Rx not ready...rtsp service cannot stream");
						rtsp_disable_stream(rtsp_ctx);
						return;
					}
			}
			break;
			default:
				RTSP_ERROR("\n\rillegal wlan state!rtsp service cannot start");
				return;
	}
#endif
	wext_get_mode(WLAN0_NAME, &mode);
	//printf("\n\rwlan mode:%d", mode);	
	if(rtsp_connect_ctx_init(rtsp_ctx) < 0)
	{
		RTSP_ERROR("\n\rrtsp connect context init failed");
		return;	
	}
	if((setsockopt(rtsp_ctx->connect_ctx.socket_id, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) < 0){
		RTSP_ERROR("\r\n Error on setting socket option");
		goto error;
	}  
        server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = *(uint32_t *)(rtsp_ctx->connect_ctx.server_ip)/*_htonl(INADDR_ANY)*/;
	server_addr.sin_port = _htons(rtsp_ctx->connect_ctx.server_port);
	
	if(bind(rtsp_ctx->connect_ctx.socket_id, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		RTSP_ERROR("\n\rCannot bind stream socket");
		goto error;	    
	}
	listen(rtsp_ctx->connect_ctx.socket_id, 1);
	printf("\n\rrtsp stream enabled");

	//enter service loop
	while(rtsp_is_stream_enabled(rtsp_ctx))
	{
	    FD_ZERO(&server_read_fds);
		s_listen_timeout.tv_sec = 1;
		s_listen_timeout.tv_usec = 0;
		FD_SET(rtsp_ctx->connect_ctx.socket_id, &server_read_fds);
		if(select(RTSP_SELECT_SOCK, &server_read_fds, NULL, NULL, &s_listen_timeout))
		{
		    client_socket = accept(rtsp_ctx->connect_ctx.socket_id, (struct sockaddr*)&client_addr, &client_addr_len);
			if(client_socket < 0)
			{
			    RTSP_ERROR("\n\rclient_socket error");
				close(client_socket);
				continue;
			}
			*(rtsp_ctx->connect_ctx.remote_ip + 3) = (unsigned char) (client_addr.sin_addr.s_addr >> 24);
			*(rtsp_ctx->connect_ctx.remote_ip + 2) = (unsigned char) (client_addr.sin_addr.s_addr >> 16);
			*(rtsp_ctx->connect_ctx.remote_ip + 1) = (unsigned char) (client_addr.sin_addr.s_addr >> 8);
			*(rtsp_ctx->connect_ctx.remote_ip) = (unsigned char) (client_addr.sin_addr.s_addr );
            while(rtsp_is_stream_enabled(rtsp_ctx))	
            {
                                FD_ZERO(&client_read_fds);
				c_listen_timeout.tv_sec = 0;
				c_listen_timeout.tv_usec = 10000;
				FD_SET(client_socket, &client_read_fds);
#ifdef KEEPALIVE_TIMEOUT_ENABLE
                                if(keep_alive_time !=0 ){
                                  if(rtw_get_current_time()-keep_alive_time>70000){
                                      printf("keepalive timeout\n\r");
                                      goto out;
                                  }
                                }
#endif
				if(select(RTSP_SELECT_SOCK, &client_read_fds, NULL, NULL, &c_listen_timeout))
				{
				    memset(request, 0, RTSP_REQUEST_BUF_SIZE);
					read(client_socket, request, RTSP_REQUEST_BUF_SIZE);
					rtsp_parse_request(rtsp_ctx, request);
					switch(rtsp_ctx->request_type)
					{
					        case(REQUEST_OPTIONS):
						    RTSP_PRINTF("\n\rReceive options request!");
							rtsp_cmd_options(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend OPTIONS response failed!");
								goto out;
							}
							break;
						case(REQUEST_DESCRIBE):
						    RTSP_PRINTF("\n\rReceive describe request!");
                                                        if(rtsp_ctx->state != RTSP_INIT)
                                                            goto out;
							rtsp_cmd_describe(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend DESCRIBE response failed!");
								goto out;
							}
							break;
                                                 case(REQUEST_GET_PARAM):
                                                        RTSP_PRINTF("REQUEST_GET_PARAM\n");
                                                        //printf("REQUEST_GET_PARAM\n");
							rtsp_cmd_getparm(rtsp_ctx);
                                                        keep_alive_time = rtw_get_current_time();
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend REQUEST_GET_PARAM response failed!");
								goto out;
							}
                                                        show_result_statistics(rtsp_ctx);
                                                        if(rtsp_ctx->cb_custom)
                                                            rtsp_ctx->cb_custom(NULL);
							break;
						case(REQUEST_SETUP):
						    RTSP_PRINTF("\n\rReceive setup request!");
                                                        if((rtsp_ctx->state != RTSP_INIT) && (rtsp_ctx->nb_streams_setup >= rtsp_ctx->nb_streams))
                                                            goto out;
							rtsp_cmd_setup(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend SETUP response failed!");
								goto out;
							}
							rtsp_ctx->state = RTSP_READY;
                                                        rtsp_ctx->nb_streams_setup++;
							RTSP_PRINTF("\n\rstate changed from RTSP_INIT to RTSP_READY");
							break;
						case(REQUEST_TEARDOWN):
							//printf("\r\nTEARDOWN%d, %s", rtsp_ctx->nb_streams_setup, request);
						    RTSP_PRINTF("\n\rReceive teardown command!");
							char * pch=strstr(request,"stream");
							if(pch!=NULL){
								rtsp_ctx->nb_streams_setup--;
								rtsp_cmd_teardown(rtsp_ctx);
								ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
								if(ok <= 0){
									RTSP_ERROR("\n\rsend TEARDOWN response failed!");
									goto out;							
								}
								break;
							}
							rtsp_ctx->state = RTSP_INIT;
							rtsp_cmd_teardown(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend TEARDOWN response failed!");
								goto out;							
							}
							RTSP_PRINTF("\n\rstreaming teardown, state changed back to RTSP_INIT");
                                                        show_result_statistics(rtsp_ctx);
							/*wait for rtp/rtcp server reinit*/
                                                        if(rtsp_ctx->cb_stop)
                                                            rtsp_ctx->cb_stop(NULL);
							vTaskDelay(1000);
							goto out;
							break;
						case(REQUEST_PLAY):
						    RTSP_PRINTF("\n\rReceive play command!");
							if(rtsp_ctx->state != RTSP_READY)
							    break;                                                        
							rtsp_cmd_play(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if(ok <= 0)
							{
							    RTSP_ERROR("\n\rsend PLAY response failed!");
								goto out;
							}
							rtsp_ctx->state = RTSP_PLAYING;
							RTSP_PRINTF("\n\rstate changed from RTSP_READY to RTSP_PLAYING");
							//here to start rtp/rtcp service
							rtw_up_sema(&rtsp_ctx->start_rtp_sema);
#ifdef SUPPORT_RTCP
							rtw_up_sema(&rtsp_ctx->start_rtcp_sema);
#endif		
                                                        if(rtsp_ctx->cb_start)
                                                            rtsp_ctx->cb_start(NULL);
							break;
						case(REQUEST_PAUSE):
						    RTSP_PRINTF("\n\rReceive pause command!");
							if(rtsp_ctx->state != RTSP_PLAYING)
                                                            goto out;
							rtsp_cmd_pause(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if (ok <= 0)
							{
									RTSP_ERROR("\n\rsend PAUSE response failed!");
									goto out;
							}	
							rtsp_ctx->state = RTSP_READY;
                                                        if(rtsp_ctx->cb_pause)
                                                            rtsp_ctx->cb_pause(NULL);
							RTSP_PRINTF("\n\rstate changed from RTSP_PLAYING to RTSP_READY");
							break;
						//case(GET_PARAMETER):
						//	break;
						default:
						        RTSP_ERROR("\n\rReceive invalid command!");
								RTSP_ERROR("\n\rREQUEST: %s",request);
							rtsp_ctx->state = RTSP_INIT;
							rtsp_cmd_error(rtsp_ctx);
							ok = write(client_socket, rtsp_ctx->response, strlen(rtsp_ctx->response));
							if (ok <= 0)
							{
									RTSP_ERROR("\n\rsend ERROR response failed!");
									//goto exit;
									goto out;
							}							
					}
				}
                                if(mode == IW_MODE_INFRA){
                                    if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) < 0)
                                            goto out;
                                }else if(mode == IW_MODE_MASTER){
                                    if(wifi_is_ready_to_transceive(RTW_AP_INTERFACE) < 0)
                                          goto out;
                                }else{
                                    goto out;
                                }
			}
out:
            rtsp_ctx->state = RTSP_INIT;
            keep_alive_time = 0;
            close(client_socket);			
		}
                if(mode == IW_MODE_INFRA)
                {
                    if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) < 0)
                    {
                            RTSP_ERROR("\n\rwifi Tx/Rx broke! rtsp service cannot stream");

    						close(rtsp_ctx->connect_ctx.socket_id);
							printf("\r\nRTSP Reconnect!");
                            goto Redo;
                    }
                }else if(mode == IW_MODE_MASTER){
                    if(wifi_is_ready_to_transceive(RTW_AP_INTERFACE) < 0)
                    {
                            RTSP_ERROR("\n\rwifi Tx/Rx broke! rtsp service cannot stream");

    						close(rtsp_ctx->connect_ctx.socket_id);
							printf("\r\nRTSP Reconnect!");
                            goto Redo;
                    }
                }else{
                    goto error;
                }
            //refresh number of streams been set up
            rtsp_ctx->nb_streams_setup = 0;
	}
error:    
    rtsp_disable_stream(rtsp_ctx);
    close(rtsp_ctx->connect_ctx.socket_id);
    free(request);
	printf("\n\rrtsp service stop");
}

static void rtp_service_udp_unicast(struct rtsp_context *rtsp_ctx)
{
	struct rtp_object *payload;
	struct stream_context *stream;
	int i, ret, stream_count;
	struct sockaddr_in rtp_addr;                
	int rtp_socket;                       
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int rtp_port; 
        unsigned int filter_count = 0;
	        /*init rtp socket*/
	rtp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	rtp_port = rtsp_ctx->transport[0].serverport_low;
	memset(&rtp_addr, 0, addrlen);
	rtp_addr.sin_family = AF_INET;
	rtp_addr.sin_addr.s_addr = *(uint32_t *)(rtsp_ctx->connect_ctx.server_ip);
	rtp_addr.sin_port = _htons((u16)rtp_port);                
	if (bind(rtp_socket,(struct sockaddr *)&rtp_addr, addrlen)<0) {
                RTSP_ERROR("bind failed\r\n");
                goto exit;
	}
        rtsp_ctx->is_rtp_start = 1;  
        printf("\n\rrtp started...");	
restart:
	while((rtsp_ctx->state == RTSP_PLAYING)&&(rtsp_is_stream_enabled(rtsp_ctx)))
	{
        for(i = 0; i < rtsp_ctx->nb_streams_setup; i++)  
		{
            stream = &rtsp_ctx->stream_ctx[i];
		    if(stream->stream_id >= 0)
            {
					if(!list_empty(&stream->input_queue))
					{
						rtw_mutex_get(&stream->input_lock);
						payload = list_first_entry(&stream->input_queue, struct rtp_object, rtp_list);
						if(payload == NULL)
						{
							rtw_mutex_put(&stream->input_lock);
								goto exit;
						}
						list_del_init(&payload->rtp_list);					
						rtw_mutex_put(&stream->input_lock);
						if(payload->state == RTP_OBJECT_READY)
						{
							payload->state = RTP_OBJECT_INUSE;
							payload->connect_ctx.socket_id = rtp_socket;
							payload->connect_ctx.remote_port = (u16)rtsp_ctx->transport[i].clientport_low;
							payload->connect_ctx.server_ip = rtsp_ctx->connect_ctx.server_ip;
							payload->connect_ctx.remote_ip = rtsp_ctx->connect_ctx.remote_ip;       
							//ret = payload->rtp_object_handler(stream, payload);
                                                        if(filter_count <= (rtsp_ctx->pre_filter_packet+80))//end condition
                                                          filter_count++;
                                                        if(filter_count>=rtsp_ctx->pre_filter_packet){
                                                          ret = payload->rtp_object_handler(stream, payload); 
                                                        }else{
                                                          ret = 0;//printf("filter_count = %d\r\n",filter_count);
                                                        }
							if(ret < 0){
								if(ret == -EAGAIN)//means packet drop
									stream->statistics.drop_packet++;
								else
									goto exit;
							}else{
								if(stream->statistics.base_timestamp == 0)
								{
									stream->statistics.base_timestamp = payload->timestamp;
									RTSP_PRINTF("\n\rbase ts:%d", stream->statistics.base_timestamp);
								}
								stream->statistics.sent_packet++;
								//if(rtw_get_current_time()%10000 < 10)
								//	RTSP_ERROR("\n\rts:%d sf:%d df:%d", payload->timestamp, stream->statistics.sent_packet, stream->statistics.drop_packet);
							}
							payload->state = RTP_OBJECT_USED;
						}else{
							payload->state = RTP_OBJECT_IDLE;
						}
						rtw_mutex_get(&stream->output_lock);
						list_add_tail(&payload->rtp_list, &stream->output_queue);
						rtw_mutex_put(&stream->output_lock);
					}
            }
		}		
        //vTaskDelay(1);
	}
	vTaskDelay(1000);
	if(rtsp_ctx->state == RTSP_READY)
	{
                filter_count = 0;//for pause
		goto restart;
	}
exit:
	rtsp_ctx->is_rtp_start = 0;
	close(rtp_socket);
	printf("\n\rrtp stopped");
}

void rtp_service_init(void *param)
{
    struct rtsp_context *rtsp_ctx = (struct rtsp_context *) param;
    int i;
    struct stream_context *stream;
    while(rtsp_is_service_enabled(rtsp_ctx))
    {
        if(rtw_down_timeout_sema(&rtsp_ctx->start_rtp_sema,100))
        {
            for(i = 0; i < rtsp_ctx->nb_streams_setup; i++)  
            {
                stream = &rtsp_ctx->stream_ctx[i];
                if(stream->stream_id >= 0)
                { 
                        rtp_stream_statistics_sync(stream);                       
                }
            }                      
            switch(rtsp_ctx->transport[0].castMode)
            {
                case(UNICAST_UDP_MODE):
                        rtsp_ctx->rtp_service_handle = rtp_service_udp_unicast;
                        rtsp_ctx->rtp_service_handle(rtsp_ctx);
                        break;
                case(MULTICAST_MODE):
                        //to do
                        break;
                case(UNICAST_TCP_MODE):
                        //to do				
                        break;
                default:
                        break;
            }	
        }
    }
    rtsp_ctx->is_rtp_start = 0;
    vTaskDelete(NULL);
}

#ifdef SUPPORT_RTCP
void rtcp_service_init(void *param)
{
    struct rtsp_context *rtsp_ctx = (struct rtsp_context *) param;
    rtsp_ctx->is_rtcp_start = 1;
    while(rtsp_is_service_enabled(rtsp_ctx))
    {
        if(rtw_down_timeout_sema(&rtsp_ctx->start_rtp_sema,10))
        {
                //to do
        }
    }
    vTaskDelete(NULL);
}
#endif

void rtsp_service_init(void *param)
{
    struct rtsp_context *rtsp_ctx = (struct rtsp_context *) param;
    rtsp_enable_service(rtsp_ctx);
    while(rtsp_is_service_enabled(rtsp_ctx))
    {
            if(rtw_down_timeout_sema(&rtsp_ctx->start_rtsp_sema,100))
            {
                //rtsp start stream
                rtsp_start_service(rtsp_ctx);
            }
    }	
    printf("\n\rrtsp service closed");
    vTaskDelete(NULL);
}

/* ----------------------------------------------- user space interface ---------------------------------------------------------*/
int rtsp_open(struct rtsp_context *rtsp_ctx)
{
	if(xTaskCreate(rtsp_service_init, ((const signed char*)"rtsp_service_init"), 2048, (void *)rtsp_ctx, RTSP_SERVICE_PRIORITY, NULL) != pdPASS) {
		  RTSP_ERROR("\r\n rtp_start_service: Create Task Error\n");
		  goto error;
	}
	
	//rtsp_service_priority = uxTaskPriorityGet(NULL);
	if(xTaskCreate(rtp_service_init, ((const signed char*)"rtp_service_init"), 1024, (void *)rtsp_ctx, RTSP_SERVICE_PRIORITY-1, NULL) != pdPASS) {
		  RTSP_ERROR("\r\n rtp_start_service: Create Task Error\n");
		  goto error;
	}
#ifdef SUPPORT_RTCP
	if(xTaskCreate(rtcp_service_init, ((const signed char*)"rtcp_service_init"), 512, (void *)rtsp_ctx, RTSP_SERVICE_PRIORITY, NULL) != pdPASS) {
		  RTSP_ERROR("\r\n rtp_start_service: Create Task Error\n");
		  goto error;
	}
#endif		
	return 0;
error:
	rtsp_close_service(rtsp_ctx);
	return -1;    
}

void rtsp_start(struct rtsp_context *rtsp_ctx)
{
	//check if more than 1 src stream registered in rtsp context 
    int i;	
    for(i = 0; i < rtsp_ctx->nb_streams; i++)
    {    
        INIT_LIST_HEAD(&rtsp_ctx->stream_ctx[i].input_queue);
        INIT_LIST_HEAD(&rtsp_ctx->stream_ctx[i].output_queue);
    }
    rtsp_ctx->state = RTSP_INIT;
    rtsp_ctx->nb_streams_setup = 0;
    rtsp_enable_stream(rtsp_ctx);
    rtw_up_sema(&rtsp_ctx->start_rtsp_sema);
}

void rtsp_stop(struct rtsp_context *rtsp_ctx)
{
    rtsp_disable_stream(rtsp_ctx);
}

void rtsp_close(struct rtsp_context *rtsp_ctx)
{
    rtsp_close_service(rtsp_ctx);
}

void rtp_object_in_stream_queue(struct rtp_object *payload, struct stream_context *stream_ctx)
{
    rtw_mutex_get(&stream_ctx->input_lock);
    list_add_tail(&payload->rtp_list, &stream_ctx->input_queue);
    rtw_mutex_put(&stream_ctx->input_lock);
}

struct rtp_object *rtp_object_out_stream_queue(struct stream_context *stream_ctx)
{
	struct rtp_object *payload = NULL;
	if(!list_empty(&stream_ctx->output_queue))
	{
		rtw_mutex_get(&stream_ctx->output_lock);
		payload = list_first_entry(&stream_ctx->output_queue, struct rtp_object, rtp_list);
		list_del_init(&payload->rtp_list);
		rtw_mutex_put(&stream_ctx->output_lock);
	}
	return payload;
}