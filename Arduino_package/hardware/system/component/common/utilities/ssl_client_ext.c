#include "platform_opts.h"

#if CONFIG_USE_POLARSSL

#include <polarssl/ssl.h>
#include <polarssl/memory.h>

//#define SSL_VERIFY_CLIENT
//#define SSL_VERIFY_SERVER

#ifdef SSL_VERIFY_CLIENT
static x509_crt* _cli_crt = NULL;
static pk_context* _clikey_rsa = NULL;

static const char *test_client_key = \
"-----BEGIN RSA PRIVATE KEY-----\r\n" \
"MIICXgIBAAKBgQDKLbkPtV0uhoqkHxHl/sZlq5TrUqu6pScqGkMnEUDKIFR5QMNf\r\n" \
"qLgbGPwbreN4AkHQlvqnn/2Swz1uurUH4pxcGp54j7QmANXvd5hJtCMhPpDcPS6k\r\n" \
"ldlIJ8y3KoCoqAot6uo9IL/IKKk3aOQqeHKayIyjOOksjMkgeE8/gCpmFQIDAQAB\r\n" \
"AoGBAKoSBj+Bh83wXUWr4SmAxLGXwSCnHVBXRveyudRuPfsJcSXCZdbdHWml/cTm\r\n" \
"5Jb6BxUJO/avreW8GLxBkLD+XhnXlkw1RJ8FYZPXdzlNJzoYyVK0GZ/qyGacEEFt\r\n" \
"ekvGfBJIq+7ksKcJt5c9qARClOvauYLRGwubl64xD6PupSINAkEA+5C395h227nc\r\n" \
"5zF8s2rYBP78i5uS7hKqqVjGy8pcIFHiM/0ehzcN3V3gJXLjkAbXfvP0h/tm8eQG\r\n" \
"QUpJBY/YLwJBAM2+IOfTmEBxrpASUeN1Lx9yg0+Swyz8oz2a2blfFwbpCWBi18M2\r\n" \
"huo+YECeMggqBBYwgQ9J2ixpaj/e9+0pkPsCQQDztTWkFf4/y4WoLBcEseNoo6YB\r\n" \
"kcv7+/V9bdXZI8ewP+OGPhdPIxS5efJmFTFEHHy0Lp6dBf6rJB6zLcYkL0BdAkEA\r\n" \
"nGBqeknlavX9DBwgiZXD308WZyDRoBvVpzlPSwnvYp01N0FpZULIgLowRmz28iWd\r\n" \
"PZBYR9qGLUNiMnGyV1xEiQJAOdlBM4M9Xj2Z9inCdkgFkbIOSe5kvIPC24CjZyyG\r\n" \
"g3lK/YezoDmdD//OLoY81y6VdO5dwjm7P0wZB63EDRidHA==\r\n" \
"-----END RSA PRIVATE KEY-----\r\n";

