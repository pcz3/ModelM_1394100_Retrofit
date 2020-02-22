/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file usb_if.h
 * @author paul.czeresko
 * @date 15 Dec 2019
 * @brief Definitions and prototypes for USB HID functionality
 ******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBIF_H
#define __USBIF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "FreeRTOSConfig.h"
#include "main.h"

/* Defines -------------------------------------------------------------------*/
#define usbifREPORT_STACK_SIZE		( 512 )
#define usbifREPORT_PRIORITY		( tskIDLE_PRIORITY + 2 )

/* Structures ----------------------------------------------------------------*/
typedef struct _USB_KEYBOARD_REPORT_S_
{
	uint8_t id;
	uint8_t modifiers;
	uint8_t keys[6];
} usb_hid_kb_rpt_t;

/* Prototypes ----------------------------------------------------------------*/
uint16_t usbifRequestKey(void);
uint16_t usbifUpdateKey(uint16_t idx, uint8_t val);
uint16_t usbifClearKey(uint16_t idx);
uint16_t usbifUpdateMod(uint8_t val);
uint16_t usbifClearMod(uint8_t val);
void usbifInit(void);

/* Exported variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __USBIF_H */
/* EOF */
