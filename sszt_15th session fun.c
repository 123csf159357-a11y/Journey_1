#include "headfile.h"

// --- 全局变量定义 ---
uint16_t PD = 1000; // 突变参数默认值 1000Hz
uint16_t PH = 5000; // 超限参数默认值 5000Hz
int PX;             // 校准值参数默认值 0 (可以为负数，所以用int)

uint8_t NDA, NDB;   // A/B通道频率突变次数统计
uint8_t NHA, NHB;   // A/B通道频率超限次数统计

// --- LED 控制函数 ---
// 蓝桥杯板子上的LED是通过74HC573锁存器控制的
void led_show(uint8_t led, uint8_t mode)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); // 打开锁存器LE
    if(mode) // mode非0表示点亮
    {
        // PC8对应LED1，往左移位 (led - 1) 刚好对应对应的引脚，写RESET点亮（低电平点亮）
        HAL_GPIO_WritePin(GPIOC, (GPIO_PIN_8 << (led - 1)), GPIO_PIN_RESET);
    }
    else // mode为0表示熄灭
    {
        HAL_GPIO_WritePin(GPIOC, (GPIO_PIN_8 << (led - 1)), GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); // 锁存数据
}

char text[20];       // LCD显示的字符串缓冲区
uint8_t data_mode;   // 数据界面模式：0为频率显示，1为周期显示

// --- 数据界面显示 (DATA) ---
void data_show(void)
{
    sprintf(text, "        DATA        ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    
    // 模式0：频率显示模式
    if(data_mode == 0)
    {
        // A通道处理
        if(freA < 0) // 若加了校准值后频率为负，按要求显示NULL
        {
            sprintf(text, "     A=NULL        ");
        }
        else if(freA >=0 && freA < 1000) // 小于1000Hz，取整，单位Hz
        {
            sprintf(text, "     A=%dHz        ", freA);
        }
        else // 大于等于1000Hz，自动切换为KHz并保留两位小数
        {
            float freA_temp = freA / 1000.0;
            sprintf(text, "     A=%.2fKHz        ", freA_temp);
        }
        LCD_DisplayStringLine(Line3, (uint8_t *)text);
        
        // B通道处理 (同上)
        if(freB < 0)
        {
            sprintf(text, "     B=NULL        ");
        }
        else if(freB >=0 && freB < 1000)
        {
            sprintf(text, "     B=%dHz        ", freB);
        }
        else
        {
            float freB_temp = freB / 1000.0;
            sprintf(text, "     B=%.2fKHz        ", freB_temp);
        }
        LCD_DisplayStringLine(Line4, (uint8_t *)text);
    }
    // 模式1：周期显示模式
    else if(data_mode == 1)
    {
        // 根据频率计算周期 (T = 1/f)，单位转为微秒(uS)
        float freA_T = 1.0 /freA * 1000000;
        float freB_T = 1.0 /freB * 1000000;
        
        // A通道周期处理
        if(freA_T < 0) // 防错处理：f为负时，周期也显示NULL
        {
            sprintf(text, "     A=NULL        ");
        }
        else if(freA_T >=0 && freA_T < 1000) // 小于1000uS，取整，单位uS
        {
            sprintf(text, "     A=%duS        ", (uint32_t)freA_T);
        }
        else // 大于等于1000uS，自动切换为mS并保留两位小数
        {
            freA_T = freA_T / 1000;
            sprintf(text, "     A=%.2fmS        ", freA_T);
        }
        LCD_DisplayStringLine(Line3, (uint8_t *)text);
        
        // B通道周期处理 (同上)
        if(freB_T < 0)
        {
            sprintf(text, "     B=NULL        ");
        }
        else if(freB_T >=0 && freB_T < 1000)
        {
            sprintf(text, "     B=%duS        ", (uint32_t)freB_T);
        }
        else
        {
            freB_T = freB_T / 1000;
            sprintf(text, "     B=%.2fmS        ", freB_T);
        }
        LCD_DisplayStringLine(Line4, (uint8_t *)text);
    }
    // 覆盖掉上一界面的残留字符
    sprintf(text, "                    ");
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    sprintf(text, "                    ");
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
}

// --- 参数界面显示 (PARA) ---
void para_show(void)
{
    sprintf(text, "        PARA        ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    
    sprintf(text, "     PD=%dHz        ", PD);
    LCD_DisplayStringLine(Line3, (uint8_t *)text);
    sprintf(text, "     PH=%dHz        ", PH);
    LCD_DisplayStringLine(Line4, (uint8_t *)text);
    sprintf(text, "     PX=%dHz        ", PX);
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    // 覆盖残留字符
    sprintf(text, "                    ");
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
}

// --- 统计界面显示 (RECD) ---
void recd_show(void)
{
    sprintf(text, "        RECD        ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    
    sprintf(text, "     NDA=%d        ", NDA);
    LCD_DisplayStringLine(Line3, (uint8_t *)text);
    sprintf(text, "     NDB=%d        ", NDB);
    LCD_DisplayStringLine(Line4, (uint8_t *)text);
    sprintf(text, "     NHA=%d        ", NHA);
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    sprintf(text, "     NHB=%d        ", NHB);
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
}

uint8_t lcd_flag; // 界面状态机：0数据界面 1参数界面 2统计界面

// --- LCD 界面调度 ---
void lcd_show(void)
{
    switch(lcd_flag)
    {
        case 0: data_show(); break;
        case 1: para_show(); break;
        case 2: recd_show(); break;
        default: break;
    }
}

// 用于记录上一时刻是否超限，实现“跨越阈值才计数”的防抖逻辑
uint8_t NHA_flag, NHB_flag;

// --- 状态与报警处理任务 ---
void change(void)
{
    // A通道超限次数计算：只有从不超过PH变成超过PH时，才加1
    if(NHA_flag == 0)
    {
        if(freA > PH)
        {
            NHA ++;
            NHA_flag = 1; // 标记已超限
        }
    }
    else
    {
        if(freA <= PH) NHA_flag = 0; // 恢复正常区间，解除标记
    }
    
    // B通道超限次数计算 (逻辑同上)
    if(NHB_flag == 0)
    {
        if(freB > PH)
        {
            NHB ++;
            NHB_flag = 1;
        }
    }
    else
    {
        if(freB <= PH) NHB_flag = 0;
    }
    
    // LED1: 处于数据界面亮，否则灭
    if(lcd_flag == 0) led_show(1, 1);
    else led_show(1, 0);
    
    // LED2: A通道频率超限亮
    if(freA > PH) led_show(2, 1);
    else led_show(2, 0);
    
    // LED3: B通道频率超限亮
    if(freB > PH) led_show(3, 1);
    else led_show(3, 0);
        
    // LED8: 任意通道突变次数>=3亮
    if((NDA >= 3) || (NDB >= 3)) led_show(8, 1);
    else led_show(8, 0);
}
