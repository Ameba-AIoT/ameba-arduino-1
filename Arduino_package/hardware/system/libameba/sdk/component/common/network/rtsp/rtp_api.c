
#include "rtp_api.h"

void rtp_object_init(struct rtp_object *payload)
{
    memset(payload, 0, sizeof(struct rtp_object));
    INIT_LIST_HEAD(&payload->rtp_list);
    rtw_mutex_init(&payload->list_lock);
    payload->state = RTP_OBJECT_IDLE;    
}

void rtp_object_deinit(struct rtp_object *payload)
{
    if(payload->rtphdr != NULL)
            free(payload->rtphdr);
    if(payload->extra != NULL)
            free(payload->extra);
    memset(payload, 0, sizeof(struct rtp_object));
    INIT_LIST_HEAD(&payload->rtp_list);
    rtw_mutex_free(&payload->list_lock);
    payload->state = RTP_OBJECT_IDLE;    
}

void rtp_object_set_fs(struct rtp_object *payload, int flag)
{
    if(flag>0)
    {
        payload->fs = 1;
    }else{
        payload->fs = 0;
    }
}

void rtp_object_set_fe(struct rtp_object *payload, int flag)
{
    if(flag>0)
    {
        payload->fe = 1;
    }else{
        payload->fe = 0;
    }
}
    
void rtp_object_set_fk(struct rtp_object *payload, int flag)
{
    if(flag>0)
    {
       payload->fk = 1;
    }else{
       payload->fk = 0;
    }
}

void rtp_object_set_fd(struct rtp_object *payload, int size)
{
    if(size > 0)
    {
       payload->fd = size;
    }else{
       payload->fd = 0;
    }
}

void rtp_fill_header(rtp_hdr_t *rtphdr, int version, int padding, int extension, int cc, int marker, int pt, u16 seq, u32 ts, u32 ssrc)
{
        int i;
	rtphdr->version = version;
        rtphdr->p = padding;
        rtphdr->x = extension;
        rtphdr->cc = cc;
        rtphdr->m = marker;
        rtphdr->pt = pt;
        rtphdr->seq = _htons(seq);
        rtphdr->ts = _htonl(ts);
        rtphdr->ssrc = _htonl(ssrc);
/* if we mix more than 1 sources in this packet fill csrc*/
        if(cc > 0)
        {
            for(i = 0; (i < cc)||(i < 16); i++)
            {
                rtphdr->csrc[i] = i;
            }
        }
}

//parse rtp general header
int rtp_parse_header(u8 *src, rtp_hdr_t *rtphdr, int is_nbo)
{
        u8 *ptr = src;
        int offset = 0;
/*
#if RTP_BIG_ENDIAN
        u16 version:2;   //protocol version
        u16 p:1;         //padding flag
        u16 x:1;         //header extension flag
        u16 cc:4;        //CSRC count
        u16 m:1;         //marker bit
        u16 pt:7;        //payload type
#else //RTP_LITTLE_ENDIAN//
        u16 cc:4;        //CSRC count 
        u16 x:1;         //header extension flag
        u16 p:1;         //padding flag 
        u16 version:2;   //protocol version
        u16 pt:7;        //payload type 
        u16 m:1;          //marker bit 
#endif
        u16 seq;             //sequence number 
        u32 ts;                //timestamp 
        u32 ssrc;              //synchronization source 
        u32 *csrc;           //optional CSRC list, skip if cc is set to 0 here  
*/
        if(is_nbo)
        {
            rtphdr->cc = *ptr & 0x0f;
            rtphdr->x = (*ptr & 0x10)>>4;
            rtphdr->p = (*ptr & 0x20)>>5;
            rtphdr->version = (*ptr & 0xc0)>>6;
            ptr++;
            rtphdr->pt = *ptr & 0x7f;
            rtphdr->m = *ptr>>7;            
        }
        else
        {
            rtphdr->version = *ptr & 0x03;
            rtphdr->p = (*ptr & 0x04)>>2;
            rtphdr->x = (*ptr & 0x08)>>3;
            rtphdr->cc = (*ptr & 0xf0)>>4;
            ptr++;
            rtphdr->m = *ptr & 0x01;
            rtphdr->pt = *ptr>>1;
        }
        ptr++;
        if(is_nbo)
            rtphdr->seq = *(u16 *)ptr;
        else
            rtphdr->seq = ntohs(*(u16 *)ptr);
        ptr += 2;
        if(is_nbo)
            rtphdr->ts = *(u32 *)ptr;
        else
            rtphdr->ts = ntohl(*(u32 *)ptr);
        ptr += 4;
        if(is_nbo)
            rtphdr->ssrc = *(u32 *)ptr;
        else
            rtphdr->ssrc = ntohl(*(u32 *)ptr);
        ptr += 4;
        offset = 12;
        if(rtphdr->cc > 0)
        {
          offset += rtphdr->cc * 4;
          //to do parse csrc
        }
        return offset;
}

extern int rtp_o_mjpeg_handler(struct stream_context *stream_ctx, struct rtp_object *payload);
extern int rtp_o_h264_handler(struct stream_context *stream_ctx, struct rtp_object *payload);
extern int rtp_o_g711_handler(struct stream_context *stream_ctx, struct rtp_object *payload);
extern int rtp_o_aac_handler(struct stream_context *stream_ctx, struct rtp_object *payload);
void rtp_load_o_handler_by_codec_id(struct rtp_object *payload, int id)
{
        switch(id)
        {
                case(AV_CODEC_ID_MJPEG):
                    payload->rtp_object_handler = rtp_o_mjpeg_handler;
                    return;
                case(AV_CODEC_ID_H264):
                    payload->rtp_object_handler = rtp_o_h264_handler;
                    return;
                case (AV_CODEC_ID_PCMU):
                case (AV_CODEC_ID_PCMA):
                    payload->rtp_object_handler = rtp_o_g711_handler;
                    return;
                case(AV_CODEC_ID_MP4A_LATM):
                    payload->rtp_object_handler = rtp_o_aac_handler;
                    return;
                default:
                    return;
        }
}

void rtp_dump_header(rtp_hdr_t *rtphdr, int is_nbo)
{
        printf("\n\rrtp header info:");
        printf("\n\rrtp version:%d", rtphdr->version);
        printf("\n\rrtp padding flag p:%d", rtphdr->p);
        printf("\n\rrtp header extension flag x:%d", rtphdr->x);
        printf("\n\rrtp CSRC count cc:%d", rtphdr->cc);
        printf("\n\rrtp marker bit m:%d", rtphdr->m);
        printf("\n\rrtp pt:%d", rtphdr->pt);
        printf("\n\rrtp seq:%d", is_nbo ? ntohs(rtphdr->seq): rtphdr->seq);
        printf("\n\rrtp timestamp ts:%d", is_nbo? ntohl(rtphdr->ts):rtphdr->ts);
        printf("\n\rrtp synchronization source ssrc:%d", is_nbo?ntohl(rtphdr->ssrc):rtphdr->ssrc);
}
