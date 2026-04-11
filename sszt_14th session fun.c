#include "headfile.h"

// ... (变量定义省略，保持原样) ...

void led_show(uint8_t led, uint8_t mode)//mode  1亮  0灭
{
    // 【考点：LED控制】国信长天板子的74HC573锁存器控制逻辑
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); // 打开锁存器
    if(mode)
    {
        HAL_GPIO_WritePin(GPIOC, (GPIO_PIN_8 << (led-1)), GPIO_PIN_RESET); // 低电平点亮
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, (GPIO_PIN_8 << (led-1)), GPIO_PIN_SET); // 高电平熄灭
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); // 锁存数据
}

// ... (LCD显示函数省略，你的拼写和格式都符合PDF图3/图4/图5要求) ...

double get_vol(void)
{
    // 【考点：模拟输入】读取电位器 R37 的电压值 (0-3.3V)
    HAL_ADC_Start(&hadc2);
    uint32_t adc_value = HAL_ADC_GetValue(&hadc2);
    return 3.3 * adc_value / 4096;
}

void change(void)
{
    // 【考点：按键锁定功能】如果处于“解锁”状态 (duty_flag == 0)，则允许 R37 控制占空比
    if(duty_flag == 0)
    {
        // 【考点：占空比与电压的线性关系】(对照PDF图2)
        if(get_vol() < 1)
        {
            P_duty = 10; // <1V，占空比 10%
        }
        else if(get_vol() >= 1 && get_vol() <= 3)
        {
            // 1V-3V 线性映射：斜率 k = (85-10)/(3-1) = 37.5
            // 截距 b = 10 - 37.5*1 = -27.5
            P_duty = 37.5 * get_vol() - 27.5; 
        }
        else
        {
            P_duty = 85; // >3V，占空比 85%
        }
    }
    // 动态修改 TIM2_CH2 的比较寄存器，更新实际 PWM 占空比
    TIM2->CCR2 = P_duty * (TIM2->ARR + 1) /100;
    
    // 只有不在参数界面时，才更新用于计算速度的 Rv 和 Kv
    if(lcd_flag != 1) 
    {
        Rv = R;
        Kv = K;
    }
    
    // 【考点：速度转换公式】v = f * 2 * PI * R / (100 * K)
    V = fre * 2 * 3.14 * Rv/(100*Kv);
    
    // 【难点考点：极值统计与2秒保持】(分高频和低频两种模式统计)
    if(M_flag) // 高频模式
    {
        switch(peak_state)
        {
            case WAITING:// 发现潜在的、大于当前记录最高值 (MH) 的新速度
                if(V > MH)
                {
                    candidate_max = V;
                    start_tick = HAL_GetTick(); // 记录当前系统滴答时间
                    peak_state = MONITORING;    // 进入持续监测状态
                }
                break;
            case MONITORING:
                if(V > candidate_max)
                {
                    candidate_max = V;
                    start_tick = HAL_GetTick(); // 如果速度还在升高，重置2秒计时器
                }
                else if(V < candidate_max)
                {
                    // 速度掉下去了，未保持住，舍弃当前候选值，退回等待状态
                    candidate_max = 0;
                    peak_state = WAITING;
                }
                else if(HAL_GetTick() - start_tick >= 2000)
                {
                    // 【考点达成】：稳定保持了 2000ms (2秒)，正式更新历史最大值
                    MH = candidate_max;
                    candidate_max = 0;
                    peak_state = WAITING;
                }
                break;
        }
    }
    else // 低频模式，逻辑同上，只是更新 ML
    {
        /* ... 低频 2 秒保持判定逻辑 ... */
    }
    
    // 【考点：LED指示灯功能】
    // 1) 处于数据界面(lcd_flag == 0)，LD1点亮
    if(lcd_flag == 0) led_show(1,1);
    else led_show(1,0);
        
    // 2) 模式切换期间，LD2 以 0.1 秒间隔闪烁 (led_state 在定时器中断里翻转)
    led_show(2, led_state);
    
    // 3) 占空比处于“锁定”状态，LD3点亮
    if(duty_flag) led_show(3,1);
    else led_show(3,0);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    // 【考点：频率测量】捕获 PA7 的脉冲周期，换算为频率
    if(htim->Instance == TIM3)
    {
        capture = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2) + 1;
        fre = 1000000 / capture; // 假设 TIM3 计数时钟为 1MHz
    }
}

// 假设这是一个 100ms 触发一次的基本定时器中断 (TIM4)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM4)
    {
        // 【难点考点：5秒平滑切换频率】低频(4KHz) -> 高频(8KHz)
        if(fre_flag == 1)
        {
            timer_5s ++;
            led_state = !led_state; // LD2 闪烁 (配合外面的 led_show)
            
            // 注意：题目要求步进 200Hz。
            // 建议修改为：中断改为 250ms 一次，执行 20 次，每次 +200。
            fre4000 += 80; 
            TIM2->ARR = 1000000 / fre4000 -1; // 动态更新 ARR 改变输出频率
            
            if(timer_5s >= 50) // 达到 5 秒
            {
                // 切换完成，清除各种标志位，更新切换次数 N
                candidate_max = 0;
                peak_state = WAITING;
                timer_5s = 0;
                fre_flag = 2; // 标记已处于高频稳定状态
                M_flag = 1;
                N++;
                led_state = 0; // LD2 熄灭
            }
        }
        
        // 高频(8KHz) -> 低频(4KHz) 的平滑降低过程
        if(fre_flag == 3)
        {
           /* ... 同上，只是做减法操作 ... */
        }
        
        // 【考点：B2 按下后 5 秒内不可再次触发切换】
        if(flag_5s)
        {
            key_5s ++;
            if(key_5s >= 50)
            {
                key_5s = 0;
                flag_5s = 0; // 5秒冷却时间结束
            }
        }
        
        // 用于检测 B4 到底按了多久，用于区分长短按
        if(B4_pressd)
        {
            B4_press_duration ++;
        }
    }
}
