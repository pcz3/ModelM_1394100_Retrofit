/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file utils.c
 * @author paul.czeresko
 * @date 15 Dec 2019
 * @brief General globally useful utilities
 ******************************************************************************/
// @formatter:off

/* Includes ------------------------------------------------------------------*/
#include "utils.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#ifdef __USART_H
#include "usart.h"
#endif

#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

/* Defines -------------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Static prototypes ---------------------------------------------------------*/
static void utilsHeartbeatTask(void *pvParameters);

/* Code ----------------------------------------------------------------------*/
static void utilsHeartbeatTask(void *pvParameters)
{
	for (;;)
	{
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

#ifdef __USART_H
/**
 * @see https://electronics.stackexchange.com/questions/206113/how-do-i-use-the-printf-function-on-stm32
 */
int _write(int file, char *data, int len)
{
	int result = 0;
	HAL_StatusTypeDef status;

	if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
	{
		errno = EBADF;
		result = -1;
	}
	else
	{
		// arbitrary timeout 1000
		status = HAL_UART_Transmit(&huart3, (uint8_t*)data, len, 1000);

		// return # of bytes written - as best we can tell
		if (status == HAL_OK)
		{
			result = len;
		}
	}
	return result;
}
#endif

void utilsInit(void)
{
	xTaskCreate(utilsHeartbeatTask, "hbeat", configMINIMAL_STACK_SIZE, NULL, utilsHEARTBEAT_PRIORITY, NULL);
}
/* EOF */
