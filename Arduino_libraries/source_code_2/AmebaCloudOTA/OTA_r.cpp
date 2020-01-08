#include "Arduino.h"
#include "OTA_r.h"
#include <HttpClient.h>
#include <WiFi.h>
#include <WiFiSSLClient.h>
#include <WiFiClient.h>


#ifdef __cplusplus
extern "C" {
#endif

//#include <lwip/sockets.h>
//#include "mDNS.h"
#include "flash_api.h"
#include "sys_api.h"

#ifdef __cplusplus
}
#endif

#define OTA_DEBUG
#ifdef OTA_DEBUG
  #define OTA_PRINTF(...) { printf(__VA_ARGS__); }
#else
  #define OTA_PRINTF(...) {}
#endif

#define BUFSIZE         512
#define IMAGE_2			0x0000B000

#define DEFAULT_IMAGE_DOWNLOAD_TIMEOUT 500000

OTAClass::OTAClass() {
    ota_addr = 0xFFFFFFFF;
}


int OTAClass::gatherOTAinfo(char* server,char* path, int port){
    int ret = -1;
    int link_status = WiFi.status();
    if(link_status !=  WL_CONNECTED){
        OTA_PRINTF("WiFi Not connected\r\n")
        return ret;
    }

    //#### HTTP
    int err =0;
    String ota_check_sum_str="";
    WiFiClient c;
    HttpClient http(c);

    err = http.get(server, port, path);
    if (err == 0) {
        Serial.println("startedRequest ok");
        err = http.responseStatusCode();
        if (err >= 0) {
            Serial.print("Got status code1: ");
            Serial.println(err);

        // Usually you'd check that the response code is 200 or a
        // similar "success" code (200-299) before carrying on,
        // but we'll print out whatever response we get
            err = http.skipResponseHeaders();
            
            if (err >= 0)
            {
                int bodyLen = http.contentLength();
                Serial.print("Content length is: ");
                Serial.println(bodyLen);
                Serial.println();
                Serial.println("Body returned follows:");

                // Now we've got to the body, so we can print it out
                unsigned long timeoutStart = millis();
                char cr;
                // Whilst we haven't timed out & haven't reached the end of the body
                while ( (http.connected() || http.available()) &&
                       ((millis() - timeoutStart) < 10000) )
                {
                    if (http.available()) {
                        cr = http.read();
                        // Print out this character
                        Serial.print(cr);
                        ota_check_sum_str += cr;
                        bodyLen--;
                        // We read something, reset the timeout counter
                        timeoutStart = millis();
                    }
                    else {
                        // We haven't got any data, so let's pause to allow some to
                        // arrive
                        delay(10);
                    }
                }
            } else {
                Serial.print("Failed to skip response headers: ");
                Serial.println(err);
            }
        } else {    
            Serial.print("Getting response failed: ");
            Serial.println(err);
        }
    } else {
        Serial.print("Connect failed: ");
        Serial.println(err);
    }

    http.stop();
    return ota_check_sum_str.toInt();
}


