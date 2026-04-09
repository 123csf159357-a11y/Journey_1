#include "headfile.h"

// 按键当前状态与历史状态
uint8_t B1_state;
uint8_t B2_state;
uint8_t B3_state;
uint8_t B4_state;

uint8_t B1_last_state;
uint8_t B2_last_state = 1; // 默认高电平未按下
uint8_t B3_last_state;
uint8_t B4_last_state;

// 参数界面中当前选中的参数项: 0-DS, 1-DR, 2-FS, 3-FR
uint8_t para_select_flag;

void key_scan(void)
{
    static uint32_t B2_last2s; // 记录B2按下的时间戳，用于长按判断
    
    [span_16](start_span)// --- 按键消抖处理 ---[span_16](end_span)
    static uint32_t last_tick;
    uint32_t current_tick = HAL_GetTick();
    if(current_tick - last_tick < 20) return; // 间隔小于20ms直接退出
    last_tick = current_tick;
    
    // 读取引脚电平 (假设低电平有效)
    B1_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    
    [span_17](start_span)// ================== B1: 界面切换逻辑[span_17](end_span) ==================
    if(B1_last_state == 1 && B1_state == 0) // B1下降沿(按下)
    {
        lcd_flag ++;
        lcd_flag %= 3; [span_18](start_span)// 0-监控, 1-统计, 2-参数 循环切换[span_18](end_span)
        
        [span_19](start_span)// 从参数界面(2)切走时，进行参数合法性校验[span_19](end_span)
        if(lcd_flag == 0)
        {
            // 校验：步长必须小于对应范围，且至少能调节一个步长 (基础值+步长 <= 范围)
            if(10 + DS_new_value <= DR_new_value && 1000 + FS_new_value <= FR_new_value)
            {
                // 合法，将新参数覆盖到实际生效的参数中
                DS_value = DS_new_value;
                DR_value = DR_new_value;
                FS_value = FS_new_value;
                FR_value = FR_new_value;
            }
            // 若非法，则不覆盖，自动还原丢弃本次修改
        }
        
        [span_20](start_span)// 进入参数界面时，初始化选择项与临时变量[span_20](end_span)
        if(lcd_flag == 2)
        {
            para_select_flag = 0; // 默认选择DS
            // 将当前生效的参数拷贝到临时变量供调整
            DS_new_value = DS_value;
            DR_new_value = DR_value;
            FS_new_value = FS_value;
            // 注意：你的原代码中这一行在大括号外面，是个隐患，我帮你移到括号里面了！
            FR_new_value = FR_value; 
        }   
    }
    
    [span_21](start_span)// ================== B2: 功能切换逻辑[span_21](end_span) ==================
    if(B2_last_state == 1 && B2_state == 0) // B2下降沿(按下)
    {
        if(lcd_flag == 0) // 监控界面下
        {
            B2_last2s = HAL_GetTick(); // 记录按下时刻，为了后续判断长按
        }
        [span_22](start_span)if(lcd_flag == 2) // 参数界面下：切换选中参数[span_22](end_span)
        {
            para_select_flag ++;
            para_select_flag %= 4; [span_23](start_span)// 0-DS, 1-DR, 2-FS, 3-FR 循环切换[span_23](end_span)
        }
    }
    else if(B2_last_state == 0 && B2_state == 1) // B2上升沿(松开)
    {
        if(lcd_flag == 0) // 监控界面下：执行短按或长按逻辑
        {
            [span_24](start_span)// 如果按下时长超过2秒 -> 长按[span_24](end_span)
            if(HAL_GetTick() - B2_last2s > 2000) 
            {
                uwTick = 0; // 重置系统时间 (注意：如果在外部文件中需声明 extern __IO uint32_t uwTick;)
            }
            [span_25](start_span)[span_26](start_span)// 否则为短按 -> 切换锁定/解锁状态[span_25](end_span)[span_26](end_span)
            else
            {
                ST_flag = !ST_flag;
            }
        }
    }
    
    [span_27](start_span)// ================== B3: 加按键逻辑[span_27](end_span) ==================
    if(B3_last_state == 1 && B3_state == 0) // B3按下
    {
        [span_28](start_span)// 仅在参数界面有效[span_28](end_span)
        if(lcd_flag == 2)
        {
            [span_29](start_span)switch(para_select_flag) // 根据当前选中的参数进行调整[span_29](end_span)
            {
                case 0: // DS: +1%
                    if(DS_new_value < 100) DS_new_value += 1;
                    else DS_new_value = 100; // 限幅防越界
                    break;
                case 1: // DR: +10%
                    if(DR_new_value < 100) DR_new_value += 10;
                    else DR_new_value = 100;
                    break;
                case 2: // FS: +100Hz
                    FS_new_value += 100;
                    break;
                case 3: // FR: +1000Hz
                    FR_new_value += 1000;
                    break;
            }
        }
    }
    
    [span_30](start_span)// ================== B4: 减按键逻辑[span_30](end_span) ==================
    if(B4_last_state == 1 && B4_state == 0) // B4按下
    {
        [span_31](start_span)// 仅在参数界面有效[span_31](end_span)
        if(lcd_flag == 2)
        {
            [span_32](start_span)switch(para_select_flag) // 根据当前选中的参数进行调整[span_32](end_span)
            {
                case 0: // DS: -1%
                    if(DS_new_value > 0) DS_new_value -= 1;
                    else DS_new_value = 0;
                    break;
                case 1: // DR: -10%
                    if(DR_new_value > 0) DR_new_value -= 10;
                    else DR_new_value = 0;
                    break;
                case 2: // FS: -100Hz
                    if(FS_new_value > 0) FS_new_value -= 100;
                    else FS_new_value = 0;
                    break;
                case 3: // FR: -1000Hz
                    if(FR_new_value > 0) FR_new_value -= 1000;
                    else FR_new_value = 0;
                    break;
            }
        }
    }
    
    // 更新按键历史状态
    B1_last_state = B1_state;
    B2_last_state = B2_state;
    B3_last_state = B3_state;
    B4_last_state = B4_state;
}
