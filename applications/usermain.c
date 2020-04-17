#include "usermain.h"
#include "rtthread.h"
#include "rtdevice.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
#include <string.h>
#include "cJSON.h"
#include <dfs_fs.h>
#include "include.h"

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

/* 定义webclient的Buffer大小 */
#define POST_RESP_BUFSZ 1024
#define POST_HEADER_BUFSZ 1024

/* 开启DEBUG功能 */
#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME "userapp"

#define DBG_COLOR
#include <rtdbg.h>

static rt_sem_t sem_base64 = RT_NULL;

static void hexdump(const rt_uint8_t *p, rt_size_t len)
{
    unsigned char *buf = (unsigned char *)p;
    int i, j;

    rt_kprintf("Dump 0x%.8x %dBytes\n", (int)p, len);
    rt_kprintf("Offset    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (i = 0; i < len; i += 16)
    {
        rt_kprintf("%08X: ", i + (int)p);

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                rt_kprintf("%02X ", buf[i + j]);
            }
            else
            {
                rt_kprintf("   ");
            }
        }
        rt_kprintf(" ");

        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                rt_kprintf("%c", ((unsigned int)((buf[i + j]) - ' ') < 127u - ' ') ? buf[i + j] : '.');
            }
        }
        rt_kprintf("\n");
    }
}

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

//初始化
static struct mjpeg_session session = {0};

//定义一帧图片的大小
#define PIC_BUF_SIZE (1024 * 50)
#define SEND_FRAME_EVENT (1 << 0)
#define SEND_FRAME_COMPLETE (1 << 1)

static void pkt_header_cb(TV_HDR_PARAM_PTR param)
{
    HDR_PTR elem_tvhdr = (HDR_PTR)param->ptk_ptr;

    elem_tvhdr->id = (UINT8)param->frame_id;
    elem_tvhdr->is_eof = param->is_eof;
    elem_tvhdr->pkt_cnt = param->frame_len;
    elem_tvhdr->size = 0;
}

static int tvideo_capture_cb(UINT8 *data, UINT32 pos, UINT32 len, UINT8 is_stop)
{
    rt_err_t ret = 0;
    int temp = 0;

    session.total_len = 0;
    session.buf = data;
    session.total_len += len;

    rt_event_send(session.event, SEND_FRAME_EVENT);
    return len;
}

static void camera_start(void)
{
    static rt_uint8_t is_start = 0;

    if (is_start == 1)
    {
        LOG_I("camera is already start\n");
        return;
    }

    session.setup.send_type = TVIDEO_SND_INTF;
    session.setup.send_func = tvideo_capture_cb;
    session.setup.start_cb = NULL;
    session.setup.end_cb = NULL;

    session.setup.pkt_header_size = sizeof(HDR_ST);
    session.setup.add_pkt_header = pkt_header_cb;

    video_transfer_init(&session.setup);

    is_start = 1;
}
static int start_flag = false;
static char file_name[15] = {0};
static int file_count = 0;

void wlan_connect()
{

#define wifi_ssid "TP-LINK_1"
#define wifi_key "jia555555"

    LOG_I("wifi连接中...\r\n");
    struct rt_wlan_info info;
    int result = 0;

    struct rt_wlan_device *wlan = (struct rt_wlan_device *)rt_device_find("w0");

    if (wlan == RT_NULL)
    {
        LOG_E("设备未找到!\r\n");
    }

    rt_wlan_info_init(&info, WIFI_STATION, SECURITY_WPA2_MIXED_PSK, wifi_ssid);

    result = rt_wlan_init(wlan, WIFI_STATION);

    if (result == RT_EOK)
    {
        result = rt_wlan_connect(wlan, &info, wifi_key);
        if (result == RT_EOK)
        {
            LOG_I("wifi已连接!\r\n");
        }
        else
        {
            LOG_E("wifi连接失败!\r\n");
        }
    }
}

