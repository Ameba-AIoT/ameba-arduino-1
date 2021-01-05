#ifndef _WIFI_CONSTANTS_H
#define _WIFI_CONSTANTS_H

#ifdef	__cplusplus
extern "C" {
#endif

#define WEP_ENABLED        0x0001
#define TKIP_ENABLED       0x0002
#define AES_ENABLED        0x0004
#define WSEC_SWFLAG        0x0008

#define SHARED_ENABLED  0x00008000
#define WPA_SECURITY    0x00200000
#define WPA2_SECURITY   0x00400000
#define WPS_ENABLED     0x10000000

#define RTW_MAX_PSK_LEN		(64)
#define RTW_MIN_PSK_LEN		(8)

#define MCSSET_LEN			16

typedef enum
{
    RTW_SUCCESS                      = 0,    /**< Success */
    RTW_PENDING                      = 1,    /**< Pending */
    RTW_TIMEOUT                      = 2,    /**< Timeout */
    RTW_PARTIAL_RESULTS              = 3,    /**< Partial results */
    RTW_INVALID_KEY                  = 4,    /**< Invalid key */
    RTW_DOES_NOT_EXIST               = 5,    /**< Does not exist */
    RTW_NOT_AUTHENTICATED            = 6,    /**< Not authenticated */
    RTW_NOT_KEYED                    = 7,    /**< Not keyed */
    RTW_IOCTL_FAIL                   = 8,    /**< IOCTL fail */
    RTW_BUFFER_UNAVAILABLE_TEMPORARY = 9,    /**< Buffer unavailable temporarily */
    RTW_BUFFER_UNAVAILABLE_PERMANENT = 10,   /**< Buffer unavailable permanently */
    RTW_WPS_PBC_OVERLAP              = 11,   /**< WPS PBC overlap */
    RTW_CONNECTION_LOST              = 12,   /**< Connection lost */

    RTW_ERROR                        = -1,   /**< Generic Error */
    RTW_BADARG                       = -2,   /**< Bad Argument */
    RTW_BADOPTION                    = -3,   /**< Bad option */
    RTW_NOTUP                        = -4,   /**< Not up */
    RTW_NOTDOWN                      = -5,   /**< Not down */
    RTW_NOTAP                        = -6,   /**< Not AP */
    RTW_NOTSTA                       = -7,   /**< Not STA  */
    RTW_BADKEYIDX                    = -8,   /**< BAD Key Index */
    RTW_RADIOOFF                     = -9,   /**< Radio Off */
    RTW_NOTBANDLOCKED                = -10,  /**< Not  band locked */
    RTW_NOCLK                        = -11,  /**< No Clock */
    RTW_BADRATESET                   = -12,  /**< BAD Rate valueset */
    RTW_BADBAND                      = -13,  /**< BAD Band */
    RTW_BUFTOOSHORT                  = -14,  /**< Buffer too short */
    RTW_BUFTOOLONG                   = -15,  /**< Buffer too long */
    RTW_BUSY                         = -16,  /**< Busy */
    RTW_NOTASSOCIATED                = -17,  /**< Not Associated */
    RTW_BADSSIDLEN                   = -18,  /**< Bad SSID len */
    RTW_OUTOFRANGECHAN               = -19,  /**< Out of Range Channel */
    RTW_BADCHAN                      = -20,  /**< Bad Channel */
    RTW_BADADDR                      = -21,  /**< Bad Address */
    RTW_NORESOURCE                   = -22,  /**< Not Enough Resources */
    RTW_UNSUPPORTED                  = -23,  /**< Unsupported */
    RTW_BADLEN                       = -24,  /**< Bad length */
    RTW_NOTREADY                     = -25,  /**< Not Ready */
    RTW_EPERM                        = -26,  /**< Not Permitted */
    RTW_NOMEM                        = -27,  /**< No Memory */
    RTW_ASSOCIATED                   = -28,  /**< Associated */
    RTW_RANGE                        = -29,  /**< Not In Range */
    RTW_NOTFOUND                     = -30,  /**< Not Found */
    RTW_WME_NOT_ENABLED              = -31,  /**< WME Not Enabled */
    RTW_TSPEC_NOTFOUND               = -32,  /**< TSPEC Not Found */
    RTW_ACM_NOTSUPPORTED             = -33,  /**< ACM Not Supported */
    RTW_NOT_WME_ASSOCIATION          = -34,  /**< Not WME Association */
    RTW_SDIO_ERROR                   = -35,  /**< SDIO Bus Error */
    RTW_WLAN_DOWN                    = -36,  /**< WLAN Not Accessible */
    RTW_BAD_VERSION                  = -37,  /**< Incorrect version */
    RTW_TXFAIL                       = -38,  /**< TX failure */
    RTW_RXFAIL                       = -39,  /**< RX failure */
    RTW_NODEVICE                     = -40,  /**< Device not present */
    RTW_UNFINISHED                   = -41,  /**< To be finished */
    RTW_NONRESIDENT                  = -42,  /**< access to nonresident overlay */
    RTW_DISABLED                     = -43   /**< Disabled in this build */
} rtw_result_t;

typedef enum {
    RTW_SECURITY_OPEN           = 0,                                                /**< Open security                           */
    RTW_SECURITY_WEP_PSK        = WEP_ENABLED,                                      /**< WEP Security with open authentication   */
    RTW_SECURITY_WEP_SHARED     = ( WEP_ENABLED | SHARED_ENABLED ),                 /**< WEP Security with shared authentication */
    RTW_SECURITY_WPA_TKIP_PSK   = ( WPA_SECURITY  | TKIP_ENABLED ),                 /**< WPA Security with TKIP                  */
    RTW_SECURITY_WPA_AES_PSK    = ( WPA_SECURITY  | AES_ENABLED ),                  /**< WPA Security with AES                   */
    RTW_SECURITY_WPA2_AES_PSK   = ( WPA2_SECURITY | AES_ENABLED ),                  /**< WPA2 Security with AES                  */
    RTW_SECURITY_WPA2_TKIP_PSK  = ( WPA2_SECURITY | TKIP_ENABLED ),                 /**< WPA2 Security with TKIP                 */
    RTW_SECURITY_WPA2_MIXED_PSK = ( WPA2_SECURITY | AES_ENABLED | TKIP_ENABLED ),   /**< WPA2 Security with AES & TKIP           */

    RTW_SECURITY_WPS_OPEN       = WPS_ENABLED,                                      /**< WPS with open security                  */
    RTW_SECURITY_WPS_SECURE     = (WPS_ENABLED | AES_ENABLED),                      /**< WPS with AES security                   */

    RTW_SECURITY_UNKNOWN        = -1,                                               /**< May be returned by scan function if security is unknown. Do not pass this to the join function! */

    RTW_SECURITY_FORCE_32_BIT   = 0x7fffffff                                        /**< Exists only to force rtw_security_t type to 32 bits */
} rtw_security_t;

typedef enum {
	RTW_FALSE = 0,
	RTW_TRUE  = 1
} rtw_bool_t;

typedef enum {
	RTW_802_11_BAND_5GHZ   = 0, /**< Denotes 5GHz radio band   */
	RTW_802_11_BAND_2_4GHZ = 1  /**< Denotes 2.4GHz radio band */
} rtw_802_11_band_t;

typedef enum {
	RTW_COUNTRY_US = 0,
	RTW_COUNTRY_EU,
	RTW_COUNTRY_JP,
	RTW_COUNTRY_CN
}rtw_country_code_t;

typedef enum {
	RTW_MODE_NONE = 0,
	RTW_MODE_STA,
	RTW_MODE_AP,
	RTW_MODE_STA_AP,
	RTW_MODE_PROMISC,
	RTW_MODE_P2P
}rtw_mode_t;

typedef enum {
	RTW_SCAN_FULL = 0,
	RTW_SCAN_SOCIAL,
	RTW_SCAN_ONE
}rtw_scan_mode_t;

typedef enum {
	RTW_LINK_DISCONNECTED = 0,
	RTW_LINK_CONNECTED
} rtw_link_status_t;

typedef enum {
    RTW_SCAN_TYPE_ACTIVE              = 0x00,  /**< Actively scan a network by sending 802.11 probe(s)         */
    RTW_SCAN_TYPE_PASSIVE             = 0x01,  /**< Passively scan a network by listening for beacons from APs */
    RTW_SCAN_TYPE_PROHIBITED_CHANNELS = 0x04   /**< Passively scan on channels not enabled by the country code */
} rtw_scan_type_t;

typedef enum {
    RTW_BSS_TYPE_INFRASTRUCTURE = 0, /**< Denotes infrastructure network                  */
    RTW_BSS_TYPE_ADHOC          = 1, /**< Denotes an 802.11 ad-hoc IBSS network           */
    RTW_BSS_TYPE_ANY            = 2, /**< Denotes either infrastructure or ad-hoc network */

    RTW_BSS_TYPE_UNKNOWN        = -1 /**< May be returned by scan function if BSS type is unknown. Do not pass this to the Join function */
} rtw_bss_type_t;

typedef enum {
	RTW_SCAN_COMMAMD = 0x01
} rtw_scan_command_t;

typedef enum{
	COMMAND1					= 0x01
}rtw_command_type;

typedef enum {
	RTW_WPS_TYPE_DEFAULT 		    	= 0x0000,
	RTW_WPS_TYPE_USER_SPECIFIED 		= 0x0001,
	RTW_WPS_TYPE_MACHINE_SPECIFIED   	= 0x0002,
	RTW_WPS_TYPE_REKEY 			        = 0x0003,
	RTW_WPS_TYPE_PUSHBUTTON 		    = 0x0004,
	RTW_WPS_TYPE_REGISTRAR_SPECIFIED 	= 0x0005,
    RTW_WPS_TYPE_NONE                   = 0x0006 
} rtw_wps_type_t;

typedef enum {
    RTW_NETWORK_B   = 1,
	RTW_NETWORK_BG  = 3,
	RTW_NETWORK_BGN = 11
} rtw_network_mode_t;

typedef enum {
    RTW_STA_INTERFACE     = 0, /**< STA or Client Interface  */
    RTW_AP_INTERFACE      = 1, /**< softAP Interface         */
} rtw_interface_t;

/**
 * Enumeration of packet filter rules
 */
typedef enum {
	RTW_POSITIVE_MATCHING  = 0, /**< Specifies that a filter should match a given pattern     */
	RTW_NEGATIVE_MATCHING  = 1  /**< Specifies that a filter should NOT match a given pattern */
} rtw_packet_filter_rule_e;

typedef enum {
	RTW_PROMISC_DISABLE = 0,  /**< disable the promisc */
	RTW_PROMISC_ENABLE = 1,   /**< enable the promisc */
	RTW_PROMISC_ENABLE_1 = 2, /**< enable the promisc special for length is used */
} rtw_rcr_level_t;

#ifdef	__cplusplus
}
#endif

#endif /* _WIFI_CONSTANTS_H */
