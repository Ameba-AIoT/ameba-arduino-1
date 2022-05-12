/******************************************************************************
 *
 * Copyright(c) 2007 - 2014 Realtek Corporation. All rights reserved.
 *                                        
 *
 *
 ******************************************************************************/
#include <drv_types.h>
#include "rom_arc4.h"

static void i_P_SHA1(
	unsigned char*  key,		// pointer to authentication key
	int             key_len,	// length of authentication key
	unsigned char*  text,		// pointer to data stream
	int             text_len,	// length of data stream
	unsigned char*  digest,		// caller digest to be filled in
	int				digest_len	// in byte
	)
{
	int i;
	int offset = 0;
	int step = 20;
	int IterationNum = (digest_len + step - 1) / step;

	for(i = 0; i < IterationNum; i ++)
	{
		text[text_len] = (unsigned char) i;
		rt_hmac_sha1(text, text_len + 1, key,key_len, digest + offset);
		offset += step;
	}
}

static void i_PRF(
	unsigned char*	secret,
	int		secret_len,
	unsigned char*	prefix,
	int		prefix_len,
	unsigned char*	random,
	int		random_len,
	unsigned char*  digest,		// caller digest to be filled in
	int		digest_len			// in byte
	)
{
	unsigned char data[100];

	//99 bytes for CalcPTK, 58 bytes for CalcGTK
	if((prefix_len + 1 + random_len) > 100)
		return;

	memcpy(data, prefix, prefix_len);
	data[prefix_len ++] = 0;
	memcpy(data + prefix_len, random, random_len);
	i_P_SHA1(secret, secret_len, data, prefix_len + random_len, digest, digest_len);
}

static const unsigned char gmk_expansion_const[] = GMK_EXPANSION_CONST;

void ram_patch_rom_psk_CalcGTK(unsigned int AuthKeyMgmt,
				unsigned char *addr, unsigned char *nonce,
				unsigned char *keyin, int keyinlen,
				unsigned char *keyout, int keyoutlen)
{
	unsigned char data[ETHER_ADDRLEN + KEY_NONCE_LEN], tmp[64];
	unsigned char *label;

	memcpy(data, addr, ETHER_ADDRLEN);
	memcpy(data + ETHER_ADDRLEN, nonce, KEY_NONCE_LEN);

#if defined(CONFIG_IEEE80211W) || defined(CONFIG_SAE_SUPPORT)
	if(wpa_key_mgmt_sha256(AuthKeyMgmt) || wpa_key_mgmt_sae(AuthKeyMgmt))
	{
		label = (unsigned char*) igtk_expansion_const;
		sha256_prf(keyin, keyinlen, label,
				data, sizeof(data), tmp, keyoutlen);
	}
	else// if(AuthKeyMgmt == WPA_KEY_MGMT_PSK)
#endif
	{
		label = (unsigned char*) gmk_expansion_const;
		i_PRF(	keyin, keyinlen, label ,
			GMK_EXPANSION_CONST_SIZE, data, sizeof(data),
			tmp, keyoutlen);
	}

	memcpy(keyout, tmp, keyoutlen);
}