int URLEncode(const char *str, const int strSize, char *result, const int resultSize)
{
    int i;
    int j = 0; //for result index
    char ch;

    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0))
    {
        return 0;
    }

    for (i = 0; (i < strSize) && (j < resultSize); ++i)
    {
        ch = str[i];
        if (((ch >= 'A') && (ch < 'Z')) ||
            ((ch >= 'a') && (ch < 'z')) ||
            ((ch >= '0') && (ch < '9')))
        {
            result[j++] = ch;
        }
        else if (ch == ' ')
        {
            result[j++] = '+';
        }
        else if (ch == '.' || ch == '-' || ch == '_' || ch == '*')
        {
            result[j++] = ch;
        }
        else
        {
            if (j + 3 < resultSize)
            {
                sprintf(result + j, "%%%02X", (unsigned char)ch);
                j += 3;
            }
            else
            {
                return 0;
            }
        }
    }

    result[j] = '\0';
    return j;
}


int tts(char *path)
{
    char *uri = rt_strdup(path);
    player_stop();
    player_set_uri(uri);
    LOG_I("开始播放");
    player_play();

    return 0;
}

int text2audio(char *text)
{
    unsigned char *url = RT_NULL;
    unsigned char *text1 = RT_NULL;
    unsigned char *text2 = RT_NULL;
    //char *text = "可回收垃圾,不可回收垃圾,垃圾垃圾都是垃圾";
    char *token = "24.77fdd5e29e31191ee6c060718b68da99.2592000.1589599990.282335-16279726";

    text1 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 20);

    LOG_I("文本转码中");
    URLEncode(text, rt_strlen(text), text1, 1024 * 20);
    print_base64(text1, rt_strlen(text1));

    text2 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 20);
    URLEncode(text1, rt_strlen(text1), text2, 1024 * 20);
    rt_free(text1);
    print_base64(text2, rt_strlen(text2));
    url = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 30);
    rt_sprintf(url, "http://tsn.baidu.com/text2audio?tok=%s&tex=%s&per=4&spd=5&pit=5&vol=5&aue=3&cuid=123456PYTHON&lan=zh&ctp=1", token, text2);
    rt_free(text2);

    LOG_I("转码完成");

    print_base64(url, rt_strlen(url));
    char *uri = web_strdup(url);
    webclient_get_file(uri, "/sd/audio.mp3");
    LOG_I("音频下载完成");
    web_free(uri);
    rt_free(url);
    return 0;
}

