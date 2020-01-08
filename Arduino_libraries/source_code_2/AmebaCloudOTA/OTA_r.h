#ifndef _OTA_H_
#define _OTA_H_

#define DEFAULT_OTA_ADDRESS 0x80000

class OTAClass {

public:
    OTAClass();

    int beginRemote(char* server,char* path,int port, int check_sum_ota, bool reboot_when_success = true,uint32_t ota_timeout = 30000);
    int gatherOTAinfo(char* server,char* path, int port);
    int setOtaAddress(uint32_t address);
    int setRecoverPin(uint32_t pin1, uint32_t pin2 = 0xFFFFFFFF);

private:
    int get_image_info(uint32_t *img2_addr, uint32_t *img2_len, uint32_t *img3_addr, uint32_t *img3_len);
    int sync_ota_addr();
    int set_system_data(uint32_t address, uint32_t value);

    uint32_t ota_addr;
};

extern OTAClass OTA;

#endif
