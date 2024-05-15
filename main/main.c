#include <stdio.h>
#include "blink_example_main.h"
#include "temp_sensor_main.h"
// #include "blufi_example_main.h"
#include "wifi_config.h"
#include "mqtt_client.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

temperature_humidity_t th;

// 事件处理函数
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // printf("Event dispatched from event loop base=%s, event_id=%d \n", base, event_id);
    //  获取MQTT客户端结构体指针
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    // 通过事件ID来分别处理对应的事件
    switch (event->event_id)
    {
    // 建立连接成功
    case MQTT_EVENT_CONNECTED:
        printf("MQTT_client cnnnect to EMQ ok. \n");
        // 发布主题，主题消息为“I am ESP32.”,自动计算有效载荷，qos=1
        esp_mqtt_client_publish(client, "ESP32_Publish", "I am ESP32.", 0, 1, 0);
        // 订阅主题，qos=0
        esp_mqtt_client_subscribe(client, "lvmaoya/#", 0);
        break;
    // 客户端断开连接
    case MQTT_EVENT_DISCONNECTED:
        printf("MQTT_client have disconnected. \n");
        break;
    // 主题订阅成功
    case MQTT_EVENT_SUBSCRIBED:
        printf("mqtt subscribe ok. msg_id = %d \n", event->msg_id);
        break;
    // 取消订阅
    case MQTT_EVENT_UNSUBSCRIBED:
        printf("mqtt unsubscribe ok. msg_id = %d \n", event->msg_id);
        break;
    //  主题发布成功
    case MQTT_EVENT_PUBLISHED:
        printf("mqtt published ok. msg_id = %d \n", event->msg_id);
        break;
    // 已收到订阅的主题消息
    case MQTT_EVENT_DATA:
        printf("mqtt received topic: %.*s \n", event->topic_len, event->topic);
        printf("topic data: %.*s\r\n", event->data_len, event->data);
        break;
    // 客户端遇到错误
    case MQTT_EVENT_ERROR:
        printf("MQTT_EVENT_ERROR \n");
        break;
    default:
        printf("Other event id:%d \n", event->event_id);
        break;
    }
}
esp_mqtt_client_handle_t client;
static void mqtt_app_start(void)
{
    // 1、定义一个MQTT客户端配置结构体，输入MQTT的url
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtts://b9dfdd51.ala.cn-hangzhou.emqxsl.cn",
        .broker.address.port = 8883,
        .credentials.username = "lvmaoya",
        .credentials.authentication.password = "king.sun1",
    };

    // 2、通过esp_mqtt_client_init获取一个MQTT客户端结构体指针，参数是MQTT客户端配置结构体
    client = esp_mqtt_client_init(&mqtt_cfg);

    // 3、注册mqtt_client回调函数用于检测执行过程中的一些状态，跟wifi连接回调函数一样
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    // 4、开启MQTT功能
    esp_mqtt_client_start(client);
}

void app_main(void)
{

    configure_led();
    blink_led(16, 16, 16);
    // blufi();
    wifi_connect_init();
    mqtt_app_start();

    while (1)
    {
        th = getTempSensor();
        printf("Temperature: %.2f°C, Humidity: %.2f%%\n", th.temperature, th.humidity);
        // 将温湿度数据格式化为字符串
        char payload[64];
        sprintf(payload, "Temp=%.2f,Humi=%.2f%%", th.temperature, th.humidity);
        esp_mqtt_client_publish(client, "room/temp_humi", payload, 0, 1, 0);
        vTaskDelay(1000); // 延时300毫秒
    }
}