#include "rtthread.h"
#include "rtdevice.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
#include <string.h>
#include "cJSON.h"
#include <dfs_fs.h>
#include "include.h"
#include "http_api/http_api.h"

#include "app_demo_softap.h"
#include "video_transfer.h"

#include <sys/socket.h>
#include "netdb.h"
#include "lwip/netif.h"

#include "player.h"

//添加图片解码的头文件
#include "tjpgd.h"

//添加文件操作使用需要的头文件
#include <dfs_posix.h>

//定义一帧图片的大小
#define PIC_BUF_SIZE (1024 * 50)
#define SEND_FRAME_EVENT (1 << 0)
#define SEND_FRAME_COMPLETE (1 << 1)


/* 定义webclient的Buffer大小 */
#define POST_RESP_BUFSZ 1024
#define POST_HEADER_BUFSZ 1024

// 创建 mjpeg 会话的结构体
static struct mjpeg_session
{
    int sock;
    int connected;
    char *buf;
    int total_len;
    int last_frame_id;
    int recv_frame_flag;
    rt_tick_t old_tick;
    rt_event_t event;
    TVIDEO_SETUP_DESC_ST setup;
};
extern struct mjpeg_session session;
int webclient_post_comm(const char *uri, const char *post_data, char *buffer);
int text2audio(char *text);
int tts(char *path);
int URLEncode(const char *str, const int strSize, char *result, const int resultSize);
void wlan_connect(void);
void camera_start(void);
int tvideo_capture_cb(UINT8 *data, UINT32 pos, UINT32 len, UINT8 is_stop);
void pkt_header_cb(TV_HDR_PARAM_PTR param);
void hexdump(const rt_uint8_t *p, rt_size_t len);
int print_long(char *buffer, int len);
//int photo2base64(const char *path, unsigned char *dst);
int photo2base64(char *buffer, unsigned char *dst);
void get_ip(char* buf);
void lcd_disp_str_ch(uint16_t usX, uint16_t usY, const uint8_t *pStr, uint16_t usColor_Background, uint16_t usColor_Foreground);
