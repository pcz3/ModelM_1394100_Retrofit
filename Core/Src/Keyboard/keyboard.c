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
	static key_struct_t kr0c1 = {
			.name = "W",
			.val = { KEY_W, KEY_2 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c2 = {
			.name = "E",
			.val = { KEY_E, KEY_3 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c3 = {
			.name = "R",
			.val = { KEY_R, KEY_4 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c4 = {
			.name = "T",
			.val = { KEY_T, KEY_5 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c5 = {
			.name = "Y",
			.val = { KEY_Y, KEY_6 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c6 = {
			.name = "U",
			.val = { KEY_U, KEY_7 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c7 = {
			.name = "I",
			.val = { KEY_I, KEY_8 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c8 = {
			.name = "O",
			.val = { KEY_O, KEY_9 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c9 = {
			.name = "P",
			.val = { KEY_P, KEY_0 },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr0c10 = {
			.name = "BSPACE",
			.val = { KEY_BACKSPACE, KEY_DELETE },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	/* Row 01 ----------------------------------------------------------------*/
	static key_struct_t kr1c0 = {
			.name = "A",
			.val = { KEY_A, KEY_A },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c1 = {
			.name = "S",
			.val = { KEY_S, KEY_S },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c2 = {
			.name = "D",
			.val = { KEY_D, KEY_D },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c3 = {
			.name = "F",
			.val = { KEY_F, KEY_F },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c4 = {
			.name = "G",
			.val = { KEY_G, KEY_G },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c5 = {
			.name = "H",
			.val = { KEY_H, KEY_H },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c6 = {
			.name = "J",
			.val = { KEY_J, KEY_J },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c7 = {
			.name = "K",
			.val = { KEY_K, KEY_K },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c8 = {
			.name = "L",
			.val = { KEY_L, KEY_L },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c9 = {
			.name = "NA",
			.val = { KEY_NONE, KEY_NONE },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr1c10 = {
			.name = "ENTER",
			.val = { KEY_ENTER, KEY_ENTER },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	/* Row 02 ----------------------------------------------------------------*/
	static key_struct_t kr2c0 = {
			.name = "Z",
			.val = { KEY_Z, KEY_Z },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c1 = {
			.name = "X",
			.val = { KEY_X, KEY_X },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c2 = {
			.name = "C",
			.val = { KEY_C, KEY_C },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c3 = {
			.name = "V",
			.val = { KEY_V, KEY_V },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c4 = {
			.name = "B",
			.val = { KEY_B, KEY_B },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c5 = {
			.name = "N",
			.val = { KEY_N, KEY_N },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c6 = {
			.name = "M",
			.val = { KEY_M, KEY_M },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c7 = {
			.name = "NA",
			.val = { KEY_NONE, KEY_NONE },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c8 = {
			.name = "COMMA",
			.val = { KEY_COMMA, KEY_DOT },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c9 = {
			.name = "RSHIFT",
			.val = { KEY_MOD_RSHIFT, KEY_MOD_RSHIFT },
			.isMod = 1,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr2c10 = {
				.name = "TAB",
				.val = { KEY_TAB, KEY_ESC },
				.isMod = 0,
				.currState = 0,
				.tempState = 0,
				.stateChanged = 0,
				.lastTrigger = 0,
				.reportIndex = 0
	};
	/* Row 03 ----------------------------------------------------------------*/
	static key_struct_t kr3c0 = {
			.name = "NA",
			.val = { KEY_NONE, KEY_NONE },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c1 = {
			.name = "LCTRL",
			.val = { KEY_MOD_LCTRL, KEY_MOD_LCTRL },
			.isMod = 1,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c2 = {
			.name = "LMETA",
			.val = { KEY_MOD_LMETA, KEY_MOD_LMETA },
			.isMod = 1,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c3 = {
			.name = "LALT",
			.val = { KEY_MOD_LALT, KEY_MOD_LALT },
			.isMod = 1,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c4 = {
			.name = "FN",
			.val = { 0xFF, 0xFF },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c5 = {
			.name = "SPACE",
			.val = { KEY_SPACE, KEY_SPACE },
			.isMod = 0,
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c6 = {
			.name = "NA",
			.val = { KEY_NONE, KEY_NONE },
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c7 = {
			.name = "LEFT",
			.val = { KEY_LEFT, KEY_HOME },
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c8 = {
			.name = "DOWN",
			.val = { KEY_DOWN, KEY_DOWN },
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c9 = {
			.name = "UP",
			.val = { KEY_UP, KEY_UP },
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	static key_struct_t kr3c10 = {
			.name = "RIGHT",
			.val = { KEY_RIGHT, KEY_END },
			.currState = 0,
			.tempState = 0,
			.stateChanged = 0,
			.lastTrigger = 0,
			.reportIndex = 0
	};
	/* TODO: make this const? */
	static key_struct_t *theKeys[44] = {
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
	/* TODO: make this const? */
	static gpio_struct_t *gpioRows[4] = { &row0, &row1, &row2, &row3 };
	static gpio_struct_t *gpioCols[11] = {
			&col0, &col1, &col2, &col3, &col4, &col5, &col6, &col7, &col8, &col9, &col10
	};

	/* Initialize keyboard ---------------------------------------------------*/
	keeb = (key_matrix_t) {
		.numRows = 4,
				.numCols = 11,
				.rowPort = GPIOB,
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