/* 用户线程入口 */
static void userapp_entry(void *parameter)
{
    while (1)
    {
        // LOG_I("Running...");
        // rt_thread_mdelay(5000);
        session.event = rt_event_create("vt_event", RT_IPC_FLAG_FIFO);
#define KEY_MID (13)
        rt_pin_mode(KEY_MID, PIN_MODE_INPUT_PULLUP);
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
                        rt_sem_release(sem_base64);
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
}

static void base64_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(sem_base64, RT_WAITING_FOREVER);
        LOG_I("收到信号量");
        char *uri = RT_NULL;
        uri = web_strdup("http://api.tianapi.com/txapi/imglajifenlei/index");
        if (uri != RT_NULL)
        {
            unsigned char *dst = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 40);
            LOG_I("开始对图片进行编码");
            photo2base64("/sd/temp.jpg", dst);
            LOG_I("编码完成");
            unsigned char *buffer = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 40);
            rt_sprintf(buffer, "img=data:image/jpg;base64,%s", dst);
            memset(dst, 0, rt_strlen(dst));
            URLEncode(buffer, rt_strlen(buffer), dst, 1024 * 40);
            memset(buffer, 0, rt_strlen(buffer));
            rt_sprintf(buffer, "key=acda67d9ac820ea200a26f73d0b41adf&img=%s", dst);

            unsigned char *res = RT_NULL;
            res = (unsigned char *)rt_malloc(4096);
            LOG_I("已发送请求");
            webclient_post_comm(uri, buffer, res);
            LOG_I("请求结束");

            web_free(uri);
            rt_free(dst);
            rt_free(buffer);
            LOG_I("开始解析返回数据");
            cJSON *root = RT_NULL;

            root = cJSON_Parse(res);
            if (RT_NULL == root)
            {
                LOG_E("cJSON_Parse Failed!\r\n");
            }

            rt_free(res);

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
            // for (int i = 0; i < array_size; i++)
            // {
            //     cJSON *object = cJSON_GetArrayItem(newslist, i);
            //     cJSON *name_json = cJSON_GetObjectItem(object, "name");
            //     cJSON *type_json = cJSON_GetObjectItem(object, "lajitype");
            //     cJSON *tip_json = cJSON_GetObjectItem(object, "lajitip");
            //     if (name_json != RT_NULL)
            //     {
            //         char *name = cJSON_Print(name_json);
            //         rt_kprintf("name:%s\r\n", name);
            //         free(name);
            //     }

            //     if (type_json != RT_NULL)
            //     {
            //         char *type = cJSON_Print(type_json);
            //         rt_kprintf("type:%s\r\n", type);
            //         free(type);
            //     }

            //     if (tip_json != RT_NULL)
            //     {
            //         char *tip = cJSON_Print(tip_json);
            //         rt_kprintf("tip:%s\r\n", tip);
            //         free(tip);
            //     }
            // }
            cJSON *object = cJSON_GetArrayItem(newslist, 0);
            cJSON *tip_json = cJSON_GetObjectItem(object, "lajitip");

            if (tip_json != RT_NULL)
            {
                char *tip = cJSON_Print(tip_json);
                //rt_kprintf("tip:%s\r\n", tip);
                text2audio(tip);
                tts("/sd/audio.mp3");
                free(tip);
            }
            else
            {
                LOG_E("无法解析垃圾信息");
            }
            

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
    LOG_I("准备运行用户程序!");
    wlan_connect();
    rt_thread_delay(4000);
    sem_base64 = rt_sem_create("sem_base64", 0, RT_IPC_FLAG_FIFO);
    if (sem_base64 == RT_NULL)
    {
        LOG_E("信号量创建失败!");
    }
    rt_thread_t userapp = rt_thread_create("userapp", userapp_entry, RT_NULL, 4096, 1, 20);
    if (userapp != RT_NULL)
    {
        rt_thread_startup(userapp);
        LOG_I("用户程序已启动!\r\n");
    }
    rt_thread_t base64_encode = rt_thread_create("base64", base64_entry, RT_NULL, 4096, 1, 20);
    if (base64_encode != RT_NULL)
    {
        rt_thread_startup(base64_encode);
        LOG_I("Base64解码线程已启动!\r\n");
    }
}

int key_photo(int argc, char **argv)
{
    // session.event = rt_event_create("vt_event", RT_IPC_FLAG_FIFO);
    // #define KEY_MID (13)
    // rt_pin_mode(KEY_MID, PIN_MODE_INPUT_PULLUP);
    // while (1)
    // {
    //     if (rt_pin_read(KEY_MID) == PIN_LOW)
    //     {
    //         rt_thread_delay(80);
    //         if (rt_pin_read(KEY_MID) == PIN_LOW)
    //         {
    //             LOG_I("KEY_MID is pressed");
    //             LOG_I("正在开启摄像头...");
    //             camera_start(); //开启摄像头传输照片
    //             tvideo_capture(1);
    //             rt_event_recv(session.event, SEND_FRAME_EVENT, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, RT_NULL);

    //             int fd, res;
    //             rt_sprintf(file_name, "/sd/temp.jpg", file_count);
    //             LOG_I("name = %s \n", file_name);
    //             fd = open(file_name, O_WRONLY | O_CREAT);
    //             if (fd >= 0)
    //             {
    //                 write(fd, session.buf, session.total_len);
    //                 LOG_I("session.total_len=%d\r\n", session.total_len);
    //                 close(fd);
    //                 LOG_I("save %s ok!!!\n", file_name);
    //                 rt_sem_release(sem_base64);
    //                 res = Decode_Jpg(file_name);
    //                 LOG_I("res = %d\n", res);
    //             }
    //             else
    //             {
    //                 LOG_E("save pic failed!!!\n");
    //             }
    //             tvideo_capture(0);
    //         }
    //     }
    // }
    return 0;
}
MSH_CMD_EXPORT(key_photo, key photo);

