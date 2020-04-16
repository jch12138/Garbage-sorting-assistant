/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-03    chenyong      the first version
 */

#include <string.h>

#include <rtthread.h>
#include <webclient.h>

#define POST_RESP_BUFSZ                1024
#define POST_HEADER_BUFSZ              4096

#define POST_LOCAL_URI                 "http://api.tianapi.com/txapi/imglajifenlei/index"

//char post_data[45*1024];
const char *post_data = "key=acda67d9ac820ea200a26f73d0b41adf";

char *webclient_post_comm(const char *uri, const char *post_data)
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

    // if (buffer)
    // {
    //     web_free(buffer);
    // }

    return buffer;
}

/* send HTTP POST request by simplify request interface, it used to received shorter data */
static int webclient_post_smpl(const char *uri, const char *post_data)
{
    char *request = RT_NULL, *header = RT_NULL;
    int index;

    //webclient_header_fields_add(&header, "Content-Length: %d\r\n", strlen(post_data));
    //webclient_header_fields_add(&header, "Content-Type: application/x-www-form-urlencoded;\r\n");
    //webclient_header_fields_add(&header, "Connection: keep-alive\r\n");
    //webclient_header_fields_add(&header, "Accept: */*\r\n");
    //webclient_header_fields_add(&header, "Accept-Encoding: gzip, deflate\r\n");
    //webclient_header_fields_add(&header, "Host: api.tianapi.com\r\n");
    //webclient_header_fields_add(&header, "User-Agent: python-requests/2.22.0\r\n");

    if (webclient_request(uri, (const char *)header, post_data, (unsigned char **)&request) < 0)
    {
        rt_kprintf("webclient send post request failed.");
        web_free(header);
        return -RT_ERROR;
    }

    rt_kprintf("webclient send post request by simplify request interface.\n");
    rt_kprintf("webclient post response data: \n");
    for (index = 0; index < rt_strlen(request); index++)
    {
        rt_kprintf("%c", request[index]);
    }
    rt_kprintf("\n");

    if (header)
    {
        web_free(header);
    }

    if (request)
    {
        web_free(request);
    }

    return 0;
}


int webclient_post_test(int argc, char **argv)
{
    // char *uri = RT_NULL;

    // if (argc == 1)
    // {
    //     uri = web_strdup(POST_LOCAL_URI);
    //     if(uri == RT_NULL)
    //     {
    //         rt_kprintf("no memory for create post request uri buffer.\n");
    //         return -RT_ENOMEM;
    //     }

    //     // int fd = open("/sd/base64.txt",O_RDONLY);
    //     // unsigned char *buffer = RT_NULL;
    //     // buffer = (unsigned char *)rt_malloc(sizeof(unsigned char)*1024*40);
    //     // if(buffer==RT_NULL)
    //     // {
    //     //     rt_kprintf("内存分配失败\r\n");
    //     // }
    //     // int res = read(fd, buffer, 1024*40);
    //     // close(fd);
    //     // rt_kprintf("res=%d\r\n",res);

    //     // strcat(post_data,"key=acda67d9ac820ea200a26f73d0b41adf&img=data:image/jpg;base64,");
    //     // strcat(post_data, buffer);
    //     // rt_kprintf(post_data);
    //     //webclient_post_comm(uri, post_data);
    //     // rt_free(buffer);
    //     // memset(post_data,0,sizeof(post_data));
    // }
    // else if (argc == 3)
    // {
    //     if (rt_strcmp(argv[1], "-s") == 0)
    //     {
    //         uri = web_strdup(POST_LOCAL_URI);
    //         if(uri == RT_NULL)
    //         {
    //             rt_kprintf("no memory for create post request uri buffer.\n");
    //             return -RT_ENOMEM;
    //         }

    //         webclient_post_smpl(uri, post_data);
    //     }
    //     else
    //     {
    //         uri = web_strdup(argv[1]);
    //         if(uri == RT_NULL)
    //         {
    //             rt_kprintf("no memory for create post request uri buffer.\n");
    //             return -RT_ENOMEM;
    //         }
    //         webclient_post_comm(uri, argv[2]);
    //     }
    // }
    // else if(argc == 3 && rt_strcmp(argv[1], "-s") == 0)
    // {
    //     uri = web_strdup(argv[2]);
    //     if(uri == RT_NULL)
    //     {
    //         rt_kprintf("no memory for create post request uri buffer.\n");
    //         return -RT_ENOMEM;
    //     }

    //     webclient_post_smpl(uri, post_data);
    // }
    // else
    // {
    //     rt_kprintf("web_post_test [uri]     - webclient post request test.\n");
    //     rt_kprintf("web_post_test -s [uri]  - webclient simplify post request test.\n");
    //     return -RT_ERROR;
    // }
    
    // if (uri)
    // {
    //     web_free(uri);
    // }

    // return RT_EOK;
}


#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(webclient_post_test, web_post_test, webclient post request test.);
#endif /* FINSH_USING_MSH */
