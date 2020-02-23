/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file keyboard.c
 * @author paul.czeresko
 * @date 11 Dec 2019
 * @brief Application code for key matrix scanning and HID reporting
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "keyboard.h"
#include "../Utilities/utils.h"

#include <stdio.h>
#include <string.h>
#include "usb_hid_keys.h"
#include "../UsbInterface/usb_if.h"

/* Defines -------------------------------------------------------------------*/
#define ROW_MASK	( 0x0003 )

/* Private variables ---------------------------------------------------------*/
static key_matrix_t keeb;

/* Static prototypes ---------------------------------------------------------*/
static void keyboardRefresh(key_matrix_t *kb, uint8_t rowNo, uint8_t colNo,
		GPIO_PinState rowVal);
static void keyboardScanTask(void *pvParameters);
static void keyboardUpdateReport(key_matrix_t *kb, key_struct_t *key);

/* Code ----------------------------------------------------------------------*/
/**
 * @brief Scans each key and records current status and changes
 * @param pvParameters key_matrix_t* of keyboard struct being scanned
 * @retval none
 */
static void keyboardScanTask(void *pvParameters)
{
	key_matrix_t *kb = (key_matrix_t *)pvParameters;
	static GPIO_PinState lastRead;

	for (;;)
	{
		/**
		 * Ye who optimize before having a working prototype shall be subject to
		 * ten thousand years of debugging in the bog of eternal stench.
		 */
		for (int rr = 0; rr < kb->numRows; rr++)
		{
			/**
			 * Switches are active low, so we sink the pin of the target row.
			 */
			HAL_GPIO_WritePin(kb->rowPins[rr]->port, kb->rowPins[rr]->pin,
					GPIO_PIN_RESET);
			for (int cc = 0; cc < kb->numCols; cc++)
			{
				/**
				 * This will report reverse of actual since the switches are
				 * active low. It is the responsibility of the refresh function
				 * to handle this, as the raw reading will be reported to that
				 * function.
				 */
				lastRead = HAL_GPIO_ReadPin(kb->colPins[cc]->port,
						kb->colPins[cc]->pin);
				keyboardRefresh(kb, rr, cc, lastRead);
			}
			/**
			 * Clean up after ourselves by sourcing the current row pin before
			 * moving on.
			 */
			HAL_GPIO_WritePin(kb->rowPins[rr]->port, kb->rowPins[rr]->pin,
					GPIO_PIN_SET);
			/**
			 * This delay is EXTREMELY FUCKING IMPORTANT! The voltage on the
			 * output pins doesn't flip fast enough between row scan loops,
			 * essentially shorting the readings for row 1 and row 2. I spent
			 * three fucking hours debugging this, so out of respect to me and
			 * that segment of life I'll never get back don't you dare fucking
			 * touch this line of code. Thank you have have a great day.
			 */
			vTaskDelay(pdMS_TO_TICKS(5));
			/*
								  /´¯/)
								,/¯../
							   /..../
						 /´¯/'...'/´¯¯`·¸
					  /'/.../..../......./¨¯\
					('(...´...´.... ¯~/'...')
					 \................'...../
					  ''...\.......... _.·´
						\..............(
						  \.............\
			 */
		}
	}
}

/**
 * @brief Updates a given key structure's status as a result of a scan
 * @param kb Pointer to keyboard struct being scanned
 * @param rowNo Index of kb->rowPins array being scanned
 * @param colNo Index of kb->colPins array being scanned
 * @param rowVal State of the pin in question as read by a scan
 * @retval none
 */
static void keyboardRefresh(key_matrix_t *kb, uint8_t rowNo, uint8_t colNo,
		GPIO_PinState rowVal)
{
	static key_struct_t *thisKey;
	static GPIO_PinState lastState;
	static GPIO_PinState keyState;

	thisKey = kb->keys[GET_IDX(colNo, rowNo, kb->numCols)];
	lastState = thisKey->currState;
	keyState = 0x0001 & (rowVal ^ 1);
	if (keyState != lastState && !(thisKey->stateChanged))
	{
		thisKey->stateChanged = 1;
		thisKey->tempState = keyState;
		thisKey->lastTrigger = HAL_GetTick();
	}
	if (HAL_GetTick() - thisKey->lastTrigger > kb->debounce)
	{
		if (thisKey->stateChanged)
		{
			thisKey->stateChanged = 0;
			if (keyState == thisKey->tempState)
			{
				thisKey->currState = keyState;
				keyboardUpdateReport(kb, thisKey);
				os_printf("Triggered: %s, State: %d\r\n",
						thisKey->name, thisKey->currState);
			}
		}
	}
}

