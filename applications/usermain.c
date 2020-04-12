#include "usermain.h"
#include "rtthread.h"
#include <wlan_dev.h>

void wlan_connect(void);

int user_app_start()
{
    rt_kprintf("用户程序运行!\r\n");
    rt_thread_delay(1000);
    //wlan_connect();

}


void wlan_connect(){

    //char wifi_ssid[32]    = "TP-LINK_1";
    //char wifi_key[32]     = "jis555555";
    struct rt_wlan_info info;
    int result = 0;

    //struct rt_wlan_device *wlan = RT_NULL;
    struct rt_wlan_device *wlan = (struct rt_wlan_device*)rt_device_find("w0");

    if(wlan == RT_NULL){
        rt_kprintf("设备未找到!\r\n");
    }

    rt_wlan_info_init(&info, WIFI_STATION, SECURITY_WPA2_MIXED_PSK, "TP-LiNK_1");

    result = rt_wlan_init(wlan, WIFI_STATION);

    if (result == RT_EOK)
    {
        result = rt_wlan_connect(wlan, &info, "jia555555");
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