int OTAClass::beginRemote(char* server,char* path, int port, int check_sum_ota, bool reboot_when_success, uint32_t ota_timeout) {

    int ret = -1;

    // variables for image processing
    flash_t flash;
    uint32_t img2_addr, img2_len, img3_addr, img3_len;
    uint32_t img_upper_bound;
    uint32_t checksum = 0;
    uint32_t signature1, signature2;

    // // variables for network processing
    const int mtusize = 512;
    // variables for OTA
    unsigned char *buff = NULL;
  
    
    int read_bytes = 0, processed_len = 0;
    uint32_t file_info[3];
    uint32_t ota_len;
    uint32_t ota_blk_size = 0;

    int i, n;

    do {
        buff = (unsigned char *) malloc (BUFSIZE);
        if (buff == NULL) {
            OTA_PRINTF("Fail to allocate memory\r\n");
            return ret;
        }

        int link_status = WiFi.status();

        if(link_status !=  WL_CONNECTED){
            OTA_PRINTF("WiFi Not connected\r\n")
            return ret;
        }

        //#### HTTP
        int err =0;
        WiFiClient c;
        HttpClient http(c);

    
        Serial.println();
        Serial.print("CHECKSUM_OTA:");
        Serial.println(check_sum_ota);
        
        if(check_sum_ota == 0 ) return ret;

        //#### HTTP
        int      bodyLen = -1;
        int      image_checksum = 0;
        int      OTAlen = 0;
        uint32_t dl_cnt=0;
        err = http.get(server, port, path);
        if (err == 0) {
            Serial.println("startedRequest ok");
            err = http.responseStatusCode();
            if (err >= 0) {
                Serial.print("Got status code2: ");
                Serial.println(err);

                //prepare OTA

                setOtaAddress(DEFAULT_OTA_ADDRESS);
                // This set the recover pin. Boot device with pull up this pin (Eq. connect pin to 3.3V) would make device boot from version 1
                setRecoverPin(18);

                sync_ota_addr();

                get_image_info(&img2_addr, &img2_len, &img3_addr, &img3_len);
                img_upper_bound = img2_addr + 0x10 + img2_len; // image2 base + header + len
                if (img3_len > 0) {
                    img_upper_bound += 0x10 + img3_len; // image 3 header + len
                }

                if ((ota_addr & 0xfff != 0) || (ota_addr == ~0x0) || (ota_addr < img_upper_bound)) {
                    OTA_PRINTF("Invalid OTA address: %08X\r\n", ota_addr);
                    return ret;
                }
                //--end prepare OTA

            // Usually you'd check that the response code is 200 or a
            // similar "success" code (200-299) before carrying on,
            // but we'll print out whatever response we get
                //err = http.skipResponseHeaders();

                //err = http.readHeader();
                String http_response="";
                int header_length = 0;
                while(c.available()){
                  char cr = c.read();
                  http_response += cr;
                  header_length ++ ;
                  if(http_response.lastIndexOf("\r\n\r\n") != -1) break;
                }
                Serial.println("-----");
                Serial.println(http_response);
                Serial.print("Header length: ");
                Serial.print(header_length);
                Serial.println("-----");


                if (err >= 0)
                {
                    //bodyLen = http.contentLength();
                    int start_cat = 0;
                    int end_cat = 0;
                    int ret = 0;
                    start_cat = http_response.indexOf("Content-Length: ") +strlen("Content-Length: ");
                    end_cat = http_response.indexOf("\r\n", start_cat);
                    bodyLen = http_response.substring(start_cat, end_cat).toInt();
                    //bodyLen -= 4; //#error
                    //erase the OTAimage first
                    
                    //ota_len = OTAlen; //...
                    ota_len = bodyLen;
                    OTA_PRINTF("Erase %d from %d\r\n",ota_len ,ota_addr);
                    ota_blk_size = ((ota_len - 1) / 4096) + 1;
                    for (i = 0; i < ota_blk_size; i++) {
                        flash_erase_sector(&flash, ota_addr + i * 4096);
                    }

                    OTA_PRINTF("Start download\r\n");
                    
                    
                    memset(buff,0,sizeof(buff));
                    int initial_state = 1;
                    static int chunk_pointer=0;
                    static int processed_len=0;
                    uint32_t timer=millis();
                    while((millis() - timer) < ota_timeout){
                      if(c.available()) {
                          
                          int buf_len=0;
                          
                          if(initial_state == 1) {
                            ret = c.read(buff, mtusize - http_response.length());
                            buf_len = ret ;
                            initial_state = 2;                          
                          }
                          else if(initial_state == 2){
                            ret = c.read(buff, mtusize);
                            buf_len = ret;                           
                          }else if(initial_state == 3){
                            c.read();
                            buf_len = 1;
                          }
                          int n = 0;
                        
                          while(n < buf_len){
                          	/*                       	
                          	if(buff[n] < 0x10) Serial.print("0");
                          	Serial.print(buff[n],HEX);
                          	Serial.print(" ");
                          	*/
                            image_checksum += ((buff[n]) & 0xFF);
                            dl_cnt++;
                            n++;
                            chunk_pointer++;
                            if(chunk_pointer == buf_len){
                               if (flash_stream_write(&flash, ota_addr + processed_len, buf_len, buff) < 0) {
                                   OTA_PRINTF("Write sector fail\r\n");
                                   break;
                               }else{
                                   OTA_PRINTF("\nWrite sector %d...ok\r\n",(ota_addr + processed_len));
                                   processed_len+=chunk_pointer;
                                   chunk_pointer = 0;
                                   //delay(5);
                               }
                              memset(buff,0,sizeof(buff));
                            }
                            
                            if(dl_cnt == bodyLen){
                               if (flash_stream_write(&flash, ota_addr + processed_len, buf_len, buff) < 0) {
                                   OTA_PRINTF("Write sector fail\r\n");
                                   break;
                               }else{
                                   OTA_PRINTF("\nWrite last sector %d...ok\r\n",(ota_addr + processed_len));
                                   OTA_PRINTF("\ntotal len = %d", dl_cnt);
                                   processed_len+=chunk_pointer;
                                   chunk_pointer=0;
                                   initial_state = 3; //to calculate total 0x00 read caused by unknow issue
                               }
                             memset(buff,0,sizeof(buff));
                             break;
                            } 
                          }
                         

                          if(image_checksum == check_sum_ota) {
                            Serial.println("[checksum ready]");
                            //break;
                          }
                          //char cc = client.read();
                          //image_checksum += (((uint8_t)cc) & 0xFF);
                          
                          memset(buff,0,sizeof(buff));
                      }else{
                        if(dl_cnt >= bodyLen) break;
                        delay(1);
                      }
                    }
                } else {
                    Serial.print("Failed to skip response headers: ");
                    Serial.println(err);
                }
            } else {    
                Serial.print("Getting response failed: ");
                Serial.println(err);
            }
        } else {
            Serial.print("Connect failed: ");
            Serial.println(err);
            return ret;
        }

       
        if(http.completed()){
            Serial.println("HTTP DONE");
        }
        http.stop();
        
        OTA_PRINTF("\nTotal bytes downloaded = %d\r\n",dl_cnt);
        Serial.print("\nimage checksum downloaded:");
        Serial.println(image_checksum);
        
        //ver2
        image_checksum = processed_len = 0;
        while ( processed_len < ota_len ) {
            n = (processed_len + BUFSIZE < ota_len) ? BUFSIZE : (ota_len - processed_len);
            flash_stream_read(&flash, ota_addr + processed_len, n, buff);
            for (i=0; i<n; i++) image_checksum += (buff[i] & 0xFF);
            processed_len += n;
        }

        Serial.print("\nimage checksum in flash:");
        Serial.println(image_checksum);

        //#### END OF HTTP
        if(check_sum_ota != image_checksum){
            OTA_PRINTF("Checksum Error\r\n");
            break;
        }


        // Put signature for OTA image
        flash_write_word(&flash, ota_addr +  8, 0x35393138);
        flash_write_word(&flash, ota_addr + 12, 0x31313738);
        flash_read_word(&flash, ota_addr +  8, &signature1);
        flash_read_word(&flash, ota_addr + 12, &signature2);
        if (signature1 != 0x35393138 || signature2 != 0x31313738) {
            OTA_PRINTF("Put signature fail\r\n");
            break;
        }

        // Mark image 2 as old image
        flash_write_word(&flash, img2_addr + 8, 0x35393130);

        ret = 0;
        OTA_PRINTF("OTA success\r\n");

    } while (0);

    if (buff != NULL) {
        free(buff);
    }


    if (ret < 0) {
        OTA_PRINTF("OTA fail\r\n");
    } else {
        if (reboot_when_success) {
            sys_reset();
        }
    }

    return ret;
}