/**
 * @brief Updates the requesting key's status in the HID report structure
 * @param key Pointer to key struct reporting its status
 * @retval none
 */
#define FNLAYER		(uint8_t)kb->isFnLayer
static void keyboardUpdateReport(key_matrix_t *kb, key_struct_t *key)
{
	key_struct_t *thisKey = key;
	GPIO_PinState keyState = thisKey->currState;
	uint16_t keyIndex = thisKey->reportIndex;
	/**
	 * On a rising edge if we haven't already reported our status
	 */
	if (keyState && !keyIndex)
	{
		if (thisKey->val[0] == 0xFF)
		{
			kb->isFnLayer = 1;
		}
		else if (thisKey->isMod)
		{
			usbifUpdateMod(thisKey->val[FNLAYER]);
		}
		else
		{
			uint16_t k = usbifRequestKey();
			/**
			 * @c usbifUpdateKey returns 6 if there is no available index in the
			 * HID report. This results in the current keypress being ignored.
			 *
			 * TODO: report rollover overflow
			 */
			if (k < 6)
			{
				thisKey->reportIndex = usbifUpdateKey(k, thisKey->val[FNLAYER]) + 1;
			}
		}
	}
	/**
	 * On a falling edge
	 */
	else if (!keyState)
	{
		if (thisKey->val[0] == 0xFF)
		{
			kb->isFnLayer = 0;
		}
		else if (thisKey->isMod)
		{
			usbifClearMod(thisKey->val[FNLAYER]);
		}
		else
		{
			thisKey->reportIndex = usbifClearKey(keyIndex - 1);
		}
	}
}
#undef FNLAYER

/**
 * @brief Use this to construct keyboard initial conditions and key mapping
 * @param none
 * @retval none
 */
