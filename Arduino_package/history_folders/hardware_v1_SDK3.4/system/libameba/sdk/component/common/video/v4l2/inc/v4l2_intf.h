#ifndef _V4L2_INTF_H_
#define _V4L2_INTF_H_

#ifdef ARDUINO_SDK
enum _streaming_state {
    STREAMING_OFF = 0,
    STREAMING_ON = 1,
    STREAMING_PAUSED = 2,
    STREAMING_READY = 3,
};
typedef enum _streaming_state streaming_state;
#else
typedef enum _streaming_state streaming_state;
enum _streaming_state {
    STREAMING_OFF = 0,
    STREAMING_ON = 1,
    STREAMING_PAUSED = 2,
    STREAMING_READY = 3,
};
#endif

long v4l_usr_ioctl(int fd, unsigned int cmd, void *arg);
int v4l_dev_open();
void v4l_dev_release();
void stop_capturing();
int start_capturing();
void uninit_v4l2_device ();
int init_v4l2_device ();
int v4l_set_param(u32 format_type, int *width, int *height, int *frame_rate, int *compression_ratio);
extern void spec_v4l2_probe(); 
#endif
