#include "usermain.h"
#include "rtthread.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
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


void wlan_connect(void);

#define wifi_ssid "TP-LINK_1"
#define wifi_key "jia555555"

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

    //char wifi_ssid[32]    = "TP-LINK_1";
    //char wifi_key[32]     = "jis555555";
    rt_kprintf("wifi连接中...\r\n");
    struct rt_wlan_info info;
    int result = 0;

    //struct rt_wlan_device *wlan = RT_NULL;
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

int get_garbage_info()
{
    #define HTTP_GET_URL "http://api.tianapi.com/txapi/imglajifenlei/index"
    #define HEADER "Content-Type: application/x-www-form-urlencoded"
    char *payload = "key=acda67d9ac820ea200a26f73d0b41adf";
    unsigned char *buffer = RT_NULL;

    
    int length = 0;

    length = webclient_request(HTTP_GET_URL, HEADER, payload, &buffer);

    if (length < 0)
    {
        rt_kprintf("webclient GET request response data error.\r\n");
        return -RT_ERROR;
    }

    rt_kprintf("webclient GET request response data :\r\n");
    rt_kprintf("%s\r\n",buffer);

    web_free(buffer);
    return RT_EOK;
}
MSH_CMD_EXPORT(get_garbage_info, get_garbage_info);