void keyboardInit()
{
	/* Initialize keys -------------------------------------------------------*/
	/* Row 00 ----------------------------------------------------------------*/
	static key_struct_t kr0c0 = {
			.name = "Q",
			.val = { KEY_Q, KEY_1 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};

	/* TODO: make this const? */
	static key_struct_t *theKeys[180] = {
			&kr0c0, &kr0c1, &kr0c2, &kr0c3, &kr0c4, &kr0c5, &kr0c6, &kr0c7, &kr0c8, &kr0c9, &kr0c10,
			&kr1c0, &kr1c1, &kr1c2, &kr1c3, &kr1c4, &kr1c5, &kr1c6, &kr1c7, &kr1c8, &kr1c9, &kr1c10,
			&kr2c0, &kr2c1, &kr2c2, &kr2c3, &kr2c4, &kr2c5, &kr2c6, &kr2c7, &kr2c8, &kr2c9, &kr2c10,
			&kr3c0, &kr3c1, &kr3c2, &kr3c3, &kr3c4, &kr3c5, &kr3c6, &kr3c7, &kr3c8, &kr3c9, &kr3c10,
	};

	/* Initialize rows -------------------------------------------------------*/
	static gpio_struct_t row0 = {
			.port = ROW_0_GPIO_Port,
			.pin = ROW_0_Pin
	};
	static gpio_struct_t row1 = {
			.port = ROW_1_GPIO_Port,
			.pin = ROW_1_Pin
	};
	static gpio_struct_t row2 = {
			.port = ROW_2_GPIO_Port,
			.pin = ROW_2_Pin
	};
	static gpio_struct_t row3 = {
			.port = ROW_3_GPIO_Port,
			.pin = ROW_3_Pin
	};
	static gpio_struct_t row4 = {
			.port = ROW_4_GPIO_Port,
			.pin = ROW_4_Pin
	};
	static gpio_struct_t row5 = {
			.port = ROW_5_GPIO_Port,
			.pin = ROW_5_Pin
	};
	static gpio_struct_t row6 = {
			.port = ROW_6_GPIO_Port,
			.pin = ROW_6_Pin
	};
	static gpio_struct_t row7 = {
			.port = ROW_7_GPIO_Port,
			.pin = ROW_7_Pin
	};

	/* Initialize columns ----------------------------------------------------*/
	static gpio_struct_t col0 = {
			.port = COL_0_GPIO_Port,
			.pin = COL_0_Pin
	};
	static gpio_struct_t col1 = {
			.port = COL_1_GPIO_Port,
			.pin = COL_1_Pin
	};
	static gpio_struct_t col2 = {
			.port = COL_2_GPIO_Port,
			.pin = COL_2_Pin
	};
	static gpio_struct_t col3 = {
			.port = COL_3_GPIO_Port,
			.pin = COL_3_Pin
	};
	static gpio_struct_t col4 = {
			.port = COL_4_GPIO_Port,
			.pin = COL_4_Pin
	};
	static gpio_struct_t col5 = {
			.port = COL_5_GPIO_Port,
			.pin = COL_5_Pin
	};
	static gpio_struct_t col6 = {
			.port = COL_6_GPIO_Port,
			.pin = COL_6_Pin
	};
	static gpio_struct_t col7 = {
			.port = COL_7_GPIO_Port,
			.pin = COL_7_Pin
	};
	static gpio_struct_t col8 = {
			.port = COL_8_GPIO_Port,
			.pin = COL_8_Pin
	};
	static gpio_struct_t col9 = {
			.port = COL_9_GPIO_Port,
			.pin = COL_9_Pin
	};
	static gpio_struct_t col10 = {
			.port = COL_10_GPIO_Port,
			.pin = COL_10_Pin
	};
	static gpio_struct_t col11 = {
			.port = COL_11_GPIO_Port,
			.pin = COL_11_Pin
	};
	static gpio_struct_t col12 = {
			.port = COL_12_GPIO_Port,
			.pin = COL_12_Pin
	};
	static gpio_struct_t col13 = {
			.port = COL_13_GPIO_Port,
			.pin = COL_13_Pin
	};
	static gpio_struct_t col14 = {
			.port = COL_14_GPIO_Port,
			.pin = COL_14_Pin
	};
	static gpio_struct_t col15 = {
			.port = COL_15_GPIO_Port,
			.pin = COL_15_Pin
	};
	static gpio_struct_t col16 = {
			.port = COL_16_GPIO_Port,
			.pin = COL_16_Pin
	};
	static gpio_struct_t col17 = {
			.port = COL_17_GPIO_Port,
			.pin = COL_17_Pin
	};
	static gpio_struct_t col18 = {
			.port = COL_18_GPIO_Port,
			.pin = COL_18_Pin
	};
	static gpio_struct_t col19 = {
			.port = COL_19_GPIO_Port,
			.pin = COL_19_Pin
	};

	static gpio_struct_t *gpioRows[8] = {
			&row0, &row1, &row2, &row3, &row4, &row5, &row6, &row7
	};
	static gpio_struct_t *gpioCols[20] = {
			&col0, &col1, &col2, &col3, &col4, &col5, &col6, &col7, &col8, &col9,
			&col10, &col11, &col12, &col13, &col14, &col15, &col16, &col17, &col18, &col19,
	};

	/* Initialize keyboard ---------------------------------------------------*/
	keeb = (key_matrix_t) {
		.numRows = 8,
				.numCols = 20,
				.rowPins = gpioRows,
				.colPins = gpioCols,
				.keys = theKeys,
				.isFnLayer = 0,
				.debounce = 30
	};
	/* FreeRTOS Stuff --------------------------------------------------------*/
	xTaskCreate(keyboardScanTask, "kbscan", keyboardSCAN_STACK_SIZE,
			(void *)&keeb, keyboardSCAN_PRIORITY, NULL);

	/* Misc. cleanup ---------------------------------------------------------*/
}
/* EOF */
