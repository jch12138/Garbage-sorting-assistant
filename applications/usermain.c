#include "usermain.h"
#include "rtthread.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
#include <string.h>
#include "cjson.h"
#ifdef RT_USING_DFS
#include <dfs_fs.h>
#endif

#define POST_RESP_BUFSZ                1024
#define POST_HEADER_BUFSZ              1024

void wlan_connect(void);

int user_app_start()
{
    rt_kprintf("用户程序运行!\r\n");
    rt_thread_delay(1000);
    wlan_connect();
    //mnt_init();

}

int print_base64(char * buffer,int len)
{
    for (int i=0;i<len;i++)
    {
        rt_kprintf("%c",buffer[i]);
    }
    rt_kprintf("\r\n");
}

int photo2base64(const char * path,unsigned char * dst)
{
    int fd = open(path,O_RDONLY);
    unsigned char *img = RT_NULL;
    img = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*24);
    //dst = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*50);
    if(img==RT_NULL || dst==RT_NULL)
    {
        rt_kprintf("内存分配失败\r\n");
    }
    int res = read(fd, img, 1024*24);
    close(fd);
    rt_kprintf("res=%d\r\n",res);
    int len = 1024*40;
    tiny_base64_encode(dst,&len,img,1024*24);
    rt_kprintf("len=%d\r\n",len);
    // unlink("/sd/base64.txt");
    // fd = open("/sd/base64.txt",O_WRONLY|O_CREAT);
    // res = write(fd,dst,len);
    // rt_kprintf("res = %d\r\n",res);
    // close(fd);
    //print_base64(dst,len);
    rt_free(img);
    return len;
}


int URLEncode(const char* str, const int strSize, char* result, const int resultSize)
{
    int i;
    int j = 0;//for result index
    char ch;
 
    if ((str==NULL) || (result==NULL) || (strSize<=0) || (resultSize<=0)) {
        return 0;
    }
 
    for ( i=0; (i<strSize)&&(j<resultSize); ++i) {
        ch = str[i];
        if (((ch>='A') && (ch<'Z')) ||
            ((ch>='a') && (ch<'z')) ||
            ((ch>='0') && (ch<'9'))) {
            result[j++] = ch;
        } else if (ch == ' ') {
            result[j++] = '+';
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {
            result[j++] = ch;
        } else {
            if (j+3 < resultSize) {
                sprintf(result+j, "%%%02X", (unsigned char)ch);
                j += 3;
            } else {
                return 0;
            }
        }
    }
 
    result[j] = '\0';
    return j;
}

int urllencode_test(int argc, char* argv[])
{
    char* src = argv[1];
    unsigned int srclength = strlen(src);
    rt_kprintf("src length: %d\n", strlen(src));
    
 
    char obj[100] = {0};
    URLEncode(src, srclength, obj, 100);
 
    rt_kprintf("obj: %s\n", obj);
    rt_kprintf("obj: %d\n", strlen(obj));
 
    return 0;
}
MSH_CMD_EXPORT(urllencode_test, urllencode_test);

int base64_test(int argc, char **argv)
{
    char *uri = RT_NULL;
    uri = web_strdup("http://api.tianapi.com/txapi/imglajifenlei/index");
    if(uri == RT_NULL)
    {
        rt_kprintf("no memory for create post request uri buffer.\n");
        return -RT_ENOMEM;
    }

    unsigned char *dst = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*40);
    photo2base64(argv[1],dst);

    unsigned char *buffer = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*40);
    rt_sprintf(buffer,"img=data:image/jpg;base64,%s",dst);
    memset(dst,0,rt_strlen(dst));   
    URLEncode(buffer, rt_strlen(buffer), dst, 1024*40);
    memset(buffer,0,rt_strlen(buffer));    
    rt_sprintf(buffer,"key=acda67d9ac820ea200a26f73d0b41adf&img=%s",dst);
    webclient_post_comm(uri, buffer);
    
    web_free(uri);
    rt_free(dst);
    rt_free(buffer);
}
MSH_CMD_EXPORT(base64_test,base64 );

void wlan_connect(){

    #define wifi_ssid "TP-LINK_1"
    #define wifi_key "jia555555"

    rt_kprintf("wifi连接中...\r\n");
    struct rt_wlan_info info;
    int result = 0;

    struct rt_wlan_device *wlan = (struct rt_wlan_device*)rt_device_find("w0");

    if(wlan == RT_NULL){
        rt_kprintf("设备未找到!\r\n");
    }

    rt_wlan_info_init(&info, WIFI_STATION, SECURITY_WPA2_MIXED_PSK, wifi_ssid);

    result = rt_wlan_init(wlan, WIFI_STATION);

    if (result == RT_EOK)
    {
        result = rt_wlan_connect(wlan, &info, wifi_key);
        if(result == RT_EOK)
        {
            rt_kprintf("wifi已连接!\r\n");
        }
        else
        {
            rt_kprintf("wifi连接失败!\r\n");
        }
    }
}


/* send HTTP POST request by common request interface, it used to receive longer data */
static int post(const char *uri, const char *post_data)
{
    struct webclient_session* session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;

    buffer = (unsigned char *) web_malloc(POST_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        rt_kprintf("no memory for receive response buffer.\n");
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(POST_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* build header for upload */
    webclient_header_fields_add(session, "Authorization: APPCODE 91fc0ae1f57341a3bc63e43e84b414ef\r\n");
    webclient_header_fields_add(session, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n");

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

    rt_kprintf("\n");

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    return ret;
}



int get_garbage_info()
{
    #define GARBAGE_API "http://recover.market.alicloudapi.com/recover"
    const char *payload =  "img=aHR0cHM6Ly9pbWcxNC4zNjBidXlpbWcuY29tL24wL2pmcy90NjQyMS8zMS8xNzk1Nzc5NS8xODAzNTUvYzU0ZjEyZGEvNTkzN2Q2ZGJOYTAxNTI0MjQuanBn";
    char *uri=RT_NULL;
    uri = web_strdup(GARBAGE_API);
    if(uri == RT_NULL)
    {
        rt_kprintf("no memory for create post request uri buffer.\n");
        return -RT_ENOMEM;
    }

    post(uri, payload);

    if (uri)
    {
        web_free(uri);
    }

    return RT_EOK;
}
MSH_CMD_EXPORT(get_garbage_info, get_garbage_info);


