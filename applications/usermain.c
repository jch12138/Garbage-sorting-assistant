#include "usermain.h"
#include "rtthread.h"
#include <wlan_dev.h>
#include <webclient.h>
#include <tinycrypt.h>
#include <string.h>
#include "cJSON.h"
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
    unlink("/sd/base64.txt");
    fd = open("/sd/base64.txt",O_WRONLY|O_CREAT);
    res = write(fd,dst,len);
    rt_kprintf("res = %d\r\n",res);
    close(fd);
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
    // unsigned char *dst = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*40);
    // char *data = (char *)rt_malloc(sizeof(char)*1024*40);
    // photo2base64(argv[1],dst);
    //rt_kprintf(dst);
    
    char *uri = RT_NULL;
    uri = web_strdup("http://api.tianapi.com/txapi/imglajifenlei/index");
    // uri = web_strdup("http://192.168.0.6:8888");
    if(uri == RT_NULL)
    {
        rt_kprintf("no memory for create post request uri buffer.\n");
        return -RT_ENOMEM;
    }
    // rt_sprintf(data,"key=acda67d9ac820ea200a26f73d0b41adf&img=data:image/jpg;base64,%s",dst);
    // rt_free(dst);
    // char *obj = (char *)rt_malloc(sizeof(char)*1024*50);
    // URLEncode(data, strlen(data), obj, 1024*50);
    // rt_free(data);
    //print_base64(obj,rt_strlen(data));
    // print_base64(data,rt_strlen(data));
    const char *data = "key=acda67d9ac820ea200a26f73d0b41adf&img=img%3Ddata%3Aimage%2Fpng%3Bbase64%2CiVBORw0KGgoAAAANSUhEUgAAAJEAAAC%2FCAYAAAD%2Bfj5lAAAgAElEQVR4nO296ZIk2ZUe9vkee0RmVmbtXb0B3UPODMAhMDOkkUMjf9Bo1APITH9kph96BuktpFeQfspkJGUyjSgOaRxgwJkhlsagAXR1d3VXde25L7F7%2BKLznes3wiMqq7vRBaDTA3nKvCLCw93DM%2B4X31nvuc5kMslxKZfyCuJ%2B3TdwKdWXSxBdyivLJYgu5ZXlEkSX8srif903UCXJ88%2F3QRzH%2BS3dycWSSya6lFeWSxBdyivL74w6u1RFvzm5ZKJCvghkl%2FJy%2BZ1hIsrnAeWSib66XDLRpbyy%2FM4wUcZNyIZc5Mr%2FruxwLDPJozIRySgvNvNG8WBOnHOVa1%2FYgx2zufKbdJ1ir1P8W7kW1o%2FxKgGiL2OvlNXR6vEEz8TJkcoApnkCL52hnjoI4hRIZrIlwHgEzOR5zE1eZ1kx3vIou%2BRkpBM5txliVhOghC6CWginXpNvMQS8AAjk64wCzBwXiWyB48sXTCDl8i8jVuU5yb%2F6QCp%2F35UAEeWreFf2HIJImahgDFeOjU9OsffhJzh6%2FAjpaAhvMoI%2FjeGOx3CHMfIsQSas4uQZAhY6xLIJ8PyNFk4bOQbuFJ7vwW%2FWEdbb8NsdbLz5BjbefgvotIW8PPN5ufBeRgBlhJGQVfAb%2BHZ%2B%2B1JJEL2KCAkhoMrKZrKliNIMh58%2BxCff%2BwGOHz5CK%2FCEqkY4fvIU3nAMbzxFlqdIPAGcnFeb5vCEjbqdLbjdFs46DsZRilrkK68Mh1McT2e49cffxbejCBvf%2FCbcUDgo42dmBZjluVt9BjpP1gJEX%2BRZ0YQJZEC9lKpJVFV%2FgGc%2Fv4snf%2FcBImGf%2BkYbjjwOz0bCQhO4k6lorxSZ78Lj%2BbEBUbMtoMoMuzT9AM0wRJ6kqgpPjs%2Bw%2B8E9nP7%2BU2zeegO%2BG81NJuU%2FT67kuGorVT2YsPp9VwJEr%2Bp%2B04B2BUCeGr8eUgHK6WePMHj8FJHYM6PBEL7YRZtOIIOdyCFiPzkZYrF7XBn4SJ57Qjm%2BXCOSa8Vpivh0iNGpgEuM7o4AK9zcwf5MLB8BImIBa%2BYqaBQxwn65PHe8wtReM0KqBIheVfS3TwO6GDwnFXtnSBtohrZfR4d2ithA6TiGMxGVJ%2Ftzcd9csWsIIiRiGBMXco00jeW12NFOAl%2FAHTlUaXKc66MjxnUkgHIS4wMKavhhagvxn7t%2B%2BFGpDIhehY1Uq%2FgyuOKV8SpJFosDFmMaT5HUYnGqanCDACNhI6onhyqKOlDUWe4bnUQ15EaevKahnunmeb4Cycmo5nI5XEAjbJXJdTQARyISEGYCrkxYKJdjvSW3fz2kMiB6FWGYJxFQ0CLxRC%2FNhHjSSNRLzdPBTehH0XuitnPMYCttyLEOmcgzMDqdDFHrBHJuIMcEQlA5JjMBXZrLNeQ6co2psFwqRjlDRjlJjJ8tjJSqcy9GOtaPjX5nQOQI06S5MWvHwkJ9UUtTDrIwVOwQYKJ0xGUXlMELZcCFhWhY06PyOep8q9NCIl7XmZwfY4aNqI56WBfPLsN4mmImtlKmx3oaeMzk8wg0enkpDIjztYPQGoHIxoTK8aRFnEiMYWGJWNRZQwbXb9TR3NpE1OkgaLTR6nXFIB7gdDSCI0AKwpq66OSXTNRUIGoro1oKIrR3dhD1RPXFQwQzUWmpMFAsgEynAqSpgsf1zddKxeWR2eRRY0RrCCDK2oDICm2nFwKTjBUKe7jilpNZWp0etq9dx6GAqNnuYvvmTZw8fY7Z8z2xl2TAZ2IbzWYYiGpimHkmqgsCrt71axpUdLohasEGrtQaiMQje%2FbxQ3z8dBezKELUaMILwrlr7y3ubG0TlWsHIooF0vxR%2FkVeZIxgAYYraqi7tYNAHgVaSIVhBmIQ74%2FGcEYT8bCgjDLS4KSwGLlEQORSZYn3lgwysbHEyhGHb8OPRO2FogJFrcn13GZbvtVIw%2BT8fE8%2BX8xxYrDI260fG60NiKz3ZlmoDCT%2BY7KBSddkmtHrFu87ElvIw9EkxpF4Y3GjgeDaNQRxgrYXwhc2aTBhJiBq5XTVxTba2kD96g6w2RAQJYjEzsqFiabhMUZihPvCRIhqJhGrVOQok4kxJrsc83oNZW1AZKWszuZAkpez4UxsGw%2BhMEY2HYGKprVzXW2kQIDR2erCu7oJT0DWFGbxwwBjGM%2BrmQeqiuq9TTgCoONwitl0iKEAZZQM8VSu93h0huu11%2BDWfI05aSyAnp2tDiC7Ofni9RqJs67TqMt2kSPuWZ4URCDj2j84xtGHn2L05LkAS1iqHmCaTJAxCDmeIEjpnrsYZIaJuvJbC4WNxnLNvBng1BMQiS7batSwFdQQCfCmkwR7WYYr734T3Tt34Aqz%2BWKDuRqxJoLkWnTzGIBcMxStHRNZWTWwncLCZfoMooa2bt9Cq17H2eBU1JHsC4WJWAs0mShr5HyeG5sIeShA9LFdbwKNEM2m%2BG1ODH8yxtnhMaZyjVguvPnWm%2FCvXcFY2MgLPPjy%2BQHDB2I%2FMaHrM63ytXwbv1lZWxBRLJC0piwzTJTKk1qrDr8W4fnBLh4d7%2BPK7evobm8IoPpIPXHvaShHAUJhDzelOhQEZmIehw1MxcyeeCmiVge1bhuTJMPB8yPsnxxho%2FMHCDc6SLxAI9WJ0B%2Fzdp4AKVMDu7ivr%2FVb%2BfXLWoOIYg3ucTxGKAzkKDMwGepjxqBiu47WRhdhrY6Gmiy55sxozgRFAtXLmQVxBD5ilLsBmmJAM8WhLny9jURU10H8DBNhrVqg0NOgpifHqIld1BOlaap22brJ2oPIis88mAyvJ4No2EncdN24l%2BKgJp6VenPFBqec5yIIEgWgW%2BTLNPosnhxqcp5sZB9TKkKgZrJ5hU3NoKOn4FxH%2BZ0BUSAsNJvFCiACIY5jsY9SVTOz2Uz38RjXNeWrBkilCwjYfAZ7mLqY21qO2MwZNja3sHN1oKBLkkQNajIYGchxTYiROTjXXc9w4%2B8MiAgK3%2Ffn6u3k5ATT6VRc%2FlBBwffs%2B6ruLBupmBppvm%2FVUlZULHJfQ9TZ5uYmJuMxRqMROp3eUtXBuk9HWs%2BfxkvEMgEHem9vT0FUFw%2BtJqroPAAtNsMi5fxcypIPMg3VoJzfbrcxFhD1%2B33Zn86Nej2muO66yvr%2BZS9IkYyVQT04OFAmorRaLd3sIK8mcMubBYPHzXPVvvIDw0TdblePOTs7UzDZc7J5jfX6yu%2BMOjPjmAtTDJSFaBN1uz1RPR1lo1XwlNWZfY%2FM5XnGLmJ232op7ovEYyMjEUAEKdmLr8vXW1f5HWIiM5jHx0fKQmSUTqeNZrM199heVGNm4O1zAkdnbSgjGVYytlSgtlVH2IhG%2Bu7unjLSvBRFI9brK2sDovJAldWI3ShHR0fKQgRBq9VWBqJaoqFMG6d8bPm5FXpnFlAEnlWBBFQQhHo9qrY4ns4N9%2FPub91kbdVZGQTGpZ%2Fi%2BfPnyhDNZlNUWUfZg5LZyYXZMvssG8N58Tp%2FwVayLOYLsGhfzWaJGu%2F8LIYNfH9tv2aVtWEiDjAZpWx%2FlJ8fHR2r58TXtFXIGATRi%2B78Qn5Vg5jsxOs0Gk19PD4%2BVhvJfsa6GthrAyIKA31WbZTLQWin7O%2FvYzAYzOM6BJK3lIJwsOjvsDrYX27wrYFdr5trE7Rl2%2BgSRBURyygEk1VHBA83CgHEjdN9TLwnLQxmI6sDbtTVl20q4apdxHQHP5v3YF3%2B8r2tm6wViKw6sUa1U6QhqFaSZKYDTJuFbEHhcYa9mD3LzwXQryKs3yY4aWiT8aguJ5OJGtlW1a6jrA2IbE6MwgGzqmo4HKpXxvEjAxFE1tAliOy2OsCrXtqXYqLieF6fBjXBasIKxwqmdZW1ApFlHpsno9AuOT095RFqB3FgyRblFMeya798zV%2FtHqBeHj%2BbnxNo6Ymjrj7V6aU6u%2BBiwUAWsoxEQFGVMDpN4HBQjcozsR3aMF96YNlnyGIqLzqjzTcsqVFbEWCBamyj03NjV0sfUVF1V3kQrea17EahKtvd3VVb6MqVKxofMmCDBhdZH7R87qL8YzUORMlSk%2B5QBzB3dSYZN5dNHVjIpolaTwOYOcthfVfsIl9APFE2PDs7USAR6AT4qpSDk1WSyoNoVcpuO20higXIebKcM3tx3wvHZy%2BPJxkQLqdKLCtRzY1G4%2Fk9nseAVc30V%2FOuP0c4QPyVU4WRhfjLp41i3e5yjdD5BjVWXp%2BfBjnvtbk%2BXznFZxpWpJdm3X2ykE2bWLGxLa%2BipbNrByI7sLSFLBNxEO2glut8vsg%2BeRkjnTffn1JO3FoAeTrXzRS%2BDQb9ucpavQ%2FKJYi%2BZuEvnMLoNAeDLEQ24mAZT8kvBsn0GV517ZdtoMV1V4F2HjO9yGaGWSxorfqiOqOdZs95WXiharI2ICqnOwgo1vRw8GiP0LVnlr2szr4o%2FnNelPo80JT3le%2BhXFrC51bNkh0J9PK5VXf91wZEdiCotqgy6A1RjRBApvzVeE%2BU1arDBShexiznq6%2FVffZ6lmHKKRgbWmDgkQxpz3tZ8rdKslYgsoNFEHGgbOTY1E%2B7%2BCpj9aukQAx7LVipHMi09UdUZ5aJ7H1fguiCCAeI6oID9PjxYwWOzZPZInzGeBgbstn%2Bcs3Qwj7RIFABBGMfmdjQIln7MjvG8wxQbaUjP5csaO%2BNGwHOigJKWe3Zz6%2BirE21VFl10DOzc8isUWu7vKwWkq3Kwk6xr1fb1ry8XtruNzEpt%2BT2O3OWIpDo6tsqg3JcqaqyNkxkVQbZ4vDwUEFkY0R2EH%2FV6533%2FMudax6X3X1nPreNyVjrRX4eoKsiawUiDhbVBZmoPCnxV7nGOXs%2F570vPr%2Bc6OU9cWPprI0X2XIU%2B7yKsjYgssIB4i%2Fd2kJkJMoXYeDLxH%2B%2BSJbPOy%2FwaJKxrCw4r1Ctqmy0NiCyA0B7w0aJX6VIfhlAOb5MiexqqKA8M8SAyUSkyUIE%2BqoauwTR1yzWnSaIOKXZFoUtUglfHgSvYg%2BV78eyUbm6wHqDtInKmfyyWquarA2IKBwIxmHo2tui%2BV81H%2FXrMKjLsjoR0l6TILJpmfI8%2FypKZV3886LK3KgqyEQUO3B26nMOEwNCUZSGYhkGp1RHVF4FyFnp0bkKiPJnGMPe%2BVw247FkSKpaO32pDKKqloJUEkQvM3ztr7vX6y0ZyEbVFYFEh3ZIbpaNmoNixcAtQ0efFteBNz%2B2nBebHzqP%2BbjF5y3SK%2FZ41nnbtIyteqx615Dq3vmK2FpmGxUuB%2FI40%2BO34T4vqiSNPWRf2zKUcm8jG13%2FopKUKsjagIgDYGdUlGd72JRCWVa9oF%2BXV2Tnr5VTKRSCaNU2s2kQe0yVS0LWBkQUqjJT9hEsDZqNWpex8kLnj19DT9fzos829WKYcfHcslE5GHnp4n%2FNYm0PM425vpSTMnk0U5B23mD9%2BpaxW2aS84KN2hjL9%2Bf3awG0XLpbLVkbEJXjLDZvZnNTdhbHF3lary6LaUuLIKM7Z6fyfDfeX7kk5NImugBiGzcwncDn2jO6SHkQXGZ8frMqw7j4y7aR9eDKrrwFkHawLcp6q1wKsjYgonBg6JmVO7vyMUnSed3zufJrBJZhlKy4rC1BWVQwlr0x3q9lH%2Bu1VVEqGSc6T%2BwgcYIixbrVpgeRo7%2F6LE9eUBnWHtJ%2B06%2BoTiwD2ThRkiwYyN6jtdl2dnYU8Lavo23dV0Wp5l2fI%2FaXbUtAOFi2E4dRZ795e8NUNtrYlEnaWhvJuPSzuWtv1ZltxG7ri6poXK8FiDhQdkCsUc1HG4dh6xgF0YqLvypfOIBfgEPrhS3qqxe2jukLmZ57XpXde8pagIhiPaLy9BwLKMp8kFbzXi9JYXwVOW8GyfKERrr3mPdHKnfxJ4NWFUhrAyKKLY%2B104asd8b9puj%2BfCaw8kWD%2BEUYowFvr2NzZxRr7xjbKH0hq191Jlob78zmqGxwsTzvy0SJ7apBpcRr%2BfmvZRDzeaJ3dQLjwlZa2ETlmqLPU3cXXdYGRFZt2Roiej2Lwftt%2FcpNz6NyPKrcjY22WdmNX40NXbr4X7OYX%2FKsmOfF1nq1Iq1g5sbnpRJXB%2BfbyGU2ys87zskWNpVTHMHH3Lx2vSI%2Fp%2F2PzFWMd7ZIttqp3TZJbHN8VW3mQKkkEy0NtjYxp3qYIoxMHyAO5mQ61kXy4tnUlIKwoRVX%2FwEL0ezq9LkWpWmkWUvWMl1xMRdAzJJYa474DY0mI32uYLSMosHBRF%2FrsniOWclcr2XJT17PBNhc9ZGL9HExGZ5LwLD9nnX%2FrVNQVbuokiCiOPYnX3BFvR7pIA%2BHZ%2FpIJmKZLI3p%2FsDsW5wDmEpH4%2FYrQIpOIWCejfEexmx8V9eM7Q8HGI6HGpA0iVRHN83J8Rt0CSMTj5pp7VKqgOQ%2FgmgaTxELiFGAlVOaWAtuJxPYNE1VZS3UmRrTYaD1RPsH%2B4hqkQ6krXCMohCe75t4DZkKZslOBZGAzElzjIW5hqMRavX6PHLsJC6mDAoKAEbTCUI%2FREbGKPJwc4VXNPyc5WnhxouacsUbE49wxhSInDtjoVxujHvb0IH3XW5UWtVJjGsBIgpVBEtPufSUbf1rm4Bq43NhgYR9GrmCokaGfeOzMymqbOQhZCd8MoNd%2B4wrCQkbNVpcwiECAk95LJu774WNVUybpholsGJRc9ksU4yxO58jasxJuS9V9Uf22djY0FrwxTRvZx5xr5qsBYjIKGYul2mWwHU8qM5u3LipSyRwqMNGHZNZrOrGD0SNFHEbHURVM9DF8KZiC6WJiXC7E9fEl6jmZIDJRjrgdjFhZ7l9sUarOVlAGDFmOoNqSs49PTnF0eEh4tEE2SjBsdwfQWQZyEpV66zXAkT6%2B5cB6ffP8N577%2BHRo0dqr7z77rvY3t7WQZ8IOxzJYJ7JMRYwNghIptJiMc%2BwEjvLMg9mSzn8wm6ZiYoMheUaovLqdWEnYRgUmXmeYxWRzcjzmrEA99nTZ7j%2FySc4PTyBn8lnOh6%2B9a1v6VaWSxB9rWKWI9%2Fa2sLNmzeLYv2JZssJEtpBXOT3%2BOQYhweHYufEqsqoltxifddFNt8pcm9UK4ZlolpNryn8hLowGm2tXreLRrOpACNTkXGoJgPP2F68duCbiHko9hqX94zcAO1aG1u9Dbzzzjt6f7SPLCNWVRwxRqtZTgcUiVVrn2R49vwZHjx4MF%2Bvg%2BUWtDFiUWGx7NsTlXJ4eKRLmbtq1HJJ88UCMW5hHxEEoYCIbjwB2BRba8I6JYfLXIVoitdHe6YhbESDXlWcnOhqDozAETUmn%2Bd6humSWYLxcMRbRCsS4LnGfmPZig2K2sqDS8P6tyjL872gTGBnlJpkpimT1VWF5DmNaiKkEZr6Hao4jfVoINLUFQXixXE%2FVVCtVle28sTTCoXlCDzGebif9lSr2dJmojw%2BZxWleG6MR%2FF1sNJeeDaNMW6MEU%2FF%2FRfg0uZi4wkC0bYntk0oqiiVBNECQIu0hl%2BoEVs9yN226IsqK5UBdGa52iQa4MtiBZ5dUVptmyQzs2TVjk7ohgmb%2BJgRnMIe8Xgqbn6kx%2FhUg1yqIZVjJ3Jt32Tlwa77ZCGbueeCw4lcR%2FY7qVkCnUzJYCPjWFRptm9AFVmIUk1L7itILoOYk41kcwWDnhi3VCsEGEHD%2BI6jxWEJWsIQfE33fBLPsC%2FeVKrA9JRJqOp4Dq8XyHFtYSWNgRN0jF%2FmhXrTDfro5EUVZVE8Z%2FtKzu8vzyubO1s7EL3s10zAcMDFD1MAceN6HGq%2FUJ2IyuEYNgQQMWuCOFNEWOjJ3i7%2Bj3%2F7b%2FBI7C310sBlFiJRj7kCbtAfirqa6bVcfp1c86MAkdncxaOCaAEYG2i0933ZFeQCSHlAVjePC7c4iz%2FXLHAnA8%2FyEb8IRjIwWBOgiPHcn01wGo%2FwNz%2F9Mb7%2Fo7%2FFn%2F%2Bnv1AX3w08fRxMRmJiOcJUU02L0ADHS9YPsVIkWub9k8r5svLq1lWTat7158jLmIjDx4FOBAAJjWm1vH24oQDIFw4R8Hi1CFNH1FY9xChP8IP3foRnZ0doX93CX%2FzV9%2FDLex9jLCqI15jMpvDlnJp4WBnDAkV0OnOLzVnecqPt9D5sg3YLGtsRpKqF%2BmsHIsp5TOQSMGQdek70zOQvZ3ridDzE%2FukxDvqnmHkyxAKqvbMTPNx%2Fjl98%2BjEaWz28%2FvffxQQp%2Fv1%2F%2Fo948OQRUs9B1Gpgxmg1v0Hx4DQlImBK6SmWgbP0uAglfF6vgKpJNaH%2FMnkpC8GoGnbVF5A47M4hL5nGODo9wfPjQ8TiZl13UnSvbOGTh5%2Fhpz%2F%2FGTo7WwIeMcRFvd148w7e%2F%2FAD3L5xE%2B1eD9%2F8xjcwZKplPFIDm%2BzW5gLFnK5dirwpgIrH8gxZuxLSatVjFdmoEne8Gg3NnXP25%2FY9kwdDiQX4nEbwJJ0pWAioRBhkyjIRAdKxsNCJgGEWedgQ1%2F%2BXn3yEs%2BkI9ZZRVR%2Fdf4Ct7U0cDyf44fs%2FRWejh2uv3UAgfj2BOB2NcXR8pNHyjrjtNr%2BvS6MVJOOUJpvwXqZijEfRDCZMYe6fHlsVpw1VQp0RFKmWb9heZ47ZCqCkRWmHsoywTcb6IM9VdtBSDNo9kY9UfjIzL0PsChMJYaSBvNWto3ftCvxmgEE8xN7xAR48foCZk%2BDZ4XP0xwPce3APP%2FrZT8XYHuH50R6%2B%2F1%2F%2FCz55cB%2B9Zkcj5t0rPTS6TRydHQvQzlRVupGvyMkEuHJHqAvDNAJ2tA0RhDW9%2F25vE7O4WB0pzkQjhnMPbmkDXvwlXSCpBIi%2BapV0XjopTmJ9HUQRDkWF3bv%2FqZZ9NNstnAgTbW5todFo4Yc%2F%2BRGmomY67S7u3HkdrU4bb7z1FsJ6pK7%2FULyxYzn%2Fr37wfXz67D5aPZae1NTWSoTZhvEEIzG6WS3AcWc%2Bjc6%2FWOMaqdagJlWYzkzJi5m3JoLuOC%2FJn11gAFGqASIGB4VQfNm8vNhQPGbFpq%2FNqqyqOhjkgy0gK1xozxSMMV1Ri0KNHU3ZACInE8zwvthB9z%2B9r%2FtZThsKyOpBiG6zhSSeaZqkLaCr1Ruivo7xfG8X7Yj9IR1Ni8zEpmGidyAu%2F2gyMQFKzy6zXtRZ0xhnqSzXhk3FJgocZVeP5U1O9vm%2FlgsKpkrYRHNZZDvMr3elvXRR1szOikVS1Hg%2F3MeUBut6nu8%2Bx77YL01hmP7ZqSZHr%2B1cw3u%2FeB8fvP8LvPb6HVx%2F7Za47xHSSYxY7BRfrvFH3%2F42nt67LyDqiEFs6pa%2B95ffx5Xupka4mZQlQHg8O9gyUh3UXY1uO4WdpkWzjlHDrLZkqICSsvmE68yL4V46k%2BCCSiWYyJQTYmEjZIvXyjjFfi13zk1Rh0ltGACRkVIBC3NgYzGCmVFn9Jp5q82NDXk9xLPHj3Hj%2Bg3konLuCpiOBSRUQxNhlWePn%2BDg2S4Gp32cnZ4iEDbb6HblWiN8ePcujg8P0Zf9OvdfPo8J3IkWpsXFvlwZSSdRmhVn4AmIGLfSP0XtOrDIaWEGrerwCwyq6jLROTKvCVqZKs1XLEJj9p3qiIMYhSEiMXIZMxoIAIZnfTSEnaiGstkMk7MhdqcxHj5%2BBIelrkjw5uuvIxYANhstUw8t24P793H71m2NJLKQDUyXpHbyopkdwtu2ndLA5GwRWCSoDWDsfZvfhovFlKVf5e%2F%2FuqRaIHJwrpW99Mt1i591oULUw3ZMr2qywnQkLvlkrCzBRGiz28GNq9fxJ3%2F0HUzFbmFRfsK5GzLQ5JBeo42GqKpUDOXIC%2BCIWrwqRvhGp4dNJl4dF7fEtWclAN19TaoWLW1YWYBiSvW8pR5M5zYCKuE9iTHOSsfyPDcLpKpIdUBU%2FlYtiPIiFlPMH8ztrFNn2axQJpDBbtUaykhkjSgKtHA%2BEbbZ3txSz2nvaB%2BD8QgHYjQ%2FefYMZ8O%2B8bzknK2tDWGiN7Czta2u%2BpVeT6%2B3KY%2BNWl2TswlLTLQMxFWmo2Hu6dySTD%2FTKdjL047YRt3qLEeGJQiugktfqrkuGANZqQaIlN8XQcT5NB3HMg3U1vDDAL6AY5qa4jAWlGnOjBMGXQ5sgC7LPGBmcvB82k6%2BXGBD9s%2BEnaiKxkENN65soysMNBB7ibXbfebob%2BfCZCPsXO2qQc2aIgKIoMjF5nIEKIHrm%2FptNrrKjNFGsHhFJDqW64%2FEME9peFOtub7%2BSfTeXC%2F4er7fV5RKgMiwzErcrRyTk%2F9GwiAEklvkpFgea5tcuYUq4cA2xT3nc5bL8tfPpTa9IhTAUEGv2UYogzkcNXF4ciw20BgByzxEje09fYah2EPbzS6c7Ry1mo9Qy0scvRav48r1WO%2FdYEc0uU6oZSCpKV4TcGfCfHWufsSOJepB5spK9No465Zz2%2BZ%2FIzCPZl9kqQSISs7YS%2BmeBfIcEMZzyAYaLRZ1xSBfI6yL1zU2tc1Fn2uyE0tmNYEqrFSXwdtstTXiPRAPbp81Rv4Im%2BLSd%2BtN1Oo1rW6saYXjBON%2BH50wQk2AQpvG9YsuI2Q88bxCuR%2FGtejtZcJSqYCHRjXPZ9OHVO4zZKs9dpBlWm%2Fe7s%2F8dUuG9dIfffF0WiVARFnNAlDKdjZrhQgkMkOWzVSl1IQV6qxbTYyq8dVDCxREzIbMMhP801qgyNgro%2FEYk8EQM2EgAuvW9jWdu9bqdEywUra62EQse22I2muJOgsJAPl8loY4xWLCVIucRuQzKq1BokzvoSGgi2dmqnVzo25qnNgO0LXLObyEdpyXvvjapTIgsuKUHm1Sk480PwKhlUhYIaG7LYMUCRFFjGaLi%2B4VEez5pEMaQzlLYOVYOc8NjUqbCsuwFpqqTR9hQLPT3VCWoqoiUDfbPWzRuBb1qDXbwnrEq5mCZMpgncxE0TUPy9Wsp6YzCG2hSK5Zd4PCizTHV8kjK0slQGQj0ZbirW29CDQ6CAU8Jyd9jI7PEMkgN2uRgEogMMtB05fz5NUY55LkttA%2FNx6Ridt4ZlKi2DJNYZfjdkgoIVQAACAASURBVBf7hwc4EU%2BNzMTQQK1RR3ejiV6rowDiI8ts1SimoxUnyma%2BztoQO4nqUpjIZcqGjCMsyBptoSJloOnpAJHfgyPOAEuS0qJrSdn2O1%2BlXSypFIjmrwu95lhDiVOdJ7EA6ASPPrqHwckZrl%2Fdxjfeehu1Vk9nWzC%2BQ9AQRDkWILLBSDIIa4FcYZa62DpbG5vY3trC7t6emQgpLvs18di2r%2BxgixMXo4YY3K5GwjWHJypyzEmRdOVNCaMCVI1tN1O2GZ6e4dOP5f4OjzS1snFtBzc2uua%2BZqm2SPaLXFtZfS%2BAdLHUmJXKgMgUgJxTJpGb95CbP2b%2F4SP88Ad%2FjU6jgdE%2F%2FTP8%2Fne%2Fi96mACn0EHB2xtIVTZRb1SJnalB9aaMhziurYbvbw%2B1r1zWyTDuKObI6DXMBlHpUiWGgiPEg3zSIUG%2BQBj2nIxGwYv%2Bc7B3g8cef4KOf%2FRw%2F%2F9FP9HP%2F%2Bb%2F%2BVwjlM8yvgzNREm0e4ZTo5kXiuQTRK0imakLthyISXRQKwjzIHnG3HTGoNwMPV1lceHiIpz%2F%2BMcLhGXbu3IHf62LAplSM7TQbOs%2FL1aYOuelJlLuaBGW9T8IOHgKoSDyurqg2t9HUzD1ZJ5%2BxydUYmdYuRUWfSN5iihrVl6i02XiEqajARIz0%2BPQUDz%2F8EH%2F3N3%2BD%2FQdP0BWAbl6%2FhnduXofXMiBi2Wwuqo4lvPaX4WC5t5vzlQtifvNSARCpIaMbLZhUQJDReC2i0qrStIQiFi9riJ26j148RltG9g0ZaL9%2FioN7d3FwNsCRqDlGoLev7qAnhnK93UIknhfn2vvilufCMPXNTRO8lMv6wigzdjGTz210HcTDKVyxX8ZRqs3Oh5weQuBMGASS0%2FpTZP0RBmJHDU6OMZXPdqcjzHZ30TvaRzObIBmmaPc6cIYncm5Hyz9mtKvYY4nqLzc%2FirwIEFlzW1PJ5ybUvn6pAIiwUFlFVsk%2B4%2FdMe5WqiOG%2BNJ2K7TNFKGqmnjlosOmUY2JBaZzi6MEzPDs6wnMGHhnHCQOdhdrZ3NDis4aoq66AiJ0%2FyAoEmGEaDxOqK04LizwMoxxjP8EUsebhktMR3OEMUSJgmMw0NlQT9RnJZ9flvlqeo%2Fm4Y2GmdObIvc3E6M9MPSbnm%2BnCMs6cdcz%2FTomNnIuKH5VqgGgun0Pp2oQcC0PCRdEez0wq3HRCTJ2asEGI2SRBmkzEWRKjeDDD6YOneD6ZiHpjo6uGzi3TT6N6C4ytQxBGXoigXYd%2FtYf6dgfhRk0TrVHGiDbQ9OrCSLnGh%2BpMsgpI%2FNkU6XgKP%2BY07pncyaKJVXme3KIGoXpSMRB9jpergPFN1zLXzNsxjluugcGaDFHITmTixTV5nGcSsLkwVyTHbEZ1DRQmjCCLTaTRG8ae2PWeqRQBR1dUJefzR6LKQgGNI2rMF1bx%2BUlxblIgYlOlzNYzVsWOspzrFosaTIR92GqvcBBsodo6SDVA9IUxEkdBxFgP2cQpAngM8Gn7mZTG7hAsYa7XhJGGY4z7Z2qscxKhr40%2Bjc2hAy7PQ14rFSDI4NfkeSQWS3jWByaeGNk%2B6g36eQGiRg21yFcbhucGjEhTtYq%2BrTdC%2BDUPs9MjLZU1d%2BqUIqYObIP2KgOqGiD6HHHm335RXloMDEWHrWRM0CCfsQVNEht7KKypS8%2FuruwyGzLnVYvMJXLDJI4c68XsOiKAC%2BtiP2fCKLkqJdZcO5xazSYNZDs2vRLPbyjG9DhlswZRj3JdwR1iAWZMQBczOlZ%2FF1WbJlSWaoAot%2BFG060exbO82M%2BS2PniLJkJ7GVabO2amalsqSeD2xdGeiKGtS%2Bu%2BlazK6ASg1kYSmM%2F9QCDyVC8to6oNDGYBwNNrDYFaH5meiv2BRixRp4hYPOQyQYymzDRJE8wENU2m%2FQxIAAbEcasqZ4MdKZIU9z68VSM8WGybNZVGDxWqgGiQuaVioW45XdE9TB%2FlWWmp7TqLjZxYLMGYZ0jJ8Xb%2F%2BRP8fv%2F6B%2Fh7k9%2Biqf3HqAr7nq9fUVnwSajKbZvXkVb3P%2FT40OcyuDHnJVRD7XKLZHBHvuRAEaA1Wsi2%2BwgDcRTZAN22d8X%2FSUOvk6KbG5fwfU7tzEa9LH%2F8AFqLSgYh8dMnxzM73puWF%2Bqs9%2BWLH6x5S%2FcLdRYni9WdWZFYTIyTayu37iOnXfexmfPD3Ht2i340xT3Rme4%2Fdo1JIMhPvj5z5GEBiydOzv4o%2F%2Fuv8X9J4%2Fx1%2F%2FL%2F4rNsAF0ZPD3jjRanQq7TMVd3759Hbf%2B9b%2FE%2BGAPn3z8sbLfQG6qubmJ7s4OOrK1rl9FXTyzjRvXMHm2h7%2F78z%2FH3vPnaPuRtunTxH5qummbElnfaF5n%2BW%2BtglQDROcY1ktxk9wG5DztG8TwYDOqGRulVhcmirDz5uvisAXYPXiCP%2Fyzf4Irm9v4%2BO9%2Bhjj0sbNzBbffvCNu%2Bya8GzfwWrOO%2F%2FF%2F%2Fp%2Fg9idAf4gf%2Fue%2F0mlG26%2FfwVAYalcG%2Fe%2F3uqLyTtGXUe9u9MSeEhuo1cLO7%2F09AWSEI2EhGuWt7pZ4fz467U2cRA2tb1r6I9ZAqgGic8ROUJzPPZOxoXvOisGIjcuFcc5Goj4eP8S%2BqKXbb7%2BLQOyj%2F%2Ff%2F%2Bn%2Bw6UX4b%2F7sX%2BDo3iN8%2BJMPsCuq6cmjZ7j1jTexfeWqqLM%2Bvv%2BX%2FwX%2BSDyzcYrB4QhNAcE%2F%2B%2B%2F%2FBzz49K7OfM23riAUu2nrxm3cfuubeProCY6OzpCGtK%2BacCdTmAiVhzQWY34gxvbpQIDd%2BLq%2Ful%2B7VLWE5UVx3KL%2FYqBFZjOupcFurzKgt27f0gafp8ImG50u9p%2Fv4v%2F%2BN%2F8OH%2F3iF7h99Tpev%2F26dkhjAwfHD7Wp%2BniSyCUDvPn2N8BCIT9sovn6G%2BjdvIFjNjWPE4wZmZbzoms3hO1qZhrrlFHzFK3eBrqyMenLQGNTWIglJs6asE9ZqsNEJdddZZ7FXxSKaOdWFuuzaULE5RSa6Ijaad%2B6ifufPcLw5Azv%2FOG7%2BJPv%2FEO4J0P89Hs%2FwNnJKaJOExvbPXTFjnE32uhEb%2BHd7%2FwD4HQk7FXD9bffxMePHwOjkbj47EEkHh9b7oktNBbQMJ%2BXsgrA93B6cqxd1hizCsXLc09OMP70oQD4wAQwK2bvfBmpCIicpcdzJ%2FUVfX%2FU1GY7u8hTT%2B74%2BBitvV1hlwF%2Bfvd9fPPabdy8vo2ffXQPj5881NDA%2Bx%2B8j%2BB5G9%2B9uYWpeFzTeIwrt2%2Fg3%2F%2F1v0V%2B2Ed9ksHjUunCOg6XdGCnVy3yN7NY8%2BnYtDwW47zRbSEUUDLIWXPrqIkxnu4%2B0ySx57vL7uWaSGXV2QtZtOIFi850WUxRZwTVhPkqGdA7N6%2Fj7Tduo0a33Hewc20bVzd7iMdDdcWvXt3GnVs38PjeR3j%2Fpz%2FRcpE%2F%2FtM%2FRlALMRj1leFYScCAIWNP4%2F19DVCyq4ini1KleHawi72TA%2FSnQ0wgLFRzEDU8BA0fmZ9ri7411GZVYCKbJjBfv7vYWxLmrlI1ZJt1%2BfVzCc%2BkmL7MTvXMwI%2BnmDzdwwfv30X2e0d45%2Fe%2Bhb%2F3ne9ozc%2FGndv403%2F%2BLzAT4%2Fd7f%2FEfdPLill%2FHH%2FzpP8ZG7yqe%2F6fv4aPHT8TeibGzvY1%2F%2BO7vyefU0JSb6LW6qLGxFTulCeA6wkZ%2BzmStQGsyxunBPkI5bvvKJvr3H%2Bns3PMU2nngusAVsUtSARAtSiEontlRYiJtKK22RlMG%2FizJdW5ZfHqmKQmuNXb%2Fx%2B9hdjxC%2F8NPMfjgM9x9fIbaSYKzvT10gwZe37iKp%2B%2Fdxd27v8Thk0c6j%2BzT%2F%2B%2BvcCet43qtgafPD9EWYxqHxwifPcLgF3fxwYNHAtYAV3Z2EM0c%2BIdn6IpHmDzexdDN0BGVxtmwg4ePET95jrOHu2gx6Ztj3n5Yl79yFnVCL9N0S6C7gDUhFx5EGpSD1jYqC%2FnldxzTAd90%2BJGBzF3Eo6nGjTjXnssk1EWl9R8%2FRbx7BuzLQOcBjp7s4m9P%2FqMaxrqY3i8%2FQF%2Fc9aPDI7FjXGEOsYsePMF7%2F%2Be%2Fw%2BbmBnY%2Fuy9s08AP%2F7f%2FHU48wmf7z9AWdehzzY%2F9Ixw8eKwdQmZyp6PTE02BjNt1NJjLO%2BkjPupjwpqj1ARDqXKpahlyMLM9lktiz8PIPJRxwQBEufAgoqzONyu%2Fk1ueSjNd3ZBdzhipzovVGLcEBIzNDFpnGG3swH%2F6HC3tITTWko%2Bw0RRjOkAq7NESY5zNGuLhEIFcY19spcnJDN52SzyuGU5%2F8WPcevcdRDsbaIhx3pRrT9jZwxWX%2FsYVnDLAyMx94CiAY1GlG9euYKvdYas27H78CaJi%2BvbL%2F6bqSSVAdG4pmlNMGNU3HZ38xybl2u1D1%2B5I1eXubmxgU4zq484pzk7FiG5HuC6eFl1yrn%2FGBV%2BYhtjbFQ9O2Oi1N17H0wefYZOdQCYT9AQoP%2FneX2IkNs%2B3%2F%2BS76InxHfePsf2NtzBMY%2B0a0hHjmhMbp0eHqPdaEKToAsYhG0fEwpLi4SWtOkbsPMPCfnv%2Fzot%2FZxXlwntnzsq2OgXWzsunx8SsPbuOUU1NhQk4hYfA0uZRfoDjkYCIJR%2BdNm68%2BTp2bt1EKCzlcdlOMcY7V7aEOa5pl7SePK93Onjt7W%2FIdRLt1xi229jYuYqR2DM33npLjmug0WmJB1eHG7DQPkQU1WVraFPPzc0rukIR7aBZYpbEOm8m7%2BrfWjW58CCiLIGIki8ebAdZLY9lSSqDjbVQgcAZq1zrbCQMw%2FqgSAa55oU6KzVyfLF9fJ01G8oF6M1F8LXsY3o2gM%2BVgaYJXPHqWkGkM2v3Hj%2FTQrVWUFf7phu1sPfZMyT9iRjNdWAkqm%2Bao54JoEapGvN%2B4mKj0UXDCTlvW%2B87cxb3%2FkWoWTrkgiKsEuqM8nnfn9pFqfGdXQGRF%2Fi68TXX3Yifib00FiYQmzs%2BG%2BJg8hTD%2FWP0BSxtUVu95gZmtRYm%2FQH2P7qPT977Jb516w28vn0D6dkIGEzREHW109rAYPcQ78p7OB5g7%2BPPsH94KOpR9o8SpEdDTBFgcnwmzDOFG89QE8REwylmR2cC1Ny47eeospU%2FqDQd6uJLZUC0KvO8a1FSzdpqMg8j0OxWNp2w2%2F0EJ8cnmInXdfzsCKH8%2B%2BTjT7UZOacOTYVldrZ20L9xgL54Vamcs9lqoi0M9cn7H%2BDt1%2B5gcHKEphtgxIXvDk%2BwtXUF%2Bw%2Bf4OToBKfP9%2BGKmnouwNP2wqI2J8enGCcx6vUQdfHExgLU6dM9HD%2FZQ90z5R7lZRvmKrn8h1VMKgOi85jfLnWAYvnLRJc7mBUrlTvaGYRGM9MgNVFWR3snWhd0dHSkqwOx5d3z3WfY39vV7h6enPj0sweoCxjvfvghHty7B5%2BlsCzU9x384u4HeHh6hCT04Lcb2Nja1omHxycnqLP0RJOwDjqNltbEJZOxANzUcW%2F0euifCFt5ubJkXkzENH9BMSM3X%2FbhVUuXni8%2FuThSGRDNpciTLSKOpmKZgT22hWF32JquYR%2Fp%2FHb2oL526xb2RX1tX7uBO998G0%2BfPNVWwrNJjIm4%2Buy%2FKAjEqaimJ589xBt3buOZeGjT8VDnpb0mHhnLbT95vovXvvkGnpwe4p0%2F%2FAP0trbw8cf38PadN7UwrbfRExaaaFqFVQQzMeTdgQBJ7LNQvLvDsz2EYsQ32k3kAjbXM20q2K9IWZUzbP3gBcBY%2B%2BmiVgBUAkSrBnVZ7C524gjZ9YyzPhzTSIGTCNnR43h%2FD8PhRNRYJmqmiXpTWIfxGhnDtjDKDWbv5UJbvTZuv3YTd%2B7cwb1uG3%2F71z9A0AhxTfaxeXnrxg6aAhz3rI7X33wNgbDcp%2Fc%2FwXQ60jVh49lYwBNreQg7jHg9YUExusfilQ19zj9yjafoGl7NV%2Bqrq1qsXwkQfSnRHkEzTScQEGyZF3Hatdg9g6NTZauEnWNjUSliQB8JY9EmYjF%2BNDMdQ8ZnfQS%2Bi30I%2BPonGPVP0RKwtBsRzoSVBkcHiHM2SB9jJLZSKMyXDM4Qc9Fh9obJxbtzMyRyrJhF8LhUqNhZZLREABaGnmnawDbFjrOkmoxmdhZhjBU8LUpnL55UC0Qvy0jy2xWvrMY4jYBi6mgXRvHGZhjsH2kn%2B4kA5nj%2FUGM1rDbMxaWvM9DYbCOW%2FfWojmZu3PsnYowzLNBi3VB%2FiM%2Fu3sVnjx7hhqi5dtY2JSCi%2BjJhom2xhbY4m2Q4xOxshlgQPEynGMYjOGKfhfI57ukQ8emxJmJzzovj5hWRa85ScUzfxksm%2Bm3Jyvesk4hK%2FYrYECHQpKajPYlm4qHNphO4IzG4xUNj93wWrZKFRsJWJwIUenO0o0xTUC6tMFFG2pDPSvYOcCwM5TG3NrqHsdhFM1FH%2B598imarrVWQqD%2FXDmtuFCJo1jBxEkzzREGj99UXQJHltEtEoNFt273WuvqmtK7kqr2MjX5z3%2BxXlkqAyH6vL%2F0CaWgL63BdDXYi085kDChq0tbVlnksHAvGMbq5p01GWn6k89U4il7YMEVlon7Y53Em4KLZ0hDbaSIqcDIYCcjEJpKdU%2FHEuKQCp0S3mkBN3PZIwBrCNLyCqLgwMJMdQ3buN2l7E4ZgYwfP2EWuYwCjHpkFkrPq7y%2F%2B%2FosIHiuVANEXSqHO2CqPRfqZ62HChKzYPyxIy7jIi9hMDXGtGQTUwc6ZJE01ox6ISmLTUDflsbm69dqLWi7dZs5L7KeIXULodY3lmuJhQQxoTs3m6kSzJNcua81uS70uUmPiZMWEylTs6UwNdAaHxqrK3HODjVVoN3yeVAZEczYqZcDV7YXpypoLA7DJuJZhyKBNZDA5n75WF2AFnqq2eqOOsXhrLNr3XLNslVmiPNDWxF4BAJZosDJyIMd2e2b1IFZLxlMTe%2BoIQ43IeGLENwW0Xo2TBHxdpG8kzJWGrnZm06UgYHpb53WPE1Aw1h7WbPoQQi0h2mhs%2FGBLyG1zSi0zcpe0m%2F79v92v%2FUtJZUCUlR%2Bd5c49BFImLHGSDLGbjTF0pnCawhxyXD%2BTgQ5MG7spV%2FURUM17PdJ70sL7TOM7vqg6szRIqoMYipvOxYLDRl2z74GoMS3lYDNRMa6djPGpTHsPZewtwyYQbG%2FMplWcNSvXn2h3kABeM8AoqOO56%2BPsuI%2Fdzx7ju2%2B8hU3Hh1f6C62dVP7rygR1Edf9qAyI7Bdpx59iv0x%2B6SyYHwgzHMxGOJ4NxWZJxe4JtIP9idhCNXk9C8R0Yrshpihkc4rFWWiIc8WhViMo3Oi8KC8xH6ztaoQtTrNYZ6qyQVXku5roBTvj%2B9p7WIDs6rKh9MByRqW54rXc01CM7jNhyn3x%2FA4RINjYQJMX9gPjADA84WQ6%2FdvxwjmAVqPXF1UqASLbxqEMovIvlX0Y3TBEbaOLgIVie%2Fta9jFx2Uom0qL7Y%2FcM%2FZGDCZtQEUB0vYUtQq5GLgBgHX4tzAwwVa2YzrJKTW5u1iTjNO1IAFfzUW%2BG2hGfQHLIPgKcaWZWKZqJSk2ymc7GnYj9NZwONc40SRzU29vodDtoFx3Z1MqHWU6izDirU4teqGS4QFIJEMHOdnUW%2BaTM9iAqtpmM%2Fs1vvIWd69dx8O1v41Bc8fHZKY4FUAPxqJ6dHAoLiM3kzuSvTqFdaDQcwE77wkgCFBbYm94j7nxyknk0PRUhQGX%2FcgIuYkMQn9OGyB6Z5syoLhMWD5BhyEZc3lzAEXhXcU32dZs93L76GnqdTWxtX9E2fwpSuPMY0Xm2j1N6vATRV5VSFZd%2B5UUSKS0AxFcTYZaYa2y02ui95mPz5i0xegNMJxONDfX7AqjjIzGW%2B%2BJRTRAXrnvMnotxLK78UN15zF3uvPTP0aXNa82OmRhZq2tSNRKWY58jtYNYmy1eW63REJZqIpJHFrpxARrOytX1ZAWBUeZqB39%2Bxng6QTY0gcdArsUEcsgk7goPXVTwWKkGiKysBEwW0%2FC5hDl7BEGN5UzsFnYzCztdGTgPXTniqtgcI1Ers9nUrKchBi%2FXOuMqi2xJk6az4oKZAU7Rk5pXz4ryyVTUkS9eFYETcBJj4BcpjMKL4vocXH06MMByWPrBqkaquOlUO%2BmDa6zxM%2BWijDtxXhuKbiY%2BnDl8XgDOBXbPqgWiFZl%2F3QQFVYdjBtAPzP4xnSzGgVigRrvGDbWMVtmBqxK1TH9sLuZiVkTUFufF1ZeLWNnn2k09XYbBdQx7aICBMzeKRWa4aDCBRK8sLZZGYjiBva%2BT2DQP5YRHLiBMY512HBur61qwmamDqou95dr0R0ViRtUAUbkGYjXzXfwLGHdJTK8fFpzR2%2BKAM%2BDIKDZ7LcymZr4Xa4NY1zO3m2G66qdZARztRJuV2Ii3wOWs6gLE4pjUmMFsmp4UHdqmyVSXs7IrYSvA1BNMEIga43ss1%2BU6I4GoNte3y3YaY48NKaqYPqsIiDAHkhnCvHCBzQBoU3zBRpibxXkZNOSyVL4MtJP5unwCYzoha6i1NbCwSW4Wy9QF9XTlocLGtZ%2BQF2uBWNUmexP12IoNZg37XNfhNLm6jMsp5naGXFH7Xaynpq68rkDEcpVcA45ZKd2h98BwAarFQpRqgAhl5bKY6meWqtJUuJgaOWpiC7mM%2FcxIA4kuu8DF62xhfODVTIbWtv91ysSWaQrE4IhX9RQsKI5TF981GXfHyYqFZUp%2BE89hZj4zqwXpatV0AV3TbtisqwZVuQJxvbf5akeLq1RSKgGicmyoLCack%2BsbnLXKmkDGZti%2FnklYrhzEOjC62sxlpUUlpJ5bqufJizxDrvkuyyP5si2r62KZmJE7v0Zxvg1BzM8yKw1ZECpz0ZZnPi81NU9MnzCvRpuJ97n4oHPkgqOrEiCilJfvBMrei2EPTZo6dtGGeRhSXmdFAdgCgia1sFCPKMBFg9mk1g0cdPVoLECU54mqK7fYOx%2FbvDinWHFaV5jOitoUpbFU91Gtcp1XXWTP87Tz%2FlIW%2Fzy0rCT2LyKeKgOihetb%2FLrnm3md5anpGmsdNaY3RHXRjtHGL1mmxWl5kc4wke4FmJDnJVCafq5ubltJkCmMsc15Z1CV5i1urkAivTwa8VkBDhtApHHNNV%2FTbKbMw3sxxxTHvfyPPjfVc5k7%2BwpSXqhpno6c6zdj%2FCZiA6WODCDTUYF4X1zY15d3vFSblnMg5zP33VKiU%2B2dvBhQ2%2FXeqEmjonL1zMhwXlHsZkPnS8yRG%2BOcgURjk2fzNTvU1mKIIDex8DKbLkmWL%2FXOKQMoO%2B%2F4CyIVAVGpDXpeNrAX05LTooYn8YQtfPm1B5kCyVMjOtOurW5aTB7Mi7SJYwbNeFxQlirSZrrPVE06BWYcjQctAL0MIFiXPjVMUwYR16Rlc6wkh1lcTw7meiGe4y5iQrpa47LVtwimLp6XlzG9KFIJEFGWQkWFJWtNXzKLG7JzPswy5U6qg0MA%2BYFnosGMFaWLnomL%2FFsRLtCYpVlW3ILHnTORARi7B7M%2Fo3pmjluyrB0df8Z5dH3i1My7d4pZHWwE6sRyD3ksp8hzpu88c19zMBYxqVVwrNYTXUTPvzIgWpblnj4m3uOrx8MBZ4CRK0lHNUfX89BWwK6ZaWFOcDS04zq2uC1XeyqbT3M2dGdjj%2FbTPC8wLODYdURcLLrhFzk22jx87hgvjoyUFTEljXTLwUz2emZ1WBNusJ%2BlgYVlMdbYcoe4i8RClOqAyH5z1nmaEzt0BJj%2FcjNPu6UdnR6rSvGaXOdugixkvqtmZlrYqKJj1ZLxjmgD5W6mDOOUfHddKyQ3qsZzfU3WKgiLQjcFLlMdCZf8TAyTzWZmLVi23KMRzTVkdUauh8k0FhD5cl0TnFRri3EuoTkmdVf%2FZKf0l1p1dtGkOiCyUtjVL4gFkgxUp9Ex68%2BzKI1gymXwJpOlNAZVCdWP3az9khW2DMtdORWbAURtCUOGSc2S4zpBkmumFZFugiHhong28gyW08bIY9MRjaW1fI9JW98NTO211l%2FDxKXIku6Lzfaclzy%2FaFI9EL1ECBYmMLkSYrvdNjmyYr8CiZHrzADBDizF1cX2FltWrEvGumtuBFhetDceDAb6yM%2BgWPDZHoxsd8zXdc5%2BZRtkrlotj3zPfJ4BjP3szLJcca2qytqAiMLB4iA2uaaHeEEccAKDMpmM5x4T91mA2I3nTlmuUQjPVZVUgMtel8fxXF3NunjPHsf9fOQxBNxoNNLjqKYajYZeIxZ7jWCnOEUjCsoliC6A6JIMAhwCgZtX2CNG5RQ2S2ZYxG5m4Ix%2BtCCw9pA53pzP6xomMyzG9%2Bz6ahYI%2FMxut6ssNObMEwEQ3yfrLIDmzUHDz%2FL99fj61%2BKvsAPNDh4HBwc6iGSNsrCNDDeCh8fagWUrGp2%2F75r3yirPMozp1A89zw68hg30Ol5h7wQYDof62XzNa1lm4vlkH%2B3%2BkZv7JUNxo5Sj21WUtQARhaxCJjg5OVFQcICtLWN%2F%2FUv1QYUhbZYsNwNI1iAQLJPVdC6aUXUEB41zHkNw8D0%2BWkYj%2BLiPoJkV3hllYZN5CqLxeKKfyWO5WUa7VGcXQMq%2Ffmu3zJf0lMdOp6uTFTWq5NjlPtM561h2ImMQMFRLdqC185oAi%2FutCrOfubiWAYu1waxdRs9uOp0UgM7mALfgXQcQOfLruohB0BfE%2FrI%2FTwiG09NTHXQ7SBYkdNfNZmarGq%2FKqBm%2BbweRBrE1hAkca6xr6aoAi4%2F2GII2L1a0tka2vVfr5Vk2pLDNXxzPsLm5iV6vN%2FcGrdq0j1WTtWIiq4I4GFQ99hduWAZL3phhDsx7PPL44XCkwOIg016hWrIeG593Oh19JIjYso%2FXJZCuXLmix%2FO5CVaae7JG%2BKzom8TPYotiHlcGnJUqAoiyNiCiWJVmB65sA8VaNmsARcBY74sDTaDwfXs8z%2BUx3KcFZAVLkeXsZ1jmIWh5nX6%2FPw9g2pAAxcaQrMqkmrNxphdttGrKWoHIejnW6CUY7KYdQArVQvfbGs%2FWgndxygAAAYxJREFUdjLeWQCGcKzKIpBox1ANWjfexoHsOVZt8jnfL%2B%2B3G1Wj9f4ajaZ%2B5jrJ2oDI2iGWDShUMTZuFEWhGtYEhTWCbdCQg0uvjOCiWFYwhnE8T3vw2tZDs4Y4z6WtxBWDaEDbuJBlvbIHx3CAtcfs51SZgaysDYgsgMqDYtWNYZt0nsLgPDSqFQ4qn1tvzgYbbTCSYkMEVu0QaLyO9d5sLIjH12rR3EhfTacs7msZNNagrrKsjXe26iaXvR4OIBOimsAvBq0MOmsH2XwbxRrqli2sp2fjRJZprOdmc2iU5aTugiXNdRaR9FXjuqqstDYgslKO41gxLrapXrSAWPWEytdfBeAqGGwA0rJMOUWyev3V%2BybOLFuV0x5VjlqvhTorD7IVO4iWHcrBvc8bLMNAq672i%2FNMtLKxeG6mX5vzHOd8dnGK4jPOWSuz1DpIZZjoUi6uVDO6dSkXSi5BdCmvLJcgupRXlksQXcoryyWILuWV5RJEl%2FLKcgmiS3lluQTRpbyyXILoUl5Z%2Fn8tRtyudnxinwAAAABJRU5ErkJggg%3D%3D";


    webclient_post_comm(uri, data);
    // rt_free(data);
    
    // rt_free(obj);
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


