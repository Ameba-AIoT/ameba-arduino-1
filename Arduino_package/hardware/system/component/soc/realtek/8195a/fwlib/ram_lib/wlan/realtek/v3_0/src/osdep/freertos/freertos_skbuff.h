#ifndef _FREERTOS_SKBUFF_H_
#define _FREERTOS_SKBUFF_H_

#if (RTL8195A_SUPPORT==1)
// For Lextra(PCI-E like interface), RX buffer along with its skb is required to be 
// 	pre-allocation and set into rx buffer descriptor ring during initialization. 
#define MAX_LOCAL_SKB_NUM			(10+18)	//tx+rx
#define MAX_SKB_BUF_NUM			(8 + 4) 	//tx+rx (8 + 16) Reduce rx skb number due to memory limitation
#else
#ifndef CONFIG_DONT_CARE_TP
#define MAX_LOCAL_SKB_NUM		10
#define MAX_SKB_BUF_NUM			7
#else
#define MAX_LOCAL_SKB_NUM		10
#define MAX_TX_SKB_BUF_NUM		6
#define MAX_RX_SKB_BUF_NUM		1
#endif
#endif


#endif