static const char *test_client_cert = \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIC4DCCAkmgAwIBAgIBAjANBgkqhkiG9w0BAQsFADB7MQswCQYDVQQGEwJDTjEL\r\n" \
"MAkGA1UECAwCSlMxCzAJBgNVBAcMAlNaMRAwDgYDVQQKDAdSZWFsc2lsMRAwDgYD\r\n" \
"VQQLDAdSZWFsdGVrMRAwDgYDVQQDDAdSZWFsc2lsMRwwGgYJKoZIhvcNAQkBFg1h\r\n" \
"QHJlYWxzaWwuY29tMB4XDTE1MTIyMzA2NTI0MFoXDTE2MTIyMjA2NTI0MFowdDEL\r\n" \
"MAkGA1UEBhMCQ04xCzAJBgNVBAgMAkpTMRAwDgYDVQQKDAdSZWFsc2lsMRAwDgYD\r\n" \
"VQQLDAdSZWFsdGVrMRYwFAYDVQQDDA0xOTIuMTY4LjEuMTQxMRwwGgYJKoZIhvcN\r\n" \
"AQkBFg1jQHJlYWxzaWwuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDK\r\n" \
"LbkPtV0uhoqkHxHl/sZlq5TrUqu6pScqGkMnEUDKIFR5QMNfqLgbGPwbreN4AkHQ\r\n" \
"lvqnn/2Swz1uurUH4pxcGp54j7QmANXvd5hJtCMhPpDcPS6kldlIJ8y3KoCoqAot\r\n" \
"6uo9IL/IKKk3aOQqeHKayIyjOOksjMkgeE8/gCpmFQIDAQABo3sweTAJBgNVHRME\r\n" \
"AjAAMCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0\r\n" \
"ZTAdBgNVHQ4EFgQUJLmwJNyKHCTEspNTPNpbPjXkjnQwHwYDVR0jBBgwFoAUAfLa\r\n" \
"cSF933h+3pYNcs36lvm7yEkwDQYJKoZIhvcNAQELBQADgYEAlo495gu94nMHFYx4\r\n" \
"+V7PjwGIqanqwLjsem9qvwJa/K1QoM4JxnqRXFUdSfZMhnlrMgPer4fDHpWAutWB\r\n" \
"X2Fiww+VVJSn8Go0seK8RQf8n/n3rJ5B3lef1Po2zHchELWhlFT6k5Won7gp64RN\r\n" \
"9PcwFFy0Va/bkJsot//kdZNKs/g=\r\n" \
"-----END CERTIFICATE-----\r\n";
#endif

#ifdef SSL_VERIFY_SERVER
static x509_crt* _ca_crt = NULL;

static const char *test_ca_cert = \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIICxDCCAi2gAwIBAgIJANdeY8UOfqpBMA0GCSqGSIb3DQEBCwUAMHsxCzAJBgNV\r\n" \
"BAYTAkNOMQswCQYDVQQIDAJKUzELMAkGA1UEBwwCU1oxEDAOBgNVBAoMB1JlYWxz\r\n" \
"aWwxEDAOBgNVBAsMB1JlYWx0ZWsxEDAOBgNVBAMMB1JlYWxzaWwxHDAaBgkqhkiG\r\n" \
"9w0BCQEWDWFAcmVhbHNpbC5jb20wHhcNMTUxMjIzMDYzMDA1WhcNMTYxMjIyMDYz\r\n" \
"MDA1WjB7MQswCQYDVQQGEwJDTjELMAkGA1UECAwCSlMxCzAJBgNVBAcMAlNaMRAw\r\n" \
"DgYDVQQKDAdSZWFsc2lsMRAwDgYDVQQLDAdSZWFsdGVrMRAwDgYDVQQDDAdSZWFs\r\n" \
"c2lsMRwwGgYJKoZIhvcNAQkBFg1hQHJlYWxzaWwuY29tMIGfMA0GCSqGSIb3DQEB\r\n" \
"AQUAA4GNADCBiQKBgQCmfNpluJZP0Sla+MIYzRGA1rljK5VncuBKQiKBF4BdO73H\r\n" \
"OTUoT0ydR7x7lS2Ns1HQop2oldroJVBj38+pLci1i/3flkONCDfsWOzfcGZ9RItq\r\n" \
"Zf9eQI8CEZI5i0Fvi3mgaoqCXvutFBrtTQRNsKQD69SqxEWWPb1y+Fd2nONeawID\r\n" \
"AQABo1AwTjAdBgNVHQ4EFgQUAfLacSF933h+3pYNcs36lvm7yEkwHwYDVR0jBBgw\r\n" \
"FoAUAfLacSF933h+3pYNcs36lvm7yEkwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\r\n" \
"AQsFAAOBgQA6McwC1Vk4k/5Bh/sf9cfwSK9A0ecaIH0NizYoWpWRAsv7TDgj0PbO\r\n" \
"Qqxi/QhpuYezgRqKqAv7QYNSQa39X7opzSsdSGtTnId374PZZeCDqZpfcAbsNk5o\r\n" \
"6HLpJ27esFa/flTL0FtmO+AT2uiPMvRP0a4u4uuLQK2Jgm/CmzJ47w==\r\n" \
"-----END CERTIFICATE-----\r\n";

