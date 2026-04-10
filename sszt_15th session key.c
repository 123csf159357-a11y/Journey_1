#include "headfile.h"

// 记录当前与上次的电平状态
uint8_t B1_state, B1_last_state;
uint8_t B2_state, B2_last_state;
uint8_t B3_state, B3_last_state;
uint8_t B4_state, B4_last_state;

uint8_t para_select; // 参数界面下的选中项：0为PD, 1为PH, 2为PX

uint8_t B3_pressd;         // B3按下标志位
uint16_t B3_press_duration; // B3按下的持续时间(利用10ms定时器累加)

void key_scan(void)
{
    // 读取当前电平
    B1_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    
    // B1: “加”按键 (检测下降沿)
    if(B1_state == 0 && B1_last_state == 1)
    {
        if(lcd_flag == 1) // 只有在参数界面才有效
        {
            switch(para_select)
            {
                case 0: // 调PD
                    PD += 100;
                    if(PD >= 1000) PD = 1000; // 边界限制
                break;
                case 1: // 调PH
                    PH += 100;
                    if(PH >= 10000) PH = 10000; // 边界限制
                break;
                case 2: // 调PX
                    PX += 100;
                    if(PX >= 1000) PX = 1000; // 边界限制
                break;
            }
        }
    }
    
    // B2: “减”按键 (检测下降沿)
    if(B2_state == 0 && B2_last_state == 1)
    {
        if(lcd_flag == 1)
        {
            switch(para_select)
            {
                case 0:
                    PD -= 100;
                    if(PD <= 100) PD = 100;
                break;
                case 1:
                    PH -= 100;
                    if(PH <= 1000) PH = 1000;
                break;
                case 2:
                    PX -= 100;
                    if(PX <= -1000) PX = -1000; // 题目要求范围是 -1000 到 1000
                break;
            }
        }
    }
    
    // B3: “切换/清零”按键
    // 检测下降沿 (按下瞬间)
    if(B3_state == 0 && B3_last_state == 1)
    {
        if(lcd_flag == 0) // 数据界面：切换频率/周期显示
        {
            data_mode ++;
            data_mode %= 2;
        }
        else if(lcd_flag == 1) // 参数界面：切换选择的参数
        {
            para_select ++;
            para_select %= 3;
        }
        else if(lcd_flag == 2) // 统计界面：启动长按计时
        {
            B3_pressd = 1;           // 标记开始按下
            B3_press_duration = 0;   // 计时器清零
        }
    }
    // 检测上升沿 (松开瞬间)，专门处理长按逻辑
    else if(B3_state == 1 && B3_last_state == 0)
    {
        // 配合定时器，B3_press_duration每10ms加1。>100即超过1000ms(1秒)
        if(B3_press_duration > 100) 
        {
            // 长按超过1秒，清零所有统计数据
            NHA = 0; NHB = 0;
            NDA = 0; NDB = 0;
        }
        B3_pressd = 0; // 松开后清除按下标志位 (这里原代码没有加，建议加上防止误触发)
    }
    
    // B4: “界面切换”按键 (检测下降沿)
    if(B4_state == 0 && B4_last_state == 1)
    {
        lcd_flag ++;
        lcd_flag %= 3;
        
        if(lcd_flag == 1) // 每次进入参数界面，默认选中PD (考题要求)
        {
            para_select = 0;
        }
        if(lcd_flag == 0) // 每次进入数据界面，默认显示频率 (考题要求)
        {
            data_mode = 0;
        }
    }
    
    // 更新历史状态
    B1_last_state = B1_state;
    B2_last_state = B2_state;
    B3_last_state = B3_state;
    B4_last_state = B4_state;
}