int ram_patch_rom_CheckMIC(OCTET_STRING EAPOLMsgRecvd, unsigned char *key, int keylen)
{
	int retVal = 0;
	OCTET_STRING EapolKeyMsgRecvd;
	unsigned char ucAlgo;
	OCTET_STRING tmp; //copy of overall 802.1x message
	unsigned char tmpbuf[512];
	struct lib1x_eapol *tmpeapol;
	lib1x_eapol_key *tmpeapolkey;
	unsigned char sha1digest[20];
	u16 eapol_body_len, mic_compute_len;

	EapolKeyMsgRecvd.Octet = EAPOLMsgRecvd.Octet +
					ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	EapolKeyMsgRecvd.Length = EAPOLMsgRecvd.Length -
					(ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	ucAlgo = Message_KeyDescVer(EapolKeyMsgRecvd);

	tmp.Length = EAPOLMsgRecvd.Length;
	tmp.Octet = tmpbuf;
	_memcpy(tmp.Octet, EAPOLMsgRecvd.Octet, EAPOLMsgRecvd.Length);
	tmpeapol = (struct lib1x_eapol *)(tmp.Octet + ETHER_HDRLEN);
	tmpeapolkey = (lib1x_eapol_key *)(tmp.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	_memset(tmpeapolkey->key_mic, 0, KEY_MIC_LEN);

	// compute MIC from protocol ver to key data = protocol ver[1]+packet type[1] + packet body len[2] + packet body[packet body len]
	eapol_body_len = _ntohs_rom(*((u16 *)(((u8 *)tmpeapol) + 2)));
	mic_compute_len = eapol_body_len + 4;

	if (ucAlgo == key_desc_ver1) {
#if 1//PSK_SUPPORT_TKIP
		rt_md5_hmac((unsigned char*)tmpeapol, mic_compute_len /* EAPOLMsgRecvd.Length - ETHER_HDRLEN */,
					key, keylen, tmpeapolkey->key_mic);

		if (!_memcmp(tmpeapolkey->key_mic, EapolKeyMsgRecvd.Octet + KeyMICPos, KEY_MIC_LEN))
			retVal = 1;
#else
		DBG_871X("%s Not PSK_SUPPORT_TKIP\n", __FUNCTION__);
#endif
	}
	else if (ucAlgo == key_desc_ver2) {
		rt_hmac_sha1((unsigned char*)tmpeapol, mic_compute_len /* EAPOLMsgRecvd.Length - ETHER_HDRLEN */, key, keylen, sha1digest);
		if (!_memcmp(sha1digest, EapolKeyMsgRecvd.Octet + KeyMICPos, KEY_MIC_LEN))
			retVal = 1;
	}
#if defined(CONFIG_IEEE80211W) || defined(CONFIG_SAE_SUPPORT)
	else{ //if (algo == key_desc_ver0 || algo == key_desc_ver3)
		omac1_aes_128(key, (unsigned char*)tmpeapol, 
					mic_compute_len, tmpeapolkey->key_mic);
		
		if (!memcmp(tmpeapolkey->key_mic, EapolKeyMsgRecvd.Octet + KeyMICPos, KEY_MIC_LEN))
			retVal = 1;
	}
#endif
	return retVal;
}

void ram_patch_rom_CalcMIC(OCTET_STRING EAPOLMsgSend, int algo, unsigned char *key, int keylen)
{
	struct lib1x_eapol *eapol = (struct lib1x_eapol *)(EAPOLMsgSend.Octet+ETHER_HDRLEN);
	lib1x_eapol_key *eapolkey = (lib1x_eapol_key *)(EAPOLMsgSend.Octet+ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
	unsigned char sha1digest[20];

	_memset(eapolkey->key_mic, 0, KEY_MIC_LEN);

	if (algo == key_desc_ver1) {
#if 1//PSK_SUPPORT_TKIP
  		rt_md5_hmac((unsigned char*)eapol, EAPOLMsgSend.Length - ETHER_HDRLEN ,
					key, keylen, eapolkey->key_mic);
#else
		DBG_871X("%s Not PSK_SUPPORT_TKIP\n", __FUNCTION__);
#endif
	}
	else if (algo == key_desc_ver2) {
		rt_hmac_sha1((unsigned char*)eapol, EAPOLMsgSend.Length - ETHER_HDRLEN ,
					key, keylen, sha1digest);
		_memcpy(eapolkey->key_mic, sha1digest, KEY_MIC_LEN);
	}
#if defined(CONFIG_IEEE80211W) || defined(CONFIG_SAE_SUPPORT)
	else{ //if (algo == key_desc_ver0 || algo == key_desc_ver3)
		omac1_aes_128(key, (unsigned char*)eapol, 
					EAPOLMsgSend.Length - ETHER_HDRLEN, eapolkey->key_mic);
	}
#endif
}

int ram_patch_rom_DecGTK(OCTET_STRING EAPOLMsgRecvd, unsigned char *kek, int keklen, int keylen, unsigned char *kout)
{
	int				retVal = 0;
	unsigned char		default_key_iv[] = { 0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6 };
	unsigned char		tmp2[257];
#if 1//PSK_SUPPORT_TKIP
	unsigned char		tmp1[257];
	struct arc4context	rc4_ctx;
	lib1x_eapol_key	*eapol_key	= (lib1x_eapol_key *)(EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);
#endif
	OCTET_STRING EapolKeyMsgRecvd;
	EapolKeyMsgRecvd.Octet = EAPOLMsgRecvd.Octet + ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN;
	EapolKeyMsgRecvd.Length = EAPOLMsgRecvd.Length - (ETHER_HDRLEN + LIB1X_EAPOL_HDRLEN);

	if(Message_KeyDescVer(EapolKeyMsgRecvd) == key_desc_ver1)
	{
#if 1//PSK_SUPPORT_TKIP
		_memcpy(tmp1, eapol_key->key_iv, KEY_IV_LEN);
		_memcpy(tmp1 + KEY_IV_LEN, kek, keklen);

		rt_arc4_init(&rc4_ctx, tmp1, KEY_IV_LEN + keklen);
		//first 256 bits is discard
		rt_arc4_crypt(&rc4_ctx, tmp2, tmp1, 256);
		rt_arc4_crypt(&rc4_ctx, (unsigned char*)tmp2, EapolKeyMsgRecvd.Octet + KeyDataPos, keylen);
		
		_memcpy(kout, tmp2, keylen);
		retVal = 1;
#else
		DBG_871X("%s Not PSK_SUPPORT_TKIP\n", __FUNCTION__);
#endif
	}	
	else// if(Message_KeyDescVer(EapolKeyMsgRecvd) == key_desc_ver2)//CONFIG_IEEE80211W, key_desc_ver2 and key_desc_ver3 both work here
	{

		keylen = Message_KeyDataLength(EapolKeyMsgRecvd);
		AES_UnWRAP(EapolKeyMsgRecvd.Octet + KeyDataPos, keylen, kek, keklen, tmp2);

		if(_memcmp(tmp2, default_key_iv, 8))
			retVal = 0;
		else
		{
			_memcpy(kout, tmp2+8, keylen);
			retVal = 1;
		}
	}
	return retVal;
}

static int Min(unsigned char *ucStr1, unsigned char *ucStr2, unsigned long ulLen)
{
	int i;
	for (i=0 ; i<ulLen ; i++) {
		if ((unsigned char)ucStr1[i] < (unsigned char)ucStr2[i])
			return -1;
		else if((unsigned char)ucStr1[i] > (unsigned char)ucStr2[i])
			return 1;
		else if(i == ulLen - 1)
  			return 0;
		else
			continue;
	}
	return 0;
}

static const unsigned char pmk_expansion_const[] = PMK_EXPANSION_CONST;

void ram_patch_rom_psk_CalcPTK(unsigned int AuthKeyMgmt,
			unsigned char *addr1, unsigned char *addr2,
			unsigned char *nonce1, unsigned char *nonce2,
			unsigned char *keyin, int keyinlen,
			unsigned char *keyout, int keyoutlen)
{
	unsigned char data[2*ETHER_ADDRLEN+ 2*KEY_NONCE_LEN], tmpPTK[128];

	if(Min(addr1, addr2, ETHER_ADDRLEN) <= 0)
	{
		memcpy(data, addr1, ETHER_ADDRLEN);
		memcpy(data + ETHER_ADDRLEN, addr2, ETHER_ADDRLEN);
	}else
	{
		memcpy(data, addr2, ETHER_ADDRLEN);
		memcpy(data + ETHER_ADDRLEN, addr1, ETHER_ADDRLEN);
	}
	if(Min(nonce1, nonce2, KEY_NONCE_LEN) <= 0)
	{
		memcpy(data + 2*ETHER_ADDRLEN, nonce1, KEY_NONCE_LEN);
		memcpy(data + 2*ETHER_ADDRLEN + KEY_NONCE_LEN, nonce2, KEY_NONCE_LEN);
	}else
	{
		memcpy(data + 2*ETHER_ADDRLEN, nonce2, KEY_NONCE_LEN);
		memcpy(data + 2*ETHER_ADDRLEN + KEY_NONCE_LEN, nonce1, KEY_NONCE_LEN);
	}

#if defined(CONFIG_IEEE80211W) || defined(CONFIG_SAE_SUPPORT)
	if(wpa_key_mgmt_sha256(AuthKeyMgmt) || wpa_key_mgmt_sae(AuthKeyMgmt))
	{
		sha256_prf(keyin, keyinlen, (unsigned char*) pmk_expansion_const,
				data, sizeof(data), tmpPTK, PTK_LEN_CCMP);
	}
	else //if(AuthKeyMgmt == WPA_KEY_MGMT_PSK)
#endif
	{
		i_PRF(keyin, keyinlen, (unsigned char*) pmk_expansion_const,
			PMK_EXPANSION_CONST_SIZE, data, sizeof(data),
			tmpPTK, PTK_LEN_TKIP);
	}

	memcpy(keyout, tmpPTK, keyoutlen);
}


/************************************************/
/* aesccmp_construct_mic_iv()                           */
/* Builds the MIC IV from header fields and PN  */
/************************************************/
void aesccmp_construct_mic_iv_ram(
	u8 *mic_iv, sint qc_exists, sint a4_exists, 
	u8 *mpdu, uint payload_length,u8 *pn_vector,
	uint frtype/* add for CONFIG_IEEE80211W, none 11w also can use */
	)
{
	sint i;

	mic_iv[0] = 0x59;
	if (qc_exists && a4_exists) mic_iv[1] = mpdu[30] & 0x0f;    /* QoS_TC           */
	if (qc_exists && !a4_exists) mic_iv[1] = mpdu[24] & 0x0f;   /* mute bits 7-4    */
	if (!qc_exists) mic_iv[1] = 0x00;
#ifdef CONFIG_IEEE80211W
	/* 802.11w management frame should set management bit(4) */
	if (frtype == WIFI_MGT_TYPE) mic_iv[1] |= BIT(4);
#endif /* CONFIG_IEEE80211W */
	for (i = 2; i < 8; i++)
	    mic_iv[i] = mpdu[i + 8];                    /* mic_iv[2:7] = A2[0:5] = mpdu[10:15] */
#ifdef CONSISTENT_PN_ORDER
	    for (i = 8; i < 14; i++)
	        mic_iv[i] = pn_vector[i - 8];           /* mic_iv[8:13] = PN[0:5] */
#else
	    for (i = 8; i < 14; i++)
	        mic_iv[i] = pn_vector[13 - i];          /* mic_iv[8:13] = PN[5:0] */
#endif
	mic_iv[14] = (unsigned char) (payload_length / 256);
	mic_iv[15] = (unsigned char) (payload_length % 256);

}


/************************************************/
/* aesccmp_construct_mic_header1()                      */
/* Builds the first MIC header block from       */
/* header fields.                               */
/************************************************/
void aesccmp_construct_mic_header1_ram(u8 *mic_header1, sint header_length, u8 *mpdu, uint frtype)                        
{
	mic_header1[0] = (u8)((header_length - 2) / 256);
	mic_header1[1] = (u8)((header_length - 2) % 256);
#ifdef CONFIG_IEEE80211W
	/* 802.11w management frame don't AND subtype bits 4,5,6 of frame control field */
	if (frtype == WIFI_MGT_TYPE)
		mic_header1[2] = mpdu[0];
	else
#endif /* CONFIG_IEEE80211W */
		mic_header1[2] = mpdu[0] & 0xcf;    /* Mute CF poll & CF ack bits */

	mic_header1[3] = mpdu[1] & 0xc7;    /* Mute retry, more data and pwr mgt bits */
	mic_header1[4] = mpdu[4];       /* A1 */
	mic_header1[5] = mpdu[5];
	mic_header1[6] = mpdu[6];
	mic_header1[7] = mpdu[7];
	mic_header1[8] = mpdu[8];
	mic_header1[9] = mpdu[9];
	mic_header1[10] = mpdu[10];     /* A2 */
	mic_header1[11] = mpdu[11];
	mic_header1[12] = mpdu[12];
	mic_header1[13] = mpdu[13];
	mic_header1[14] = mpdu[14];
	mic_header1[15] = mpdu[15];

}


/************************************************/
/* aesccmp_construct_mic_header2()                      */
/* Builds the last MIC header block from        */
/* header fields.                               */
/************************************************/
void aesccmp_construct_mic_header2_ram(
	u8 *mic_header2, u8 *mpdu, sint a4_exists, sint qc_exists)
{
    sint i;

    for (i = 0; i<16; i++) mic_header2[i]=0x00;

    mic_header2[0] = mpdu[16];    /* A3 */
    mic_header2[1] = mpdu[17];
    mic_header2[2] = mpdu[18];
    mic_header2[3] = mpdu[19];
    mic_header2[4] = mpdu[20];
    mic_header2[5] = mpdu[21];

    //mic_header2[6] = mpdu[22] & 0xf0;   /* SC */
    mic_header2[6] = 0x00;
    mic_header2[7] = 0x00; /* mpdu[23]; */


    if (!qc_exists && a4_exists)
    {
        for (i=0;i<6;i++) mic_header2[8+i] = mpdu[24+i];   /* A4 */

    }

    if (qc_exists && !a4_exists)
    {
        mic_header2[8] = mpdu[24] & 0x0f; /* mute bits 15 - 4 */
        mic_header2[9] = mpdu[25] & 0x00;
    }

    if (qc_exists && a4_exists)
    {
        for (i=0;i<6;i++) mic_header2[8+i] = mpdu[24+i];   /* A4 */

        mic_header2[14] = mpdu[30] & 0x0f;
        mic_header2[15] = mpdu[31] & 0x00;
    }

}


/************************************************/
/* aesccmp_construct_mic_header2()                      */
/* Builds the last MIC header block from        */
/* header fields.                               */
/************************************************/
void aesccmp_construct_ctr_preload_ram(
	u8 *ctr_preload, sint a4_exists, sint qc_exists,
	u8 *mpdu, u8 *pn_vector, sint c, uint frtype)/* add for CONFIG_IEEE80211W, none 11w also can use */
{
    sint i = 0;
	
    for (i=0; i<16; i++) ctr_preload[i] = 0x00;
    i = 0;

    ctr_preload[0] = 0x01;                                  /* flag */
    if (qc_exists && a4_exists) 
		ctr_preload[1] = mpdu[30] & 0x0f;   /* QoC_Control */
    if (qc_exists && !a4_exists) 
		ctr_preload[1] = mpdu[24] & 0x0f;
#ifdef CONFIG_IEEE80211W
	/* 802.11w management frame should set management bit(4) */
	if (frtype == WIFI_MGT_TYPE)
		ctr_preload[1] |= BIT(4);
#endif /* CONFIG_IEEE80211W */

    for (i = 2; i < 8; i++)
        ctr_preload[i] = mpdu[i + 8];                       /* ctr_preload[2:7] = A2[0:5] = mpdu[10:15] */
#ifdef CONSISTENT_PN_ORDER
      for (i = 8; i < 14; i++)
            ctr_preload[i] =    pn_vector[i - 8];           /* ctr_preload[8:13] = PN[0:5] */
#else
      for (i = 8; i < 14; i++)
            ctr_preload[i] =    pn_vector[13 - i];          /* ctr_preload[8:13] = PN[5:0] */
#endif
    ctr_preload[14] =  (unsigned char) (c / 256); /* Ctr */
    ctr_preload[15] =  (unsigned char) (c % 256);

}

static void aesccmp_bitwise_xor(u8 *ina, u8 *inb, u8 *out)
{
	sint i;

	for (i=0; i<16; i++)
	{
		out[i] = ina[i] ^ inb[i];
	}

}

u32 ram_patch_aes_80211_encrypt(
	u8 *pframe, u32 wlan_hdr_len, 
	u32 payload_len, u8 *key, 
	u32 frame_type, u8 *mic)
{
	uint	qc_exists, a4_exists, i, j, payload_remainder,
		num_blocks, payload_index;

	u8 pn_vector[6];
	u8 mic_iv[16];
	u8 mic_header1[16];
	u8 mic_header2[16];
	u8 ctr_preload[16];

	/* Intermediate Buffers */
	u8 chain_buffer[16];
	u8 aes_out[16];
	u8 padded_buffer[16];

	u32 frtype = frame_type & 0xC;


	memset((void *)mic_iv, 0, 16);
	memset((void *)mic_header1, 0, 16);
	memset((void *)mic_header2, 0, 16);
	memset((void *)ctr_preload, 0, 16);
	memset((void *)chain_buffer, 0, 16);
	memset((void *)aes_out, 0, 16);
	memset((void *)padded_buffer, 0, 16);

	if ((wlan_hdr_len == WLAN_HDR_A3_LEN )||(wlan_hdr_len ==  WLAN_HDR_A3_QOS_LEN))
		a4_exists = 0;
	else
		a4_exists = 1;

	// Check whether QoC control exist or not. 
	if (							// WIFI_DATA_TYPE =	(BIT(3))
		(frame_type == 0x18) ||	//WIFI_DATA_CFACK (BIT(4) | WIFI_DATA_TYPE)
		(frame_type == 0x28)||	//WIFI_DATA_CFPOLL (BIT(5) | WIFI_DATA_TYPE)
		(frame_type == 0x38))		//WIFI_DATA_CFACKPOLL (BIT(5) | BIT(4) | WIFI_DATA_TYPE)
		{
			qc_exists = 1;
		}
	/* add for CONFIG_IEEE80211W, none 11w also can use */
	else if (
		(frame_type == 0x88) ||	//QoS Data
		(frame_type == 0x98)||	//QoS Data + CF-Ack
		(frame_type == 0xa8)||	//QoS Data + CF-Poll
		(frame_type == 0xb8))		//QoS Data + CF-Ack + CF-Poll
		{
			if (wlan_hdr_len !=  WLAN_HDR_A3_QOS_LEN)
				wlan_hdr_len += 2;
			
			qc_exists = 1;
		}
	else
		qc_exists = 0;

	pn_vector[0]=pframe[wlan_hdr_len];
	pn_vector[1]=pframe[wlan_hdr_len+1];
	pn_vector[2]=pframe[wlan_hdr_len+4];
	pn_vector[3]=pframe[wlan_hdr_len+5];
	pn_vector[4]=pframe[wlan_hdr_len+6];
	pn_vector[5]=pframe[wlan_hdr_len+7];
	
	aesccmp_construct_mic_iv_ram(
				mic_iv,
				qc_exists,
				a4_exists,
				pframe,	 //message,
				payload_len,
				pn_vector,
				frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
				);

	aesccmp_construct_mic_header1_ram(
				mic_header1,
				wlan_hdr_len,
				pframe,	//message
				frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
				);
	aesccmp_construct_mic_header2_ram(
				mic_header2,
				pframe,	//message,
				a4_exists,
				qc_exists
				);


	payload_remainder = payload_len % 16;
	num_blocks = payload_len / 16;

	/* Find start of payload */
	payload_index = (wlan_hdr_len + 8);

	/* Calculate MIC */
	aes1_encrypt(key, mic_iv, aes_out);
	aesccmp_bitwise_xor(aes_out, mic_header1, chain_buffer);
	aes1_encrypt(key, chain_buffer, aes_out);
	aesccmp_bitwise_xor(aes_out, mic_header2, chain_buffer);
	aes1_encrypt(key, chain_buffer, aes_out);

	for (i = 0; i < num_blocks; i++)
	{
		aesccmp_bitwise_xor(aes_out, &pframe[payload_index], chain_buffer);//bitwise_xor(aes_out, &message[payload_index], chain_buffer);

		payload_index += 16;
		aes1_encrypt(key, chain_buffer, aes_out);
	}

	/* Add on the final payload block if it needs padding */
	if (payload_remainder > 0)
	{
		for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
		for (j = 0; j < payload_remainder; j++)
		{
			padded_buffer[j] = pframe[payload_index++];//padded_buffer[j] = message[payload_index++];
		}
		aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
		aes1_encrypt(key, chain_buffer, aes_out);

	}

	for (j = 0 ; j < 8; j++) mic[j] = aes_out[j];

	/* Insert MIC into payload */
	for (j = 0; j < 8; j++)
		pframe[payload_index+j] = mic[j];	//message[payload_index+j] = mic[j];

	payload_index = wlan_hdr_len + 8;
	for (i=0; i< num_blocks; i++)
	{
		aesccmp_construct_ctr_preload_ram(
                        ctr_preload,
                        a4_exists,
                        qc_exists,
                        pframe,	//message,
                        pn_vector,
                        i+1,
                        frtype); /* add for CONFIG_IEEE80211W, none 11w also can use */
		aes1_encrypt(key, ctr_preload, aes_out);
		aesccmp_bitwise_xor(aes_out, &pframe[payload_index], chain_buffer);//bitwise_xor(aes_out, &message[payload_index], chain_buffer);
		for (j=0; j<16;j++) pframe[payload_index++] = chain_buffer[j];//for (j=0; j<16;j++) message[payload_index++] = chain_buffer[j];
	}

	if (payload_remainder > 0)          /* If there is a short final block, then pad it,*/
	{                                   /* encrypt it and copy the unpadded part back   */
		aesccmp_construct_ctr_preload_ram(
		                        ctr_preload,
		                        a4_exists,
		                        qc_exists,
		                        pframe,	//message,
		                        pn_vector,
		                        num_blocks+1,
		                        frtype); /* add for CONFIG_IEEE80211W, none 11w also can use */

		for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
		for (j = 0; j < payload_remainder; j++)
		{
		    padded_buffer[j] = pframe[payload_index+j];//padded_buffer[j] = message[payload_index+j];
		}
		aes1_encrypt(key, ctr_preload, aes_out);
		aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
		for (j=0; j<payload_remainder;j++) pframe[payload_index++] = chain_buffer[j];//for (j=0; j<payload_remainder;j++) message[payload_index++] = chain_buffer[j];
	}

	/* Encrypt the MIC */
	aesccmp_construct_ctr_preload_ram(
	                    ctr_preload,
	                    a4_exists,
	                    qc_exists,
	                    pframe,	//message,
	                    pn_vector,
	                    0,
		             frtype); /* add for CONFIG_IEEE80211W, none 11w also can use */

	for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
	for (j = 0; j < 8; j++)
	{
	    padded_buffer[j] = pframe[j+wlan_hdr_len+8+payload_len];//padded_buffer[j] = message[j+wlan_hdr_len+8+payload_len];
	}

	aes1_encrypt(key, ctr_preload, aes_out);
	aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
	for (j=0; j<8;j++) pframe[payload_index++] = chain_buffer[j];//for (j=0; j<8;j++) message[payload_index++] = chain_buffer[j];

	return _SUCCESS;
}


u32 ram_patch_aes_80211_decrypt(
	u8 *pframe, u32 wlan_hdr_len, 
	u32 payload_len, u8 *key, 
	u32 frame_type, u8 *mic)
{
	uint	qc_exists, a4_exists, i, j, payload_remainder,
			num_blocks, payload_index;
	sint res = _SUCCESS;
	u8 pn_vector[6];
	u8 mic_iv[16];
	u8 mic_header1[16];
	u8 mic_header2[16];
	u8 ctr_preload[16];

    /* Intermediate Buffers */
	u8 chain_buffer[16];
	u8 aes_out[16];
	u8 padded_buffer[16];
	u8 mic_dec[8];
	
	u32 frtype = frame_type & 0xC;

	memset((void *)mic_iv, 0, 16);
	memset((void *)mic_header1, 0, 16);
	memset((void *)mic_header2, 0, 16);
	memset((void *)ctr_preload, 0, 16);
	memset((void *)chain_buffer, 0, 16);
	memset((void *)aes_out, 0, 16);
	memset((void *)padded_buffer, 0, 16);

	//start to decrypt the payload

	num_blocks = (payload_len-8) / 16; //(payload_len including llc, payload_length and mic )

	payload_remainder = (payload_len-8) % 16;

	pn_vector[0]  = pframe[wlan_hdr_len];
	pn_vector[1]  = pframe[wlan_hdr_len+1];
	pn_vector[2]  = pframe[wlan_hdr_len+4];
	pn_vector[3]  = pframe[wlan_hdr_len+5];
	pn_vector[4]  = pframe[wlan_hdr_len+6];
	pn_vector[5]  = pframe[wlan_hdr_len+7];

	if ((wlan_hdr_len == WLAN_HDR_A3_LEN )||(wlan_hdr_len ==  WLAN_HDR_A3_QOS_LEN))
		a4_exists = 0;
	else
		a4_exists = 1;

	// Check whether QoC control exist or not. 
	// Do not use wifi.h which is 
	if (							// WIFI_DATA_TYPE =	(BIT(3))
		(frame_type == 0x18) ||	//WIFI_DATA_CFACK (BIT(4) | WIFI_DATA_TYPE)
		(frame_type == 0x28)||	//WIFI_DATA_CFPOLL (BIT(5) | WIFI_DATA_TYPE)
		(frame_type == 0x38))		//WIFI_DATA_CFACKPOLL (BIT(5) | BIT(4) | WIFI_DATA_TYPE)
		{
			if (wlan_hdr_len !=  WLAN_HDR_A3_QOS_LEN)
				wlan_hdr_len += 2;
			
			qc_exists = 1;
		}
	else if ( /* only for data packet . add for CONFIG_IEEE80211W, none 11w also can use */
		(frame_type == 0x88) ||	//QoS Data
		(frame_type == 0x98)||	//QoS Data + CF-Ack
		(frame_type == 0xa8)||	//QoS Data + CF-Poll
		(frame_type == 0xb8))		//QoS Data + CF-Ack + CF-Poll
		{
			if (wlan_hdr_len !=  WLAN_HDR_A3_QOS_LEN)
				wlan_hdr_len += 2;
			
			qc_exists = 1;
		}
	else
		qc_exists = 0;


	// now, decrypt pframe with wlan_hdr_len offset and payload_len long

	payload_index = wlan_hdr_len + 8; // 8 is for extiv
	
	for (i=0; i< num_blocks; i++) {
		aesccmp_construct_ctr_preload_ram(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                pframe,
                                pn_vector,
                                i+1,
                                frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
                            );

		aes1_encrypt(key, ctr_preload, aes_out);
		aesccmp_bitwise_xor(aes_out, &pframe[payload_index], chain_buffer);

		for (j=0; j<16;j++) pframe[payload_index++] = chain_buffer[j];
	}

	if (payload_remainder > 0) {
		/* If there is a short final block, then pad it,*/
		/* encrypt it and copy the unpadded part back   */

		aesccmp_construct_ctr_preload_ram(
                                ctr_preload,
                                a4_exists,
                                qc_exists,
                                pframe,
                                pn_vector,
                                num_blocks+1,
                                frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
                            );

		for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
		for (j = 0; j < payload_remainder; j++) {
			padded_buffer[j] = pframe[payload_index+j];
		}
		aes1_encrypt(key, ctr_preload, aes_out);
		aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
		for (j=0; j<payload_remainder;j++) pframe[payload_index++] = chain_buffer[j];
	}

	/* Decrypt the MIC */
	aesccmp_construct_ctr_preload_ram(
			ctr_preload,
			a4_exists,
			qc_exists,
			pframe,
			pn_vector,
			0,
                     frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
                     );

	for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
	for (j = 0; j < 8; j++) {
		padded_buffer[j] = pframe[j+wlan_hdr_len+8+payload_len-8];
	}
	aes1_encrypt(key, ctr_preload, aes_out);
	aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
	for (j=0; j<8;j++) mic_dec[j] = chain_buffer[j];


	//start to calculate the mic	

	pn_vector[0]=pframe[wlan_hdr_len];
	pn_vector[1]=pframe[wlan_hdr_len+1];
	pn_vector[2]=pframe[wlan_hdr_len+4];
	pn_vector[3]=pframe[wlan_hdr_len+5];
	pn_vector[4]=pframe[wlan_hdr_len+6];
	pn_vector[5]=pframe[wlan_hdr_len+7];


	
	aesccmp_construct_mic_iv_ram(
                        mic_iv,
                        qc_exists,
                        a4_exists,
                        pframe,
                        payload_len-8,
                        pn_vector,
                        frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
                        );

	aesccmp_construct_mic_header1_ram(
                            mic_header1,
                            wlan_hdr_len,
                            pframe,
                            frtype /* add for CONFIG_IEEE80211W, none 11w also can use */
                            );
	aesccmp_construct_mic_header2_ram(
                            mic_header2,
                            pframe,
                            a4_exists,
                            qc_exists
                            );


	payload_remainder = (payload_len-8) % 16;
	num_blocks = (payload_len-8) / 16;

	/* Find start of payload */
	payload_index = (wlan_hdr_len + 8);

	/* Calculate MIC */
	aes1_encrypt(key, mic_iv, aes_out);
	aesccmp_bitwise_xor(aes_out, mic_header1, chain_buffer);
	aes1_encrypt(key, chain_buffer, aes_out);
	aesccmp_bitwise_xor(aes_out, mic_header2, chain_buffer);
	aes1_encrypt(key, chain_buffer, aes_out);

	for (i = 0; i < num_blocks; i++) {
		aesccmp_bitwise_xor(aes_out, &pframe[payload_index], chain_buffer);

		payload_index += 16;
		aes1_encrypt(key, chain_buffer, aes_out);
	}

	/* Add on the final payload block if it needs padding */
	if (payload_remainder > 0) {
		for (j = 0; j < 16; j++) padded_buffer[j] = 0x00;
		for (j = 0; j < payload_remainder; j++) {
			padded_buffer[j] = pframe[payload_index++];
		}
		aesccmp_bitwise_xor(aes_out, padded_buffer, chain_buffer);
		aes1_encrypt(key, chain_buffer, aes_out);
	}

	for (j = 0 ; j < 8; j++) mic[j] = aes_out[j];

	//compare the mic
	for(i=0;i<8;i++) {
		if(mic_dec[i] != mic[i])
		{
			res = _FAIL;
		}
	}
	return res;
}
#if defined ( __GNUC__ )
void __wrap_rom_psk_CalcGTK(unsigned int AuthKeyMgmt,
				unsigned char *addr, unsigned char *nonce,
				unsigned char *keyin, int keyinlen,
				unsigned char *keyout, int keyoutlen){
				
	ram_patch_rom_psk_CalcGTK(AuthKeyMgmt, addr, nonce, keyin, keyinlen,
				keyout, keyoutlen);
}

void __wrap_rom_psk_CalcPTK(unsigned int AuthKeyMgmt,
			unsigned char *addr1, unsigned char *addr2,
			unsigned char *nonce1, unsigned char *nonce2,
			unsigned char *keyin, int keyinlen,
			unsigned char *keyout, int keyoutlen){

	ram_patch_rom_psk_CalcPTK(AuthKeyMgmt, addr1, addr2, nonce1, nonce2, keyin, keyinlen,
			keyout, keyoutlen);
}

void __wrap_aesccmp_construct_mic_iv(
	u8 *mic_iv, sint qc_exists, sint a4_exists, 
	u8 *mpdu, uint payload_length,u8 *pn_vector,
	uint frtype){

	aesccmp_construct_mic_iv_ram(mic_iv, qc_exists, a4_exists, mpdu, payload_length, pn_vector, frtype);
}

void __wrap_aesccmp_construct_mic_header1(u8 *mic_header1, sint header_length, u8 *mpdu, uint frtype){

	aesccmp_construct_mic_header1_ram(mic_header1, header_length, mpdu, frtype);
} 

void __wrap_aesccmp_construct_ctr_preload(
	u8 *ctr_preload, sint a4_exists, sint qc_exists,
	u8 *mpdu, u8 *pn_vector, sint c, uint frtype){

	aesccmp_construct_ctr_preload_ram(ctr_preload, a4_exists, qc_exists, mpdu, pn_vector, c, frtype);
}

u32 __wrap_aes_80211_encrypt(
	u8 *pframe, u32 wlan_hdr_len, 
	u32 payload_len, u8 *key, 
	u32 frame_type, u8 *mic){

	return ram_patch_aes_80211_encrypt(pframe, wlan_hdr_len, payload_len, key, frame_type, mic);
}

u32 __wrap_aes_80211_decrypt(
	u8 *pframe, u32 wlan_hdr_len, 
	u32 payload_len, u8 *key, 
	u32 frame_type, u8 *mic){

	return ram_patch_aes_80211_decrypt(pframe, wlan_hdr_len, payload_len, key, frame_type, mic);
}

u32 __wrap_CheckMIC(
    OCTET_STRING EAPOLMsgRecvd, 
    unsigned char *key, int keylen){

    return ram_patch_rom_CheckMIC(EAPOLMsgRecvd, key, keylen);
}

void __wrap_CalcMIC(
    OCTET_STRING EAPOLMsgSend, 
    int algo, 
    unsigned char *key, 
    int keylen){

    ram_patch_rom_CalcMIC(EAPOLMsgSend, algo, key, keylen);
}

#endif