static int my_verify(void *data, x509_crt *crt, int depth, int *flags) 
{
	char buf[1024];
	((void) data);

	printf("Verify requested for (Depth %d):\n", depth);
	x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	printf("%s", buf);

	if(((*flags) & BADCERT_EXPIRED) != 0)
		printf("server certificate has expired\n");

	if(((*flags) & BADCERT_REVOKED) != 0)
		printf("  ! server certificate has been revoked\n");

	if(((*flags) & BADCERT_CN_MISMATCH) != 0)
		printf("  ! CN mismatch\n");

	if(((*flags) & BADCERT_NOT_TRUSTED) != 0)
		printf("  ! self-signed or not signed by a trusted CA\n");

	if(((*flags) & BADCRL_NOT_TRUSTED) != 0)
		printf("  ! CRL not trusted\n");

	if(((*flags) & BADCRL_EXPIRED) != 0)
		printf("  ! CRL expired\n");

	if(((*flags) & BADCERT_OTHER) != 0)
		printf("  ! other (unknown) flag\n");

	if((*flags) == 0)
		printf("  Certificate verified without error flags\n");

	return(0);
}
#endif

int ssl_client_ext_init(void)
{
#ifdef SSL_VERIFY_CLIENT
	_cli_crt = polarssl_malloc(sizeof(x509_crt));
	
	if(_cli_crt)
		x509_crt_init(_cli_crt);
	else
		return -1;

	_clikey_rsa = polarssl_malloc(sizeof(pk_context));

	if(_clikey_rsa)
		pk_init(_clikey_rsa);
	else
		return -1;
#endif
#ifdef SSL_VERIFY_SERVER
	_ca_crt = polarssl_malloc(sizeof(x509_crt));

	if(_ca_crt)
		x509_crt_init(_ca_crt);
	else
		return -1;
#endif
	return 0;
}

void ssl_client_ext_free(void)
{
#ifdef SSL_VERIFY_CLIENT
	if(_cli_crt) {
		x509_crt_free(_cli_crt);
		polarssl_free(_cli_crt);
		_cli_crt = NULL;
	}

	if(_clikey_rsa) {
		pk_free(_clikey_rsa);
		polarssl_free(_clikey_rsa);
		_clikey_rsa = NULL;
	}
#endif	
#ifdef SSL_VERIFY_SERVER
	if(_ca_crt) {
		x509_crt_free(_ca_crt);
		polarssl_free(_ca_crt);
		_ca_crt = NULL;
	}
#endif
}

int ssl_client_ext_setup(ssl_context *ssl)
{
#ifdef SSL_VERIFY_CLIENT
	if(x509_crt_parse(_cli_crt, test_client_cert, strlen(test_client_cert)) != 0)
		return -1;

	if(pk_parse_key(_clikey_rsa, test_client_key, strlen(test_client_key), NULL, 0) != 0)
		return -1;

	ssl_set_own_cert(ssl, _cli_crt, _clikey_rsa);
#endif
#ifdef SSL_VERIFY_SERVER
	if(x509_crt_parse(_ca_crt, test_ca_cert, strlen(test_ca_cert)) != 0)
		return -1;

	ssl_set_ca_chain(ssl, _ca_crt, NULL, NULL);
	ssl_set_authmode(ssl, SSL_VERIFY_REQUIRED);
	ssl_set_verify(ssl, my_verify, NULL);
#endif
	return 0;
}

#elif CONFIG_USE_MBEDTLS /* CONFIG_USE_POLARSSL */

#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/ssl.h"

#define SSL_VERIFY_CLIENT
#define SSL_VERIFY_SERVER

#ifdef SSL_VERIFY_CLIENT
static mbedtls_x509_crt* _cli_crt = NULL;
static mbedtls_pk_context* _clikey_rsa = NULL;

