#include "headfile.h"

char text[30];          // 用于LCD显示的字符串缓存
double R37_vol;         [span_0](start_span)// 模拟信号A电压值 (0-3.3V)，控制占空比[span_0](end_span)
double R38_vol;         [span_1](start_span)// 模拟信号B电压值 (0-3.3V)，控制频率[span_1](end_span)
uint32_t capture;       // TIM2输入捕获值
uint32_t PA15_fre;      [span_2](start_span)// PA15捕获的脉冲信号频率[span_2](end_span)

[span_3](start_span)// 系统工作参数与默认值[span_3](end_span)
uint8_t DS_value = 1;     // 占空比步长 (DS)，默认1%
uint8_t DR_value = 80;    // 占空比范围 (DR)，默认80%
uint32_t FS_value = 100;  // 频率步长 (FS)，默认100Hz
uint32_t FR_value = 2000; // 频率范围 (FR)，默认2000Hz

// 参数界面中用于临时调节的新参数，确认有效后才覆盖原参数
uint8_t DS_new_value;
uint8_t DR_new_value;
uint32_t FS_new_value;
uint32_t FR_new_value;

uint8_t PA7_duty;       // 当前PA7输出的PWM占空比
uint32_t PA7_fre;       // 当前PA7输出的PWM频率

uint8_t lcd_flag;       [span_4](start_span)// LCD界面标志位：0-监控，1-统计，2-参数[span_4](end_span)

[span_5](start_span)// 运行时长变量[span_5](end_span)
uint16_t hour;
uint16_t min;
uint16_t sec;

// 记录发生频率异常时的运行时长
uint16_t error_hour;
uint16_t error_min;
uint16_t error_sec;

[span_6](start_span)// 记录发生频率异常时的系统状态[span_6](end_span)
uint32_t CF_error_value; // 异常时的PA7输出频率
uint8_t CD_error_value;  // 异常时的PA7输出占空比
uint32_t DF_error_value; // 异常时的PA15捕获频率
uint32_t XF_value;       // 异常时的频率差值

uint32_t initial_timer;  // 启动延时计数器
uint8_t initial_flag;    // 初始化完成标志，防止上电瞬间数据乱跳误判异常
int chazhi;              // 频率差值
uint8_t error_flag;      [span_7](start_span)// 频率异常标志位 (差值 > 1000Hz)[span_7](end_span)

uint8_t ST_flag;         [span_8](start_span)// 输出状态锁定标志：0-解锁(UNLOCK)，1-锁定(LOCK)[span_8](end_span)

// 获取ADC转换的电压值 (0-3.3V)
double get_vol(ADC_HandleTypeDef *hadc)
{
    HAL_ADC_Start(hadc);
    uint32_t adc_value = HAL_ADC_GetValue(hadc);
    return 3.3 * adc_value / 4096;
}

// 核心处理函数（放入主循环）
void change(void)
{
    [span_9](start_span)// 1. 采集两路模拟信号[span_9](end_span)
    R37_vol = get_vol(&hadc2);
    R38_vol = get_vol(&hadc1);
    
    // 2. 计算系统运行时长
    hour = HAL_GetTick() / 3600000;
    min =  HAL_GetTick() / 60000 - hour * 60;
    sec = HAL_GetTick() / 1000 - hour * 3600 - min * 60;
    
    [span_10](start_span)// 3. 如果未处于锁定状态，则根据电压动态调节PWM[span_10](end_span)
    if(ST_flag == 0)
    {
        [span_11](start_span)// 占空比阶梯调节逻辑 [cite: 40-57]
        // duty_index 计算当前电压落在哪一个台阶上
        uint32_t duty_index = R37_vol * (DR_value - 10) / (3.3 * DS_value);
        TIM3->CCR2 = 10 + duty_index * DS_value; // 基础值10% + 偏移量
        
        [cite_start]// 频率阶梯调节逻辑 [cite: 58-73]
        uint32_t fre_index = R38_vol * (FR_value - 1000) / (3.3 * FS_value);
        uint32_t freq = 1000 + fre_index * FS_value; // 基础值1000Hz + 偏移量
        TIM3->PSC = 800000 / freq - 1; // 假设TIM3时钟源为80MHz，先经过某些分频，此处公式需根据实际定时器配置核对
        
        // 更新当前输出状态记录
        PA7_duty = TIM3->CCR2;
        PA7_fre = 800000 / (TIM3->PSC + 1);
    }
    
    [cite_start]// 4. 频率异常判定 (系统初始化完成后才开始判定)[span_11](end_span)
    if(initial_flag)
    {
        // 计算绝对差值
        chazhi = PA15_fre - PA7_fre;
        if(chazhi < 0)
        {
            chazhi = PA7_fre - PA15_fre;
        }
        
        // 当偏差 > 1000Hz 时触发异常判定
        if(chazhi > 1000)
        {
            [span_12](start_span)// 仅在“第一次”进入异常状态时记录数据，持续异常不更新[span_12](end_span)
            if(error_flag == 0)
            {
                error_flag = 1;
                CF_error_value = PA7_fre;
                CD_error_value = PA7_duty;
                DF_error_value = PA15_fre;
                XF_value = chazhi;
                error_hour = hour;
                error_min = min;
                error_sec = sec;
            }
            
        }
        else
        {
            error_flag = 0; // 恢复正常
        }
    }
    
    [span_13](start_span)// 5. LED 指示灯逻辑 [cite: 192-197]
    led_show(1, lcd_flag == 0); // LD1: 监控界面点亮
    led_show(2, ST_flag);       // LD2: 锁定状态点亮
    led_show(3, error_flag);    // LD3: 异常状态点亮
}

