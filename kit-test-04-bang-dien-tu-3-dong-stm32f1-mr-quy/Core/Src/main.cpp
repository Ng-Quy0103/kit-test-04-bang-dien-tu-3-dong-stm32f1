/* USER CODE BEGIN Header */
/* USER CODE END Header */
#include "main.h"
#include "Indicator.h" 
#include "sensor.h"
#include "SystemUI.h"

void SystemClock_Config(void);

uint32_t millis() {
    return HAL_GetTick();
}

Indicator systemIndicator;
sensor proximitysensor;
SystemUI ui;        // man hinh chinh + cai dat bang remote RF (ben trong co LED 7 doan, RTC, RF)

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  systemIndicator.init();
  proximitysensor.init();

  // Phai goi SAU SystemClock_Config vi can SystemCoreClock de tinh thoi gian
  ui.init(&systemIndicator, millis());

  while (1)
  {
    uint32_t current_time = millis();

    if (proximitysensor.update(current_time) == true) {
        systemIndicator.triggerSingleBeep(current_time);
        ui.onProduct();                     // cong them 'he so' vao san luong
    }

    systemIndicator.update(current_time);
    ui.process(current_time);               // RF, RTC, man hinh LED (khong chan, khong delay)
  }
}
// SystemClock_Config va Error_Handler giu nguyen
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