static const char *test_client_key = \
/*
"-----BEGIN EC PRIVATE KEY-----\r\n" \
"MHcCAQEEIAo+/Ww1GDA+WBdJ9Vu1bVVYJXExjupnWAb89mwM58/foAoGCCqGSM49\r\n" \
"AwEHoUQDQgAEr9tr9Emc00+yuqtzWNMEFi8wleH2WWvzkTdpagagqBaGzqpR6L+e\r\n" \
"1oo2la3IXmd2S47Votz30YfZDuZFJKFWWg==\r\n" \
"-----END EC PRIVATE KEY-----\r\n";
*/
"-----BEGIN RSA PRIVATE KEY-----\r\n" \
"MIIEowIBAAKCAQEA5vUFULwn4Kaf12omqdLZJnBy7u3DzyPDvf3LvagyYcRL1tIE\r\n" \
"atZLODo3aawa/Ea3n50kYU7fPXXpRDOLBbHE6b2+MQGvWhaEtYuuMVx8SNmg0puk\r\n" \
"qeggLfMML7a/bY9lSUpjVfVQPkLUl/lkTVjEamQ9X52hHb07f9QvyLRtbS/4AlQX\r\n" \
"UR2IWlHfulvDihR3IfbCxt7UetBqF38Bsc17rorsb94OGRW7zXZYhK4RikpFumCN\r\n" \
"b9tkfN3OBqxv9WVwF/VYbkl9gexAEIW/UOSBTVBS2+vCALYo/Yy85Nn1IuDnctCG\r\n" \
"9hG1F0FPvbyQSyBJ/3UugZy/gSXQa1HkHMLtHwIDAQABAoIBAQDMexMBsBT+aYgL\r\n" \
"iQhCQ1PPHLAlqo060Ed150aD3G7+8HTU9YzDqodeeOymuqIJyfK5dr/HB7XPDJ5C\r\n" \
"q//iQso8yKmjagJ+mIFW2xyWx3OibExfKz5W0BXtmMqpU/uYXOUoRpw8dr1c4n6n\r\n" \
"Lz8G3BjKuYU9KqqDUg9j1dGYuWZIzAkJNGJUA0L2NnuZL/u+YJMaQLN4/XcYzFjU\r\n" \
"h/OKqKJJKPUE6ywD1P29pHUaJDCJnypomoUAK1TLnUO2+NzPp+6kFQQOFQUne1f4\r\n" \
"5TRPw4fO5Num6O9FWoHR/A1EYKYU0w/6y5eTCT0JdKOj+EzQk01lm81mpNJui24F\r\n" \
"Jxz1uNmBAoGBAPWnD66aifbtEKaYTRYjxoLxrp5TQEbWMrN9ChthtkZpvCOg2VO0\r\n" \
"yzFLF42TJVmRioUguQbh6eAq0mtDyrIxhVGKptgO7Ex16yXoOthIq90zktQhCGaQ\r\n" \
"EXQLutFZAfxVqOheoCx3l6jwD8EZgNJyZ5WyLUhFj+uiks2C34tAiLdHAoGBAPCv\r\n" \
"foaJTXmqjp5l4UPoa7yn6uatN8WPJBy+zswut78gQymZVTLBL2RFuPYUwOXLkmEp\r\n" \
"W14zFzni7KVqjToA/JxLAU8fDOowQDafUoDdRFxwgwi4QYjEbXj+HMAYZoVBKQKD\r\n" \
"EoSkqcRZhJXo5sgCBFoeA7rhb6akBy9Otgj29LdpAoGAfFD1QLl4hHvoZ0bADCpC\r\n" \
"tdW1Nu6OosqXkfn/eWfzpKKx0Z3/Hbtq8SE9ZCpJRpC+9yxeNrtxbj59ikcedxtU\r\n" \
"irWORd0XfIJYZDCoRvhP8Xu+HJgy9iSGkKG3A2b1+EspVZ89lANZvubuMhzD8rxu\r\n" \
"63TmMaLyeJ8nh9VpZ8Fa7tUCgYB2vz7/hZJx6pI+2CkR5gPxqi8c7G1NzVeVzxYc\r\n" \
"axhA9dvtFDeSuPl20Wd2EbsyJQPtaAgqK67T4n+7BRz0dzQqsF7O+JTYnkGwMV71\r\n" \
"MTXfHauoi6/ZmIAiZ80rgV5jdEiVcrGaO9t+gmQFykjCeSxIgfJ5K2x4nQjmcEEj\r\n" \
"nyQRsQKBgDOsVcfi+LJ6K8M1tmeds2NWVQD+0jrdLCl8ER8YYWuSzy89ChRntCqo\r\n" \
"NSGNJsL/xTW6AJ1n0ydYmtyxpBLGZ1j3pxd73khfgbyGNZmm37XncWKABVR1stdj\r\n" \
"P/F6EaWDBCBSDaW9rdD696YeDzov6Jk+eGWMkFyuweS8jn2bViEz\r\n" \
"-----END RSA PRIVATE KEY-----\r\n";

static const char *test_client_cert = \
/*
"-----BEGIN CERTIFICATE-----\r\n" \
"MIICHDCCAcGgAwIBAgIBBDAKBggqhkjOPQQDAjBjMQswCQYDVQQGEwJUVzEPMA0G\r\n" \
"A1UECAwGVEFJV0FOMQ8wDQYDVQQHDAZUQUlQRUkxEDAOBgNVBAoMB1JFQUxURUsx\r\n" \
"DDAKBgNVBAsMA1NEOTESMBAGA1UEAwwJTE9DQUxIT1NUMB4XDTE5MDcwMTA3MzQy\r\n" \
"MloXDTIwMDYzMDA3MzQyMlowTjELMAkGA1UEBhMCVFcxDzANBgNVBAgMBlRBSVdB\r\n" \
"TjEQMA4GA1UECgwHUkVBTFRFSzEMMAoGA1UECwwDU0Q5MQ4wDAYDVQQDDAVBTUVC\r\n" \
"QTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABK/ba/RJnNNPsrqrc1jTBBYvMJXh\r\n" \
"9llr85E3aWoGoKgWhs6qUei/ntaKNpWtyF5ndkuO1aLc99GH2Q7mRSShVlqjezB5\r\n" \
"MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wgR2VuZXJhdGVkIENl\r\n" \
"cnRpZmljYXRlMB0GA1UdDgQWBBRz2tQhBGcEAL6tPYLhU4kg3uYOHTAfBgNVHSME\r\n" \
"GDAWgBRSTDLNZIgisg0l9pqLuvmeENz+ojAKBggqhkjOPQQDAgNJADBGAiEA+dke\r\n" \
"wVXyG8NAUvFS1yFkW2NPtmxJkbntcgRGeNLwjpwCIQCAPJ0pFMhmhshc0SnR2uGw\r\n" \
"w2Avx+M2miirhAP3h7RKlA==\r\n" \
"-----END CERTIFICATE-----\r\n";
*/

