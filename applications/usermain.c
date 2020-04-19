#include "usermain.h"
#include "myfunc.h"
#include "drv_lcd.h"

/* 开启DEBUG功能 */
#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME "userapp"

#define DBG_COLOR
#include <rtdbg.h>

static rt_sem_t sem_request = RT_NULL;
extern unsigned char image_rttlogo[];
/* 拍照线程入口 */
static void camera_entry(void *parameter)
{   
    //tts("/sd/welcome.mp3");
    char file_name[15] = {0};
    int file_count = 0;
    session.event = rt_event_create("vt_event", RT_IPC_FLAG_FIFO); 
    while (1)
    {
        if (rt_pin_read(KEY_MID) == PIN_LOW)
        {
            rt_thread_delay(80);
            if (rt_pin_read(KEY_MID) == PIN_LOW)
            {
                LOG_I("KEY_MID is pressed");
                LOG_I("正在开启摄像头...");
                camera_start(); //开启摄像头传输照片
                tvideo_capture(1);
                rt_event_recv(session.event, SEND_FRAME_EVENT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, RT_NULL);

                rt_sem_release(sem_request);
                LOG_I("已釋放信號量");

                tts("/sd/sorting.mp3");

                int fd, res;
                rt_sprintf(file_name, "/sd/temp.jpg", file_count);
                LOG_I("name = %s \n", file_name);
                fd = open(file_name, O_WRONLY | O_CREAT);
                if (fd >= 0)
                {
                    write(fd, session.buf, session.total_len);
                    LOG_I("session.total_len=%d\r\n", session.total_len);
                    close(fd);
                    LOG_I("save %s ok!!!\n", file_name);
                    res = Decode_Jpg(file_name);

                   

                    LOG_I("res = %d\n", res);
                }
                else
                {
                    LOG_E("save pic failed!!!\n");
                }
                tvideo_capture(0);
            }
        }
        rt_thread_mdelay(100);
    }

}

static void request_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(sem_request, RT_WAITING_FOREVER);
        LOG_I("收到信号量");
        char *uri = RT_NULL;
        uri = web_strdup("http://api.tianapi.com/txapi/imglajifenlei/index");
        if (uri != RT_NULL)
        {
            unsigned char *dst = (unsigned char *)tcm_malloc(sizeof(unsigned char) * 1024 * 40);
            LOG_I("开始对图片进行编码");
            photo2base64(session.buf, dst);
            LOG_I("编码完成");
            unsigned char *buffer = (unsigned char *)tcm_malloc(sizeof(unsigned char) * 1024 * 40);
            rt_sprintf(buffer, "img=data:image/jpg;base64,%s", dst);
            memset(dst, 0, rt_strlen(dst));
            URLEncode(buffer, rt_strlen(buffer), dst, 1024 * 40);
            memset(buffer, 0, rt_strlen(buffer));
            rt_sprintf(buffer, "key=acda67d9ac820ea200a26f73d0b41adf&img=%s", dst);

            unsigned char *res = RT_NULL;
            res = (unsigned char *)rt_malloc(1024*4);
            LOG_I("已发送请求");
            webclient_post_comm(uri, buffer, res);
            LOG_I("请求结束");

            web_free(uri);
            tcm_free(dst);
            tcm_free(buffer);

            LOG_I("开始解析返回数据");
            cJSON *root = RT_NULL;

            root = cJSON_Parse(res);
            if (RT_NULL == root)
            {
                LOG_E("cJSON_Parse Failed!\r\n");
                tts("/sd/net_error.mp3");
                continue;
            }

            cJSON *code_json = cJSON_GetObjectItem(root, "code");
            if (RT_NULL != code_json)
            {
                char *code = cJSON_Print(code_json);
                printf("code:%s\n", code);
                if(rt_strcmp(code,"200") != 0)
                {
                    LOG_E("网络错误");
                    tts("/sd/net_error.mp3");
                    continue;
                }
                free(code);
            }
            cJSON *newslist = cJSON_GetObjectItem(root, "newslist");
            int array_size = cJSON_GetArraySize(newslist);
            cJSON *object = cJSON_GetArrayItem(newslist, 0);
            cJSON *tip_json = cJSON_GetObjectItem(object, "lajitip");

            if (tip_json != RT_NULL)
            {
                char *tip = cJSON_Print(tip_json);
                if(text2audio(tip) != 0)
                {
                    tts("/sd/net_error.mp3");
                }
                tts("/sd/audio.mp3");
                free(tip);
            }
            else
            {
                LOG_E("无法解析垃圾信息");
            }
            
            rt_free(res);
            cJSON_Delete(root);
        }
        else
        {
            LOG_E("URL解析失败");
        }
        
    }
}

/* 创建用户应用线程 */
int user_app_start()
{
    player_set_volume(99);
    tts("/sd/welcome.mp3");
    lcd_clear(WHITE);
    lcd_show_image(0, 0, 240, 69, image_rttlogo);
    lcd_disp_str_en(20,100,"Garbage-sorting-assistant",WHITE,BLACK);
    int result=0;
    LOG_I("准备运行用户程序!");
    wlan_connect();
    
    rt_thread_delay(4000);
    rt_pin_mode(KEY_MID, PIN_MODE_INPUT_PULLUP);

    sem_request = rt_sem_create("sem_request", 0, RT_IPC_FLAG_FIFO);
    if (sem_request == RT_NULL)
    {
        LOG_E("信号量创建失败!");
    }

    rt_thread_t tid1 = rt_thread_create("camera", camera_entry, RT_NULL, 1200, 1, 20);
    if (tid1 != RT_NULL)
    {
        rt_thread_startup(tid1);
        LOG_I("拍照线程已启动!");
    }
    rt_thread_t tid2 = rt_thread_create("request", request_entry, RT_NULL, 4096, 0, 50);
    if (tid2 != RT_NULL)
    {
        rt_thread_startup(tid2);
        LOG_I("网络线程已启动!");
    }
    lcd_disp_str_en(80, 150, "Ready!",WHITE,GREEN);

}