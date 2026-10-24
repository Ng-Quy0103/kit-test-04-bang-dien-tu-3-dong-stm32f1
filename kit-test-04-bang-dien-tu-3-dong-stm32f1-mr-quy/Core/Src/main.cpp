/* USER CODE BEGIN Header */
/* USER CODE END Header */
#include "main.h"
#include "Indicator.h" 
#include "sensor.h"
#include "SystemUI.h"
#include "ModbusRTU.h"

void SystemClock_Config(void);

uint32_t millis() {
    return HAL_GetTick();
}

Indicator systemIndicator;
sensor proximitysensor;
SystemUI ui;        // man hinh chinh + cai dat bang remote RF + tu dong luu mat dien & reset qua ngay moi
ModbusRTU modbus;   // RS485 Modbus RTU Slave (ID = 5, 115200-8N1, doc/ghi thong so + FPS)

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  systemIndicator.init();
  proximitysensor.init();

  // Phai goi SAU SystemClock_Config vi can SystemCoreClock de tinh thoi gian
  ui.init(&systemIndicator, millis());
  modbus.init(&ui, millis());

  while (1)
  {
    uint32_t current_time = millis();

    if (proximitysensor.update(current_time) == true) {
        systemIndicator.triggerSingleBeep(current_time);
        ui.onProduct();                     // cong them 'he so' vao san luong + luu ngay chong mat dien
    }

    systemIndicator.update(current_time);
    ui.process(current_time);               // RF, RTC, man hinh LED, kiem tra qua ngay moi, luu Flash
    modbus.process(current_time);           // xu ly truyen nhan RS485 Modbus RTU va tinh FPS
  }
}

// Cau hinh xung nhip he thong 72MHz (HSE 8MHz x 9), co co che an toan khong bao gio treo o Error_Handler
void SystemClock_Config(void)
{
  // 1. Dua SYSCLK ve HSI (8MHz) va tat PLL truoc khi cau hinh lai
  //    (Tranh loi HAL_RCC_OscConfig tra ve HAL_ERROR khi nap code / warm-reset)
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}
  RCC->CFGR &= ~RCC_CFGR_SW;
  while ((RCC->CFGR & RCC_CFGR_SWS) != 0) {}
  RCC->CR &= ~RCC_CR_PLLON;

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // 2. Uu tien khoi dong thach anh ngoai HSE 8MHz -> PLL x9 = 72MHz
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    // Neu HSE gap su co, tu dong chuyen sang HSI/2 x 16 = 64MHz de mach luon chay, khong treo
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
  SystemCoreClockUpdate();
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