"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDpTCCAo2gAwIBAgIBBjANBgkqhkiG9w0BAQsFADBgMQswCQYDVQQGEwJUVzEP\r\n" \
"MA0GA1UECAwGVEFJV0FOMQ8wDQYDVQQHDAZUQUlQRUkxEDAOBgNVBAoMB1JFQUxU\r\n" \
"RUsxDDAKBgNVBAsMA1NEOTEPMA0GA1UEAwwGU0VSVkVSMB4XDTE5MDcwMTEyNTE1\r\n" \
"MFoXDTIwMDYzMDEyNTE1MFowTzELMAkGA1UEBhMCVFcxDzANBgNVBAgMBlRBSVdB\r\n" \
"TjEQMA4GA1UECgwHUkVBTFRFSzEMMAoGA1UECwwDU0Q5MQ8wDQYDVQQDDAZDTElF\r\n" \
"TlQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDm9QVQvCfgpp/Xaiap\r\n" \
"0tkmcHLu7cPPI8O9/cu9qDJhxEvW0gRq1ks4OjdprBr8RrefnSRhTt89delEM4sF\r\n" \
"scTpvb4xAa9aFoS1i64xXHxI2aDSm6Sp6CAt8wwvtr9tj2VJSmNV9VA+QtSX+WRN\r\n" \
"WMRqZD1fnaEdvTt/1C/ItG1tL/gCVBdRHYhaUd+6W8OKFHch9sLG3tR60GoXfwGx\r\n" \
"zXuuiuxv3g4ZFbvNdliErhGKSkW6YI1v22R83c4GrG/1ZXAX9VhuSX2B7EAQhb9Q\r\n" \
"5IFNUFLb68IAtij9jLzk2fUi4Ody0Ib2EbUXQU+9vJBLIEn/dS6BnL+BJdBrUeQc\r\n" \
"wu0fAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5TU0wg\r\n" \
"R2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBQCxCte0w/NK9aLwK1IorGQ\r\n" \
"EebvKzAfBgNVHSMEGDAWgBRYl2uL1taCIPUNMykHTeoeIPbyWTANBgkqhkiG9w0B\r\n" \
"AQsFAAOCAQEANRu9C+sxMUkQYq9h6Y7hV96K7Ox17sJZjzZHoTAPgdeRM1KjW0Xt\r\n" \
"wM2oophB+M2mqegrYzSGZS0OWx6Hl5c3dnPEtfRMEGcTrN3hvNt4GZl+gQW2TEod\r\n" \
"yi97SsrdLqKTuWCN195is3GU0YzJdodQHzPQjhJMODwC47e8EEAAn+WA2W0rAOfs\r\n" \
"d5rScFNZ8XTZi53x9Hb2LF0zr2cXCcl2oo7cAbEp/9bGw3K1Z7Y4W+H3TiT0OhjY\r\n" \
"f+s7MtLN5qT3jO6vhNlO3ZyT9LuuBdR8U2F05r8G64BNCO1qr+bHOo8gkrLk+kdp\r\n" \
"1sRMENCOM1bMSf02RfTITv0+06+GYLo72A==\r\n" \
"-----END CERTIFICATE-----\r\n";

#endif

#ifdef SSL_VERIFY_SERVER
static mbedtls_x509_crt* _ca_crt = NULL;

static const char *test_ca_cert = \
/*
"-----BEGIN CERTIFICATE-----\r\n" \
"MIICGzCCAcGgAwIBAgIUGkjFUIsaqFnLq4fRKITr6Y458LkwCgYIKoZIzj0EAwIw\r\n" \
"YzELMAkGA1UEBhMCVFcxDzANBgNVBAgMBlRBSVdBTjEPMA0GA1UEBwwGVEFJUEVJ\r\n" \
"MRAwDgYDVQQKDAdSRUFMVEVLMQwwCgYDVQQLDANTRDkxEjAQBgNVBAMMCUxPQ0FM\r\n" \
"SE9TVDAeFw0xOTA3MDEwNzI2MDNaFw0yMDA2MzAwNzI2MDNaMGMxCzAJBgNVBAYT\r\n" \
"AlRXMQ8wDQYDVQQIDAZUQUlXQU4xDzANBgNVBAcMBlRBSVBFSTEQMA4GA1UECgwH\r\n" \
"UkVBTFRFSzEMMAoGA1UECwwDU0Q5MRIwEAYDVQQDDAlMT0NBTEhPU1QwWTATBgcq\r\n" \
"hkjOPQIBBggqhkjOPQMBBwNCAAR5KcnWOU5xkoqJFDwc1Ql62fFhVoG8rcsTblQE\r\n" \
"Kkma7LagSOAVFyEKggYVWPjdbDJ/ELeiUPi3etX/crjTdpnDo1MwUTAdBgNVHQ4E\r\n" \
"FgQUUkwyzWSIIrINJfaai7r5nhDc/qIwHwYDVR0jBBgwFoAUUkwyzWSIIrINJfaa\r\n" \
"i7r5nhDc/qIwDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBFAiAmE/lq\r\n" \
"7z/w2cpRERZBsjcv5RRV+bWdUzzoRrlQNrHSLgIhAOAhzIzGzCzO8IijJIO3zWhQ\r\n" \
"bcsa/hlOcqVIKF5+vFEI\r\n" \
"-----END CERTIFICATE-----\r\n";
*/

"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDoTCCAomgAwIBAgIUCRcN8u1q2XZGZlOApA0cmM9LCgIwDQYJKoZIhvcNAQEL\r\n" \
"BQAwYDELMAkGA1UEBhMCVFcxDzANBgNVBAgMBlRBSVdBTjEPMA0GA1UEBwwGVEFJ\r\n" \
"UEVJMRAwDgYDVQQKDAdSRUFMVEVLMQwwCgYDVQQLDANTRDkxDzANBgNVBAMMBlNF\r\n" \
"UlZFUjAeFw0xOTA3MDExMjQ4MzJaFw0yMDA2MzAxMjQ4MzJaMGAxCzAJBgNVBAYT\r\n" \
"AlRXMQ8wDQYDVQQIDAZUQUlXQU4xDzANBgNVBAcMBlRBSVBFSTEQMA4GA1UECgwH\r\n" \
"UkVBTFRFSzEMMAoGA1UECwwDU0Q5MQ8wDQYDVQQDDAZTRVJWRVIwggEiMA0GCSqG\r\n" \
"SIb3DQEBAQUAA4IBDwAwggEKAoIBAQDOn2lJVxUGTX4dRvOmjyLJcX2lgS5DLyxy\r\n" \
"IfqOhZB4ts2THtyrRI8xlgRD8KDNjF4oTb47dL4NEHC2a3wvpaijmUkhFa2pOQew\r\n" \
"+XV6bdy8CwVWrdCnD4RXJMOltJE4QUh7tiLLN+XOBOJ4YasHm7YnrbGGBtrAkw+U\r\n" \
"15OyAsbVmTL9eQ6JozorfP6JjPUJ470bYipWGVCN0nCBhC8THpcfzWSgr12zhSmL\r\n" \
"iXyB8mEIZ3nzMD9mKuQONhbnWfiVAvnXMR0chbV3zzrLhYmjSczpkIagdu6C2oKo\r\n" \
"OANOlT3yul2Eu/1wpZ6I7T8BgH+RN1BbYKpk2WGxi5OPdjcBIjUnAgMBAAGjUzBR\r\n" \
"MB0GA1UdDgQWBBRYl2uL1taCIPUNMykHTeoeIPbyWTAfBgNVHSMEGDAWgBRYl2uL\r\n" \
"1taCIPUNMykHTeoeIPbyWTAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUA\r\n" \
"A4IBAQAIMlY+V+PPGpwz/mCOZVyDI81llJDn/NTYXdXSXoy9AJ22/Pw8wB2EgFey\r\n" \
"sIav0qCoeTMhHQ3h97DpBRLecFhIiadNwO1J1DF0gYxeqgr5FD0Szx1LohnOhqlX\r\n" \
"LhFMNuPMQ6TgTuIKgCjUeZvusH6sRt7H4of94YIDwSVKT8lCTu73hKneb9iZ3bCP\r\n" \
"MyB2Q+wpTi4upraKyVLznnIDDkLOdsuO0+H7g+F9zdKlWvylPmYIiz0hH39QL9F/\r\n" \
"AJ7ckGhgH7gEn/zIXCTmNpmOEgRrUZ6fXltZSP7GZ0yNZ8k9hjwy7fyJRm5lfu9U\r\n" \
"dT2suAYS0ijNIOlhjE7z1N53sXrv\r\n" \
"-----END CERTIFICATE-----\r\n";

