#include "myfunc.h"


/* 开启DEBUG功能 */
#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME "userapp"

#define DBG_COLOR
#include <rtdbg.h>

//初始化
struct mjpeg_session session = {0};

void hexdump(const rt_uint8_t *p, rt_size_t len)
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

void pkt_header_cb(TV_HDR_PARAM_PTR param)
{
    HDR_PTR elem_tvhdr = (HDR_PTR)param->ptk_ptr;

    elem_tvhdr->id = (UINT8)param->frame_id;
    elem_tvhdr->is_eof = param->is_eof;
    elem_tvhdr->pkt_cnt = param->frame_len;
    elem_tvhdr->size = 0;
}

int tvideo_capture_cb(UINT8 *data, UINT32 pos, UINT32 len, UINT8 is_stop)
{
    rt_err_t ret = 0;
    int temp = 0;

    session.total_len = 0;
    session.buf = data;
    session.total_len += len;

    rt_event_send(session.event, SEND_FRAME_EVENT);
    return len;
}

void camera_start(void)
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
            // char *ip = "0.0.0.0";
            // while(1)
            // {
            //     get_ip(ip);
            //     LOG_I("ip=%s",ip);
            //     if(rt_strcmp(ip,"0.0.0.0") != 0)
            //     {
            //         LOG_I("wifi已连接!");
            //         break;
            //     }
            //     rt_thread_mdelay(2000);
            // }
            LOG_I("WIFI已連接");
            
        }
        else
        {
            LOG_E("wifi连接失败!");
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

    char *token = "24.77fdd5e29e31191ee6c060718b68da99.2592000.1589599990.282335-16279726";

    text1 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 15);

    LOG_I("文本转码中");
    URLEncode(text, rt_strlen(text), text1, 1024 * 20);

    text2 = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 15);
    URLEncode(text1, rt_strlen(text1), text2, 1024 * 20);
    rt_free(text1);

    url = (unsigned char *)rt_malloc(sizeof(unsigned char) * 1024 * 15);
    rt_sprintf(url, "http://tsn.baidu.com/text2audio?tok=%s&tex=%s&per=4&spd=5&pit=5&vol=5&aue=3&cuid=123456PYTHON&lan=zh&ctp=1", token, text2);
    rt_free(text2);

    LOG_I("转码完成");

    char *uri = web_strdup(url);
    webclient_get_file(uri, "/sd/audio.mp3");
    LOG_I("音频下载完成");
    web_free(uri);
    rt_free(url);
    return 0;
}

int webclient_post_comm(const char *uri, const char *post_data, char *buffer)
{
    struct webclient_session* session = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;

    /* create webclient session and set header response size */
    session = webclient_session_create(POST_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* build header for upload */
    webclient_header_fields_add(session, "Content-Length: %d\r\n", strlen(post_data));
    webclient_header_fields_add(session, "Content-Type: application/x-www-form-urlencoded;\r\n");

    /* send POST request by default header */
    if ((resp_status = webclient_post(session, uri, post_data)) != 200)
    {
        rt_kprintf("webclient POST request failed, response(%d) error.\n", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }

    rt_kprintf("webclient post response data: \n");
    do
    {
        bytes_read = webclient_read(session, buffer, POST_RESP_BUFSZ);
        if (bytes_read <= 0)
        {
            break;
        }

        for (index = 0; index < bytes_read; index++)
        {
            rt_kprintf("%c", buffer[index]);
        }
    } while (1);



    rt_kprintf("\r\n");

__exit:
    if (session)
    {
        webclient_close(session);
    }

    return 0;
}

int print_long(char *buffer, int len)
{
    for (int i = 0; i < len; i++)
    {
        rt_kprintf("%c", buffer[i]);
    }
    rt_kprintf("\r\n");
}

int photo2base64(char *buffer, unsigned char *dst)
{
    int len = 1024 * 40;
    tiny_base64_encode(dst, &len, buffer, 1024 * 24);
    rt_kprintf("len=%d\r\n", len);
    return len;
}

void get_ip(char* buf)
{
    rt_ubase_t index;
    struct netif * netif;

    rt_enter_critical();
    netif = netif_list;

    while(netif != RT_NULL)
    {
        if(netif->flags & NETIF_FLAG_LINK_UP)
        {
            //rt_kprintf("ip address: %s\n", ipaddr_ntoa(&(netif->ip_addr)));
            memcpy(buf, ipaddr_ntoa(&(netif->ip_addr)), 16);
        }
        netif = netif->next;
    }
    rt_exit_critical();
}