// LED 控制底层驱动
void led_show(uint8_t led, uint8_t mode) 
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); // 打开锁存器
    if(mode)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET); // 点亮
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);   // 熄灭
    }
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); // 关闭锁存器
}

[cite_start]// 监控界面显示 [cite: 82-106]
void pwm_show(void)
{
    sprintf(text, "        PWM   ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    sprintf(text, "   CF=%dHz    ", PA7_fre);
    LCD_DisplayStringLine(Line3, (uint8_t *)text);
    sprintf(text, "   CD=%d%%    ", PA7_duty);
    LCD_DisplayStringLine(Line4, (uint8_t *)text);
    sprintf(text, "   DF=%dHz    ", PA15_fre);
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    
    [cite_start]// 锁定状态显示[span_13](end_span)
    if(ST_flag == 0)
    {
        sprintf(text, "   ST=UNLOCK    ");
    }
    else if(ST_flag == 1)
    {
        sprintf(text, "   ST=LOCK    ");
    }
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
    
    [span_14](start_span)// 运行时长显示格式: xxHyyMzzS[span_14](end_span)
    sprintf(text, "   %02dH%02dM%02dS   ", hour, min ,sec);
    LCD_DisplayStringLine(Line7, (uint8_t *)text);
}

[span_15](start_span)// 统计界面显示 (异常记录) [cite: 107-125]
void recd_show(void)
{
    sprintf(text, "        RECD   ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    sprintf(text, "   CF=%dHz     ", CF_error_value);
    LCD_DisplayStringLine(Line3, (uint8_t *)text);
    sprintf(text, "   CD=%d%%     ", CD_error_value);
    LCD_DisplayStringLine(Line4, (uint8_t *)text);
    sprintf(text, "   DF=%dHz    ", DF_error_value);
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    sprintf(text, "   XF=%dHz     ", XF_value);
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
    sprintf(text, "   %02dH%02dM%02dS   ", error_hour, error_min , error_sec);
    LCD_DisplayStringLine(Line7, (uint8_t *)text);
}

[cite_start]// 参数界面显示 [cite: 126-148]
void para_show(void)
{
    sprintf(text, "        PARA   ");
    LCD_DisplayStringLine(Line1, (uint8_t *)text);
    sprintf(text, "   DS=%d%%     ", DS_new_value);
    LCD_DisplayStringLine(Line3, (uint8_t *)text);
    sprintf(text, "   DR=%d%%    ", DR_new_value);
    LCD_DisplayStringLine(Line4, (uint8_t *)text);
    sprintf(text, "   FS=%dHz    ", FS_new_value);
    LCD_DisplayStringLine(Line5, (uint8_t *)text);
    sprintf(text, "   FR=%dHz     ", FR_new_value);
    LCD_DisplayStringLine(Line6, (uint8_t *)text);
    
    // 清除下方的残留显示
    sprintf(text, "                 ");
    LCD_DisplayStringLine(Line7, (uint8_t *)text);
}

// 界面切换调度
void lcd_show(void)
{
    switch(lcd_flag)
    {
        case 0:
            pwm_show();
            break;
        case 1:
            recd_show();
            break;
        case 2:
            para_show();
            break;
    }
}

// TIM4 周期中断回调：用于系统初始化延时
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM4)
    {
        initial_timer ++;
        if(initial_timer >= 100) // 达到设定时间后
        {
            initial_flag = 1; // 标记初始化完成
            HAL_TIM_Base_Stop_IT(&htim4); // 关闭此定时器，节省资源
        }
    }
}

[cite_start]// TIM2 输入捕获中断回调：计算 PA15 频率[span_15](end_span)
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        capture = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1) + 1;
        PA15_fre = 1000000 / capture; // 根据捕获周期计算频率 (假设定时器频率为1MHz)
    }
}
