#include "headfile.h"

// ... (变量定义省略) ...

void key_scan(void)
{
    B1_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    // ... 获取按键电平

    // 【考点：B1 界面切换】数据 -> 参数 -> 记录 循环
    if(B1_state == 0 && B1_last_state == 1) // 下降沿检测 (按下)
    {
        lcd_flag ++;
        lcd_flag %= 3;
        // 每次进入参数界面 (lcd_flag==1)，默认可调整参数为 R
        if(lcd_flag == 1)
        {
            RK_flag = 0;
        }
    }
    
    // 【考点：B2 功能复用】
    if(B2_state == 0 && B2_last_state == 1)
    {
        if(lcd_flag == 0) // 在数据界面：触发高低频切换
        {
            if(flag_5s == 0) // 检查是否处于 5 秒冷却期
            {
                if(fre_flag == 0)
                {
                    fre_flag = 1; // 触发 向高频变化状态
                    flag_5s = 1;  // 开启 5 秒冷却
                }
                if(fre_flag == 2)
                {
                    fre_flag = 3; // 触发 向低频变化状态
                    flag_5s = 1;
                }
            }
        }
        else if(lcd_flag == 1) // 在参数界面：切换选择 R 或 K
        {
            RK_flag ++;
            RK_flag %= 2;
        }
    }
    
    // 【考点：B3 参数增加】
    if(B3_state == 0 && B3_last_state == 1)
    {
        if(lcd_flag == 1) // 仅在参数界面有效
        {
            if(RK_flag == 0)
            {
                R++;
                if(R>10) R=1; // 边界值循环 (题目要求1-10)
            }
            else
            {
                K++;
                if(K>10) K=1;
            }
        }
    }
    
    // 【核心考点：B4 长短按复用】
    // 1. 按下瞬间的处理
    if(B4_state == 0 && B4_last_state == 1)
    {
        if(lcd_flag == 1) // 在参数界面：作为减按键 (短按)
        {
            if(RK_flag == 0)
            {
                R--;
                if(R<1) R=10; // 边界值处理
            }
            else /* ... K-- 逻辑 ... */
        }
        else if(lcd_flag == 0) // 在数据界面：开始记录按压时长
        {
            B4_pressd = 1;          // 标记按键正在被按下
            B4_press_duration = 0;  // 清零时长计数器(由TIM4中断累加)
        }
    }
    // 2. 松开瞬间的处理 (判断长短按)
    else if(B4_state == 1 && B4_last_state == 0) 
    {
        if(B4_pressd)
        {
            // 假设按压时长阈值 20 次触发，对应 20*100ms = 2秒 (如果TIM4是100ms的话)
            if(B4_press_duration >= 20) 
            {
                // 大于 2 秒，判定为“长按”，执行锁定占空比动作
                B4_pressd = 0;
                B4_press_duration = 0;
                duty_flag = 1; // 设置锁定标志位
            }
            else
            {
                // 小于 2 秒，判定为“短按”，执行解锁占空比动作
                duty_flag = 0; // 解除锁定
            }
        }
    }
    
    // 更新历史状态，用于下一次的跳变沿检测
    B1_last_state = B1_state;
    // ...
}