int print_base64(char *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        rt_kprintf("%c", buffer[i]);
    }
    rt_kprintf("\r\n");
}

int photo2base64(const char *path, unsigned char *dst)
{
    int fd = open(path, O_RDONLY);
    unsigned char *img = RT_NULL;
    img = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 24);
    if (img == RT_NULL || dst == RT_NULL)
    {
        LOG_E("内存分配失败\r\n");
    }
    int res = read(fd, img, 1024 * 24);
    close(fd);
    rt_kprintf("res=%d\r\n", res);
    int len = 1024 * 40;
    tiny_base64_encode(dst, &len, img, 1024 * 24);
    rt_kprintf("len=%d\r\n", len);
    rt_free(img);
    return len;
}

int play_test(int argc, char **argv)
{
    char *uri = rt_strdup("/sd/loading.mp3");
    player_stop();
    player_set_uri(uri);
    LOG_I("开始播放");
    player_play();

    return 0;
}
MSH_CMD_EXPORT(play_test, play_test);

// int urllencode_test(int argc, char *argv[])
// {
//     char *src = argv[1];
//     unsigned int srclength = strlen(src);
//     rt_kprintf("src length: %d\n", strlen(src));

//     char obj[100] = {0};
//     URLEncode(src, srclength, obj, 100);

//     rt_kprintf("obj: %s\n", obj);
//     rt_kprintf("obj: %d\n", strlen(obj));

//     return 0;
// }
// MSH_CMD_EXPORT(urllencode_test, urllencode_test);

// int cJSON_test(int argc, char **argv)
// {
//     cJSON *root = RT_NULL;

//     root = cJSON_Parse("{\"code\":200,\"msg\":\"success\",\"newslist\":[{\"name\":\"键盘\",\"trust\":62,\"lajitype\":0,\"lajitip\":\"键盘是可回收垃圾，常见包括各类废金属、玻璃瓶、饮料瓶、电子产品等。投放时应注意轻投轻放、清洁干燥、避免污染。\"},{\"name\":\"笔记本电脑\",\"trust\":44,\"lajitype\":0,\"lajitip\":\"笔记本电脑是可回收垃圾，常见包括各类废金属、玻璃瓶、饮料瓶、电子产品等。投放时应注意轻投轻放、清洁干燥、避免污染。\"},{\"name\":\"笔记本\",\"trust\":28,\"lajitype\":0,\"lajitip\":\"笔记本是可回收垃圾，常见包括各类废金属、玻璃瓶、饮料瓶、电子产品等。投放时应注意轻投轻放、清洁干燥、避免污染。\"},{\"name\":\"触控板\",\"trust\":14,\"lajitype\":4,\"lajitip\":\"触控板的垃圾分类系统暂时无法判别，请重新尝试拍摄物体的主要特征。\"},{\"name\":\"台式电脑\",\"trust\":0,\"lajitype\":0,\"lajitip\":\"台式电脑是可回收垃圾，常见包括各类废金属、玻璃瓶、饮料瓶、电子产品等。投放时应注意轻投轻放、清洁干燥、避免污染。\"}]}");
//     if (RT_NULL == root)
//     {
//         LOG_E("cJSON_Parse Failed!\r\n");
//     }
//     cJSON *code_json = cJSON_GetObjectItem(root, "code");
//     if (RT_NULL != code_json)
//     {
//         char *code = cJSON_Print(code_json);
//         printf("code:%s\n", code);
//         free(code);
//     }
//     cJSON *newslist = cJSON_GetObjectItem(root, "newslist");
//     int array_size = cJSON_GetArraySize(newslist);
//     for (int i = 0; i < array_size; i++)
//     {
//         cJSON *object = cJSON_GetArrayItem(newslist, i);
//         cJSON *name_json = cJSON_GetObjectItem(object, "name");
//         cJSON *type_json = cJSON_GetObjectItem(object, "lajitype");
//         cJSON *tip_json = cJSON_GetObjectItem(object, "lajitip");
//         if (name_json != RT_NULL)
//         {
//             char *name = cJSON_Print(name_json);
//             rt_kprintf("name:%s\r\n", name);
//             free(name);
//         }