int OTAClass::setOtaAddress(uint32_t address) {

    set_system_data(0x0000, address);
    ota_addr = address;
}

int OTAClass::setRecoverPin(uint32_t pin1, uint32_t pin2) {
    uint8_t boot_pin1 = 0xFF;
    uint8_t boot_pin2 = 0xFF;
    uint32_t boot_pins = 0;
    
    if ( pin1 < TOTAL_GPIO_PIN_NUM ) {
        boot_pin1 = (g_APinDescription[pin1].pinname) & 0xFF;
        boot_pin1 |= 0x80;
    }

    if ( pin2 < TOTAL_GPIO_PIN_NUM ) {
        boot_pin2 = (g_APinDescription[pin2].pinname) & 0xFF;
        boot_pin2 |= 0x80;
    }

    if (boot_pin1 != 0xFF || boot_pin2 != 0xFF) {
        boot_pins = (boot_pin1 << 8) | boot_pin2;
        set_system_data(0x0008, boot_pins);
    }
}

int OTAClass::set_system_data(uint32_t address, uint32_t value) {

    flash_t flash;
    uint32_t i, data;

    flash_write_word(&flash, FLASH_SYSTEM_DATA_ADDR + address, value);
    flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR + address, &data);

    if (value != data) {

		//erase backup sector
		flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);

		//backup system data to backup sector
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR + i, &data);
			flash_write_word(&flash, FLASH_RESERVED_DATA_BASE + i,data);
		}

		//erase system data
		flash_erase_sector(&flash, FLASH_SYSTEM_DATA_ADDR);

		//write data back to system data
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(&flash, FLASH_RESERVED_DATA_BASE + i, &data);
			if(i == address) data = value;
			flash_write_word(&flash, FLASH_SYSTEM_DATA_ADDR + i,data);
		}

		//erase backup sector
		flash_erase_sector(&flash, FLASH_RESERVED_DATA_BASE);
    }
}

int OTAClass::get_image_info(uint32_t *img2_addr, uint32_t *img2_len, uint32_t *img3_addr, uint32_t *img3_len) {

    flash_t flash;
    uint32_t img3_load_addr = 0;

    *img2_addr = IMAGE_2;
    *img3_addr = *img3_len = 0;

    flash_read_word(&flash, *img2_addr, img2_len);
    *img3_addr = IMAGE_2 + *img2_len + 0x10;
    flash_read_word(&flash, *img3_addr, img3_len);
    flash_read_word(&flash, (*img3_addr) + 4, &img3_load_addr);

    if (img3_load_addr != 0x30000000) {
        // There is no img3
        *img3_addr = *img3_len = 0;
    }
}

int OTAClass::sync_ota_addr() {
    flash_t flash;
    uint32_t ota_addr_in_flash;

    flash_read_word(&flash, FLASH_SYSTEM_DATA_ADDR, &ota_addr_in_flash);
    if (ota_addr_in_flash == ~0x0) {
        // No OTA address configuired in flash
        OTA_PRINTF("use default OTA address\r\n");
        ota_addr = DEFAULT_OTA_ADDRESS;
        flash_write_word(&flash, FLASH_SYSTEM_DATA_ADDR, ota_addr);
    } else {
        ota_addr = ota_addr_in_flash;
    }
}

OTAClass OTA;

