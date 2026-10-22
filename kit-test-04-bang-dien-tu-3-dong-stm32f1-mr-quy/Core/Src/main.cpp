/* USER CODE BEGIN Header */
/* USER CODE END Header */
#include "main.h"
#include "Indicator.h" 
#include "sensor.h"
#include "led7seg.h"

volatile uint32_t totalProducts = 0;

void SystemClock_Config(void);

uint32_t millis() {
    return HAL_GetTick();
}

Indicator systemIndicator;
sensor proximitysensor;
Led7Seg myLed;

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  systemIndicator.init();
  proximitysensor.init();
  
  // Khoi tao LED 
  myLed.init(true); 
  
  // CHI XUAT DU LIEU DUNG 1 LAN O DAY ROI DE IC TU NHO, KHONG UPDATE LIEN TUC NUA
  myLed.displayRows(22, 10, 2026); 
  
  while (1)
  {
    uint32_t current_time = millis();

    // Phan quet cam bien giu nguyen (khong block, hoat dong tron tru)
    if (proximitysensor.update(current_time) == true) {
        systemIndicator.triggerSingleBeep(current_time);
        totalProducts = proximitysensor.getCount();
        
        // Sau nay neu muon dem san pham, ban chi can goi myLed.displayRows(totalProducts, 10, 2026) nam trong khoi IF nay
    }

    systemIndicator.update(current_time);
  }
}

// Cac ham he thong giu nguyen mac dinh 
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif