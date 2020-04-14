#include "usermain.h"
#include "rtthread.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
#include <string.h>
#ifdef RT_USING_DFS
#include <dfs_fs.h>
// int mnt_init(void)
// {
//     if (dfs_mount(RT_NULL, "/ram", "ram", 0, dfs_ramfs_create(rt_malloc(1024),3)) == 0)
//     {
//         rt_kprintf("RAM file system initializated!\n");
//     }
//     else
//     {
//         rt_kprintf("RAM file system initializate failed!\n");
//     }

//     return 0;
// }

// MSH_CMD_EXPORT(mnt_init,mnt init);


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

unsigned char *photo2base64(void)
{
    int fd = open("/sd/temp.jpg",O_RDONLY);
    unsigned char *img = RT_NULL;
    unsigned char *dst = RT_NULL;
    img = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*24);
    dst = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*50);
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
    fd = open("/sd/base64.txt",O_WRONLY|O_CREAT);
    res = write(fd,dst,len);
    rt_kprintf("res = %d\r\n",res);
    close(fd);
    rt_kprintf(dst);
    rt_free(img);
    rt_free(dst);

}
MSH_CMD_EXPORT(photo2base64,base64 );

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


