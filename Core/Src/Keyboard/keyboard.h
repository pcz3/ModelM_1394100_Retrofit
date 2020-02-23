/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file keyboard.h
 * @author paul.czeresko
 * @date 11 Dec 2019
 * @brief Definitions and prototypes for key matrix scanning
 ******************************************************************************/
// @formatter:off

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"

/* Defines -------------------------------------------------------------------*/
#define keyboardSCAN_STACK_SIZE		( 1024 )
#define keyboardSCAN_PRIORITY		( tskIDLE_PRIORITY + 3 )

/* Structures ----------------------------------------------------------------*/
typedef struct _KEYBOARD_KEY_S_
{
	uint8_t name[8];		/* Name up to 7 characters long (+ null term) */
	uint8_t val[2];			/* HID keyboard scan values of this key */
	_Bool isMod;			/* 0 is not modifier key, 1 is modifier key */
	GPIO_PinState currState;/* 0 is not pressed, 1 is pressed */
	GPIO_PinState tempState;/* 0 is not pressed, 1 is pressed (for debouncing*/
	_Bool stateChanged;		/* 0 is no change since last update, 1 is has changed */
	uint32_t lastTrigger;	/* Time stamp at which the key was last pressed */
	uint16_t reportIndex;	/* Current index in HID report */
} key_struct_t;

typedef struct _KEYBOARD_MATRIX_S_
{
	uint8_t numRows;		/* Number of rows to be scanned */
	uint8_t numCols;		/* Number of columns to be scanned */
	gpio_struct_t **rowPins;/* Pointer to array of row pins */
	gpio_struct_t **colPins;/* Pointer to array of column pins */
	key_struct_t **keys;	/* Pointer to base address of key array */
	_Bool isFnLayer;		/* 0 for normal mode, 1 for alternate functions */
	uint16_t debounce;		/* debounce time in ms */
} key_matrix_t;

/* Prototypes ----------------------------------------------------------------*/
void keyboardInit(void);
void keyboardLoop(void);
void keyboardUpdateKey(key_struct_t *self, uint8_t newVal);

/* Exported variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __KEYBOARD_H */
/* EOF */
