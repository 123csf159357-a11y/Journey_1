#include "headfile.h"

uint32_t freA_meature; // A通道原始测量频率
uint32_t freB_meature; // B通道原始测量频率
uint32_t captureA;     // TIM2 捕获计数值
uint32_t captureB;     // TIM3 捕获计数值

int freA; // 最终显示的A通道频率 (原值 + PX)
int freB; // 最终显示的B通道频率 (原值 + PX)

// --- 输入捕获回调函数 ---
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    // A通道 (PA15 -> TIM2_CH1)
    if(htim->Instance == TIM2)
    {
        // 获取计数值，+1是为了补偿计数偏差。假设定时器频率为1MHz。
        captureA = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1) + 1;
        freA_meature = 1000000 / captureA; // f = 1M / N
    }
    // B通道 (PB4 -> TIM3_CH1)
    if(htim->Instance == TIM3)
    {
        captureB = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1) + 1;
        freB_meature = 1000000 / captureB;
    }
}

// 计时器变量，依托于 10ms 的基本定时器中断
uint16_t timer_100ms;
uint16_t timerA_3s;
uint16_t timerB_3s;

// 3秒窗口内的最大值与最小值记录
int freA_max, freA_min;
int freB_max, freB_min;

// --- 基本定时器回调函数 (TIM6, 配置为10ms触发一次) ---
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM6)
    {
        // 1. 数据更新频次控制 (题目要求：10次/秒，即每100ms更新一次)
        timer_100ms ++;
        if(timer_100ms >= 10) // 10ms * 10 = 100ms
        {
            timer_100ms = 0;
            freA = freA_meature + PX; // 最终频率 = 测量频率 + 校准值
            freB = freB_meature + PX;
        }
    }
    
    // 2. 处理B3按键的长按计时
    if(B3_pressd)
    {
        B3_press_duration ++; // 每10ms加1，100次即1秒
    }
    
    // 3. A通道：3秒滑动窗口求极值
    if(freA > 0) // 防错保护
    {
        if(timerA_3s == 0) // 窗口刚开始，初始化最大最小值
        {
            freA_max = freA;
            freA_min = freA;
        }
        else // 窗口进行中，不断比较更新最大最小值
        {
            if(freA > freA_max) freA_max = freA;
            if(freA < freA_min) freA_min = freA;
        }
        timerA_3s ++;
    }
    
    // 4. B通道：3秒滑动窗口求极值 (逻辑同上)
    if(freB >0)
    {
        if(timerB_3s == 0)
        {
            freB_max = freB;
            freB_min = freB;
        }
        else
        {
            if(freB > freB_max) freB_max = freB;
            if(freB < freB_min) freB_min = freB;
        }
        timerB_3s ++;
    }   
    
    // 5. 3秒窗口结算
    if(timerA_3s >= 300) // 10ms * 300 = 3000ms = 3s
    {
        timerA_3s = 0; // 重置计时器，开启下一个窗口
        // 如果窗口内的最大差值大于突变参数PD，则突变次数加1
        if((freA_max - freA_min) > PD) NDA++;
        
        // （个人建议：结算后其实不用再手动赋freA，因为下一轮timerA_3s==0时会自动初始化）
        freA_max = freA; 
        freA_min = freA;
    }
    
    if(timerB_3s >= 300) // 3s
    {
        timerB_3s = 0;
        if((freB_max - freB_min) > PD) NDB++;
        freB_max = freB;
        freB_min = freB;
    }
}