//         if (type_json != RT_NULL)
//         {
//             char *type = cJSON_Print(type_json);
//             rt_kprintf("type:%s\r\n", type);
//             free(type);
//         }

//         if (tip_json != RT_NULL)
//         {
//             char *tip = cJSON_Print(tip_json);
//             rt_kprintf("tip:%s\r\n", tip);
//             free(tip);
//         }
//     }
//     cJSON_Delete(root);
// }
// MSH_CMD_EXPORT(cJSON_test, cJSON test);

int tts_test(int argc, char **argv)
{
    unsigned char *url = RT_NULL;
    unsigned char *text1 = RT_NULL;
    unsigned char *text2 = RT_NULL;
    char *origin = "可回收垃圾,不可回收垃圾,垃圾垃圾都是垃圾";
    char *token = "24.77fdd5e29e31191ee6c060718b68da99.2592000.1589599990.282335-16279726";

    text1 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 20);

    URLEncode(origin, rt_strlen(origin), text1, 1024 * 20);
    print_base64(text1, rt_strlen(text1));

    text2 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 20);
    URLEncode(text1, rt_strlen(text1), text2, 1024 * 20);
    rt_free(text1);
    print_base64(text2, rt_strlen(text2));
    url = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 30);
    rt_sprintf(url, "http://tsn.baidu.com/text2audio?tok=%s&tex=%s&per=4&spd=5&pit=5&vol=5&aue=3&cuid=123456PYTHON&lan=zh&ctp=1", token, text2);
    rt_free(text2);
    print_base64(url, rt_strlen(url));
    char *uri = web_strdup(url);
    webclient_get_file(uri, "/sd/audio.mp3");
    web_free(uri);
    rt_free(url);
    return 0;
}
MSH_CMD_EXPORT(tts_test, tts test);

// int base64_test(int argc, char **argv)
// {
//     char *uri = RT_NULL;
//     uri = web_strdup("http://api.tianapi.com/txapi/imglajifenlei/index");
//     if (uri == RT_NULL)
//     {
//         rt_kprintf("no memory for create post request uri buffer.\n");
//         return -RT_ENOMEM;
//     }

//     unsigned char *dst = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 40);
//     photo2base64(argv[1], dst);

//     unsigned char *buffer = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 40);
//     rt_sprintf(buffer, "img=data:image/jpg;base64,%s", dst);
//     memset(dst, 0, rt_strlen(dst));
//     URLEncode(buffer, rt_strlen(buffer), dst, 1024 * 40);
//     memset(buffer, 0, rt_strlen(buffer));
//     rt_sprintf(buffer, "key=acda67d9ac820ea200a26f73d0b41adf&img=%s", dst);
//     char *res = (unsigned char *)rt_malloc(4096);
//     webclient_post_comm(uri, buffer, res);
    
//     rt_kprintf(res);
//     rt_free(res);

//     web_free(uri);
//     rt_free(dst);
//     rt_free(buffer);
// }
// MSH_CMD_EXPORT(base64_test, base64);


