/* USER CODE BEGIN 2 */
    // 初始化前先关掉LED锁存器，防止上电乱闪
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
    
    // LCD初始化及背景配置
    LCD_Init();
    LCD_Clear(Black);
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);
    
    // 启动输入捕获中断 (测量频率)
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
    // 启动基本定时器中断 (负责10ms心跳、时间窗口、按键长按)
    HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 这里的轮询非常高效，因为每个函数内部都是无阻塞的 (没有HAL_Delay)
      key_scan();  // 扫描按键状态
      lcd_show();  // 刷新显示屏内容
      change();    // 检测阈值和控制LED
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
