/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file usb_if.c
 * @author paul.czeresko
 * @date 15 Dec 2019
 * @brief Main software interface for USB HID functionality
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_if.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usbd_hid.h"

/* Defines -------------------------------------------------------------------*/

/* Global variables ---------------------------------------------------------*/
extern USBD_HandleTypeDef hUsbDeviceFS;

/* Private variables ---------------------------------------------------------*/
static usb_hid_kb_rpt_t hidKeyboard;

/* Static prototypes ---------------------------------------------------------*/
static void usbifReportTask(void *pvParameters);

/* Code ----------------------------------------------------------------------*/
/**
 * @brief Periodically sends HID keyboard report to the USB host
 * @param pvParameters UNUSED, for FreeRTOS consistency only
 * @retval none
 */
static void usbifReportTask(void *pvParameters)
{
	UNUSED(pvParameters);
	for (;;)
	{
		USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t *)&hidKeyboard, sizeof(usb_hid_kb_rpt_t));
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

/**
 * @brief Looks through HID keys report and returns the least available index
 * @note If array is full, this will return an out-of-bounds index that needs to
 *       be handled by the receiving function.
 * @param none
 * @retval index The next available slot in the keys report. If no position is
 *         available (i.e. if 6 keys are pressed) the return value is 6.
 */
uint16_t usbifRequestKey()
{
	uint16_t index = 0;
	while (index < 6 && hidKeyboard.keys[index] != 0)
	{
		index++;
	}
	return index;
}

/**
 * @brief Assigns an HID scan code to an address of the HID keys report
 * @param idx The index the requestor is trying to fill
 * @param val The HID keyboard scan code of the key
 * @retval idx if successful, otherwise 0
 */
uint16_t usbifUpdateKey(uint16_t idx, uint8_t val)
{
	if (idx < 6 && hidKeyboard.keys[idx] == 0)
	{
		hidKeyboard.keys[idx] = val;
		return idx;
	}
	return 0;
}

/**
 * @brief Clears the requested index of the HID keyboard report
 * @param idx The index to be cleared
 * @retval 0 as confirmation
 */
uint16_t usbifClearKey(uint16_t idx)
{
	hidKeyboard.keys[idx] = 0;
	return 0;
}

/**
 * @brief Sets requested bits in the HID modifier report
 * @param val The HID keyboard modifier code of the key
 * @retval idx if successful, otherwise 0
 */
uint16_t usbifUpdateMod(uint8_t val)
{
	hidKeyboard.modifiers |= val;
	return 0;
}

/**
 * @brief Clears the requested bits of the HID modifier report
 * @param val The bits to be zeroed
 * @retval 0 as confirmation
 */
uint16_t usbifClearMod(uint8_t val)
{
	hidKeyboard.modifiers &= ~val;
	return 0;
}

/**
 * @brief Initializes necessary components and creates tasks
 * @param none
 * @retval none
 */
void usbifInit()
{
	/* Initialize report structure -------------------------------------------*/
	hidKeyboard = (usb_hid_kb_rpt_t) {
		.id = 1,
				.modifiers = 0,
				.keys = { 0 }
	};

	/* Initialize RTOS features ----------------------------------------------*/
	xTaskCreate(usbifReportTask, "usbrpt", usbifREPORT_STACK_SIZE, NULL,
			usbifREPORT_PRIORITY, NULL);
}

/* EOF */
