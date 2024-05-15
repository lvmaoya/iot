#include <stdio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <stdio.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"
#include <math.h>

#define DHT11_PIN 15 // 定义DHT11的引脚

// 宏定义是一种C/C++预处理器指令,它可以用来定义一个标识符(宏名)与一串文本(宏值)之间的对应关系。这个过程称为"宏替换"。
#define uchar unsigned char // 一种无符号的8位整数类型,取值范围为0到255。
#define uint8 unsigned char
#define uint16 unsigned short // 一种无符号的16位整数类型,取值范围为0到65535。

// 温湿度定义
uchar ucharFLAG, uchartemp;
float Humi, Temp;
uchar ucharT_data_H, ucharT_data_L, ucharRH_data_H, ucharRH_data_L, ucharcheckdata;
uchar ucharT_data_H_temp, ucharT_data_L_temp, ucharRH_data_H_temp, ucharRH_data_L_temp, ucharcheckdata_temp;
uchar ucharcomdata;

// 静态函数,意味着它的作用域仅限于当前源文件内部,不能被其他源文件访问
// 设置端口为输入
static void InputInitial(void)
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);        // 函数将指定的 GPIO 引脚选择为 GPIO 模式
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT); // 将 DHT11_PIN 引脚的方向设置为输入模式。GPIO 引脚可以配置为输入或输出模式,输入模式表示该引脚用于读取电平信号。
}

static void OutputHigh(void) // 输出1
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 1);
}

static void OutputLow(void) // 输出0
{
    esp_rom_gpio_pad_select_gpio(DHT11_PIN);
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
}

static uint8 getData() // 读取状态
{
    return gpio_get_level(DHT11_PIN);
}

// 读取一个字节数据
static void COM(void) // 温湿写入
{
    uchar i;
    for (i = 0; i < 8; i++)
    {
        ucharFLAG = 2; // ucharFLAG 变量在这个函数中用于检测 IO 口状态变化的超时情况,避免程序无限等待而阻塞。0~255
        // 等待IO口变低，变低后，通过延时去判断是0还是1 这是DHT11 传感器发送数据的起始信号。
        while ((getData() == 0) && ucharFLAG++)
            esp_rom_delay_us(10);
        esp_rom_delay_us(35); // 延时35us,这是 DHT11 传感器发送数据的时间窗口。
        uchartemp = 0;

        // 根据 IO 口在 35 微秒后的电平状态,判断该位是 0 还是 1。如果 IO 口在 35 微秒后还是高电平,则认为该位是 1,否则为 0。
        if (getData() == 1)
            uchartemp = 1;
        ucharFLAG = 2;

        // 等待IO口变高，变高后，表示可以读取下一位
        while ((getData() == 1) && ucharFLAG++)
            esp_rom_delay_us(10);
        if (ucharFLAG == 1)
            break;
        ucharcomdata <<= 1;        // 是为了给下一位数据腾出空间,为后续的位操作做准备。
        ucharcomdata |= uchartemp; // 将这个位设置为想要的值，同时也不影响其他位的设置
    }
}

void Delay_ms(uint16 ms)
{
    int i = 0;
    for (i = 0; i < ms; i++)
    {
        esp_rom_delay_us(1000);
    }
}
double convertIntToDecimal(int num)
{
    if (num < 0 || num > 255)
    {
        // 处理错误情况，例如返回0或设置一个错误代码
        return 0.0;
    }

    if (num == 0)
    {
        return 0.0;
    }

    // 将整数除以10的适当次幂，以得到形如0.xxx的小数
    // 对于1-9，除以100；对于10-99，除以10；对于100-255，除以1
    double divisor = 1.0;
    if (num >= 10 && num < 100)
    {
        divisor = 10.0;
    }
    else if (num >= 100)
    {
        divisor = 100.0;
    }
    else
    {
        divisor = 10.0; // 对于1-9，我们仍然想除以100来得到0.xx的形式
    }

    return (double)num / divisor;
}
void DHT11(void) // 温湿传感启动
{
    OutputLow();    // 输出低电平，DHT11 传感器的启动信号。
    Delay_ms(19);   //>18MS
    OutputHigh();   // 输出高电平
    InputInitial(); // 设置端口为输入
    esp_rom_delay_us(30);
    if (!getData()) // 表示传感器拉低总线
    {
        ucharFLAG = 2;
        // 等待总线被传感器拉高
        while ((!getData()) && ucharFLAG++)
            esp_rom_delay_us(10);
        // 等待总线被传感器拉低
        while ((getData()) && ucharFLAG++)
            esp_rom_delay_us(10);
        COM(); // 读取第1字节，
        ucharRH_data_H_temp = ucharcomdata;
        COM(); // 读取第2字节，
        ucharRH_data_L_temp = ucharcomdata;
        COM(); // 读取第3字节，
        ucharT_data_H_temp = ucharcomdata;
        COM(); // 读取第4字节，
        ucharT_data_L_temp = ucharcomdata;
        COM(); // 读取第5字节，
        ucharcheckdata_temp = ucharcomdata;
        OutputHigh();
        // 判断校验和是否一致
        uchartemp = (ucharT_data_H_temp + ucharT_data_L_temp + ucharRH_data_H_temp + ucharRH_data_L_temp);
        if (uchartemp == ucharcheckdata_temp)
        {
            // 校验和一致，
            ucharRH_data_H = ucharRH_data_H_temp; // 表示温度的整数部分
            ucharRH_data_L = ucharRH_data_L_temp; // 表示温度的小数部分
            ucharT_data_H = ucharT_data_H_temp;
            ucharT_data_L = ucharT_data_L_temp;
            ucharcheckdata = ucharcheckdata_temp;
            // 保存温度和湿度
            Humi = ucharRH_data_H + (convertIntToDecimal(ucharRH_data_L));
            // Humi = ((uint16)Humi << 8 | ucharRH_data_L) / 10;
            // 将 Humi 变量左移 8 位,相当于将其乘以 256。这是为了为湿度的小数部分腾出空间
            // 运算符 | 将湿度的小数部分(低 8 位)合并到 Humi 中,形成一个 16 位的完整湿度值。
            // 最后将这个 16 位的湿度值除以 10,得到最终的湿度值。这是因为 DHT11 传感器的湿度数据是以 0.1% 为单位的,所以需要除以 10 来获得正确的湿度百分比。
            Temp = ucharT_data_H + (convertIntToDecimal(ucharT_data_L));
            // Temp = ((uint16)Temp << 8 | ucharT_data_L) / 10;
        }
        else
        {
            Humi = 100;
            Temp = 100;
        }
    }
    else // 没用成功读取，返回0
    {
        Humi = 0,
        Temp = 0;
    }

    OutputHigh(); // 输出
}

typedef struct {
    float temperature;
    float humidity;
} temperature_humidity_t;

temperature_humidity_t getTempSensor()
{
    temperature_humidity_t th;
    DHT11(); // 读取温湿度
    th.temperature = Temp;
    th.humidity = Humi;
    return th;
}