static int my_verify(void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags) 
{
	char buf[1024];
	((void) data);

	printf("Verify requested for (Depth %d):\n", depth);
	mbedtls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
	printf("%s", buf);

	if(((*flags) & MBEDTLS_X509_BADCERT_EXPIRED) != 0)
		printf("server certificate has expired\n");

	if(((*flags) & MBEDTLS_X509_BADCERT_REVOKED) != 0)
		printf("  ! server certificate has been revoked\n");

	if(((*flags) & MBEDTLS_X509_BADCERT_CN_MISMATCH) != 0)
		printf("  ! CN mismatch\n");

	if(((*flags) & MBEDTLS_X509_BADCERT_NOT_TRUSTED) != 0)
		printf("  ! self-signed or not signed by a trusted CA\n");

	if(((*flags) & MBEDTLS_X509_BADCRL_NOT_TRUSTED) != 0)
		printf("  ! CRL not trusted\n");

	if(((*flags) & MBEDTLS_X509_BADCRL_EXPIRED) != 0)
		printf("  ! CRL expired\n");

	if(((*flags) & MBEDTLS_X509_BADCERT_OTHER) != 0)
		printf("  ! other (unknown) flag\n");

	if((*flags) == 0)
		printf("  Certificate verified without error flags\n");

	return(0);
}
#endif

int ssl_client_ext_init(void)
{
#ifdef SSL_VERIFY_CLIENT
	_cli_crt = (mbedtls_x509_crt *) mbedtls_calloc(1, sizeof(mbedtls_x509_crt));

	if(_cli_crt)
		mbedtls_x509_crt_init(_cli_crt);
	else
		return -1;

	_clikey_rsa = (mbedtls_pk_context *) mbedtls_calloc(1, sizeof(mbedtls_pk_context));

	if(_clikey_rsa)
		mbedtls_pk_init(_clikey_rsa);
	else
		return -1;
#endif
#ifdef SSL_VERIFY_SERVER
	_ca_crt = (mbedtls_x509_crt *) mbedtls_calloc(1, sizeof(mbedtls_x509_crt));

	if(_ca_crt)
		mbedtls_x509_crt_init(_ca_crt);
	else
		return -1;
#endif
	return 0;
}

void ssl_client_ext_free(void)
{
#ifdef SSL_VERIFY_CLIENT
	if(_cli_crt) {
		mbedtls_x509_crt_free(_cli_crt);
		mbedtls_free(_cli_crt);
		_cli_crt = NULL;
	}

	if(_clikey_rsa) {
		mbedtls_pk_free(_clikey_rsa);
		mbedtls_free(_clikey_rsa);
		_clikey_rsa = NULL;
	}
#endif
#ifdef SSL_VERIFY_SERVER
	if(_ca_crt) {
		mbedtls_x509_crt_free(_ca_crt);
		mbedtls_free(_ca_crt);
		_ca_crt = NULL;
	}
#endif
}

int ssl_client_ext_setup(mbedtls_ssl_config *conf)
{
	/* Since mbedtls crt and pk API will check string terminator to prevent non-null-terminated string, need to count string terminator to buffer length */
#ifdef SSL_VERIFY_CLIENT
	if(mbedtls_x509_crt_parse(_cli_crt, test_client_cert, strlen(test_client_cert) + 1) != 0)
		return -1;

	if(mbedtls_pk_parse_key(_clikey_rsa, test_client_key, strlen(test_client_key) + 1, NULL, 0) != 0)
		return -1;

	mbedtls_ssl_conf_own_cert(conf, _cli_crt, _clikey_rsa);
#endif
#ifdef SSL_VERIFY_SERVER
	if(mbedtls_x509_crt_parse(_ca_crt, test_ca_cert, strlen(test_ca_cert) + 1) != 0)
		return -1;

	mbedtls_ssl_conf_ca_chain(conf, _ca_crt, NULL);
	mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_verify(conf, my_verify, NULL);
#endif
	return 0;
}

#endif /* CONFIG_USE_POLARSSL */