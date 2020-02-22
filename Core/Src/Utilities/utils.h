/*******************************************************************************
 * @copyright This is where we'd put a copyright... IF WE HAD ONE
 * @file utils.h
 * @author paul.czeresko
 * @date 15 Dec 2019
 * @brief Definitions and prototypes for general utilities
 ******************************************************************************/
// @formatter:off

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "main.h"
#include <stdio.h>

/* Defines -------------------------------------------------------------------*/
#define utilsHEARTBEAT_PRIORITY		( tskIDLE_PRIORITY )

/* Structures ----------------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define os_printf(fmt, ...) do {\
	if (os_running) taskENTER_CRITICAL();\
	printf(fmt, ## __VA_ARGS__);\
	if (os_running) taskEXIT_CRITICAL();\
} while (0)

/* Prototypes ----------------------------------------------------------------*/
void utilsInit(void);

/* Exported variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __UTILS_H */
/* EOF */
