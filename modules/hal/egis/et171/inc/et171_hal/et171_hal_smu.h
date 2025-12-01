/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

/**
 * \file
 * \brief ET171 HAL SMU
 */
#ifndef __ET171_HAL_SMU_H__
#define __ET171_HAL_SMU_H__

#include "et171_hal_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SMU 0x04 */

/**
 *  \enum ROOT_CLK_SEL
 *  \brief root clock select
 */
typedef enum
{
    ROOT_CLK_32K = SMU_CLK_SRC_32K,   ///< 32k root clock source
    ROOT_CLK_250M = SMU_CLK_SRC_250M  ///< 250M root clock source
} ROOT_CLK_SEL;

/**
 *  \enum ROOT_CLK_DIV
 *  \brief root clock div
 */
typedef enum
{
    ROOT_CLK_DIV_1 = SMU_CLK_SRC_DIV_1,  ///< clock source div  1
    ROOT_CLK_DIV_2 = SMU_CLK_SRC_DIV_2,  ///< clock source div  2
    ROOT_CLK_DIV_3 = SMU_CLK_SRC_DIV_3,  ///< clock source div  3
    ROOT_CLK_DIV_4 = SMU_CLK_SRC_DIV_4,  ///< clock source div  4
    ROOT_CLK_DIV_6 = SMU_CLK_SRC_DIV_6,  ///< clock source div  6
    ROOT_CLK_DIV_8 = SMU_CLK_SRC_DIV_8,  ///< clock source div  8
    ROOT_CLK_DIV_16 = SMU_CLK_SRC_DIV_16 ///< clock source div 16
} ROOT_CLK_DIV;


/**
 *  \enum APB_CLK_DIV
 *  \brief apb clock div
 */
typedef enum
{
    APB_CLK_DIV_1 = SMU_CLK_SRC_APB_DIV_1,  ///< APB clock div  1
    APB_CLK_DIV_2 = SMU_CLK_SRC_APB_DIV_2,  ///< APB clock div  2
    APB_CLK_DIV_3 = SMU_CLK_SRC_APB_DIV_3,  ///< APB clock div  3
    APB_CLK_DIV_4 = SMU_CLK_SRC_APB_DIV_4,  ///< APB clock div  4
    APB_CLK_DIV_6 = SMU_CLK_SRC_APB_DIV_6,  ///< APB clock div  6
    APB_CLK_DIV_8 = SMU_CLK_SRC_APB_DIV_8,  ///< APB clock div  8
    APB_CLK_DIV_16 = SMU_CLK_SRC_APB_DIV_16 ///< APB clock div 16
} APB_CLK_DIV;

/**
 *  \enum SMU_CLK
 *  \brief SMU clock
 */
typedef enum
{
    SMU_CLK_ROOT,                       ///< root clock
    SMU_CLK_CPU = SMU_CLK_ROOT,         ///< CPU clock
    SMU_CLK_AHB = SMU_CLK_ROOT,         ///< AHB clock
    SMU_CLK_SPI = SMU_CLK_AHB,          ///< APB clock
    SMU_CLK_APB,                        ///< SPI clock
    SMU_CLK_UART = SMU_CLK_APB,         ///< UART clock
    SMU_CLK_DOWN_CNT,                   ///< Sleep mode down count clock by 100K/AHB_DIV
    SMU_CLK_32K,                        ///< 32k clock
    SMU_CLK_RTC = SMU_CLK_32K,          ///< RTC clock
    SMU_CLK_PITPWM_EXT,                 ///< PITPWM external source 32K
    SMU_CLK_WDT_EXT                     ///< WDT external source 32K

} SMU_CLK;

/** 
 *  \enum SMU_DLDO
 *  \brief SMU DLDO
 */
typedef enum
{
    SMU_DLDO_1V0=0, ///< DLDO 1.0V
    SMU_DLDO_1V1,   ///< DLDO 1.1V
    SMU_DLDO_1V2,   ///< DLDO 1.2V
    SMU_DLDO_1V3    ///< DLDO 1.3V
} SMU_DLDO;

/**
 *  \enum SMU_POWER_MODE
 *  \brief SMU power mode
 */
typedef enum
{
    SMU_POWER_ACTIVE = 0,   ///< SMU power active
    SMU_POWER_IDLE,         ///< SMU power idle
    SMU_POWER_SLEEP,        ///< SMU power sleep
    SMU_POWER_DOWN          ///< SMU power down
} SMU_POWER_MODE;

/**
 *  \enum SMU_FW_TARGET
 *  \brief SMU firmware target
 */
typedef enum
{
    SMU_FW_ROM               = 1, ///< SMU firmware ROM
    SMU_FW_FLASH_BOOTLOADER  = 2, ///< SMU firmware flash bootloader
    SMU_FW_FLASH_APP         = 0  ///< SMU firmware flash application
} SMU_FW_TARGET;

// parameter definition for IP_sel
#define SMU_IPSEL_SPIM3   SMU_RST_SPIM3
#define SMU_IPSEL_SYSRAM3 SMU_RST_SYSRAM3
#define SMU_IPSEL_SYSRAM2 SMU_RST_SYSRAM2
#define SMU_IPSEL_SYSRAM  SMU_RST_SYSRAM
#define SMU_IPSEL_SPIM2   SMU_RST_SPIM2
#define SMU_IPSEL_SPIM1   SMU_RST_SPIM1
#define SMU_IPSEL_SPIS    SMU_RST_SPIS
#define SMU_IPSEL_CRYPTO  SMU_RST_CRYPTO
#define SMU_IPSEL_DMAC    SMU_RST_DMAC
#define SMU_IPSEL_USB2    SMU_RST_USB2
#define SMU_IPSEL_HWA     SMU_RST_HWA
#define SMU_IPSEL_HWA2    SMU_RST_HWA2
#define SMU_IPSEL_GPIO    SMU_RST_GPIO
#define SMU_IPSEL_PITPWM  SMU_RST_PITPWM
#define SMU_IPSEL_I2C     SMU_RST_I2C
#define SMU_IPSEL_UART    SMU_RST_UART
#define SMU_IPSEL_OTPC    SMU_RST_OTPC
#define SMU_IPSEL_WDT     SMU_RST_WDT
#define SMU_IPSEL_RTC     SMU_RST_RTC

// Sleep mode config
// bit define for SLEEP_MODE_CONFIG.wakeup_src
#define SLEEP_WAKEUP_ALL_EN           (SLEEP_WAKEUP_FOD_EN|SLEEP_WAKEUP_USB_EN|SLEEP_WAKEUP_SPIS_EN|SLEEP_WAKEUP_AOGPIO0_INT_EN|SLEEP_WAKEUP_AOGPIO1_INT_EN|SLEEP_WAKEUP_AOGPIO2_INT_EN|SLEEP_WAKEUP_CNT_INT_EN|SLEEP_WAKEUP_SPIS2_EN)
#define SLEEP_WAKEUP_FOD_EN           SMU_POWER_FOD_INT_EN
#define SLEEP_WAKEUP_USB_EN           SMU_POWER_USB_INT_EN
#define SLEEP_WAKEUP_AOGPIO0_INT_EN   SMU_POWER_AOGPIO0_INT_EN
#define SLEEP_WAKEUP_AOGPIO1_INT_EN   SMU_POWER_AOGPIO1_INT_EN
#define SLEEP_WAKEUP_AOGPIO2_INT_EN   SMU_POWER_AOGPIO2_INT_EN
#define SLEEP_WAKEUP_SPIS_EN          SMU_POWER_SPIS_INT_EN // enable SPIS_CSN active low wakeup source
#define SLEEP_WAKEUP_CNT_INT_EN       SMU_POWER_CNT_INT_EN
#define SLEEP_WAKEUP_SPIS2_EN         BIT(7)                // for ET171B, enable SPIS2_CSN active low wakeup source

// bit define for SLEEP_MODE_CONFIG.wakeup_fod_polarity
#define SLEEP_WAKEUP_FOD_ACTIVE_LOW   1     // SMU_POWER_FOD_INT_POLARITY_LOW
#define SLEEP_WAKEUP_FOD_ACTIVE_HIGH  0     // SMU_POWER_FOD_INT_POLARITY_HIGH

// bit define for SLEEP_MODE_CONFIG.wakeup_gpio_polarity
#define SLEEP_WAKEUP_GPIO_HIGH_LEVEL    0
#define SLEEP_WAKEUP_GPIO_LOW_LEVEL     1
#define SLEEP_WAKEUP_GPIO_RISING_EDGE   2
#define SLEEP_WAKEUP_GPIO_FALLING_EDGE  3


/**
 *  \brief       Get chip id version
 *  
 *  \return      chip id version: 0x1711 for ET171
 */
#define HAL_SMU_ChipVer()    (ET171_AOSMU->CHIP_VER)

/**
 *  \brief       Get clock from given clock source of SMU_CLK.
 *  
 *  \param[in]   clk_src  clock source
 *  
 *  \return      clock source for each HW IP, unit: Hz
 */
uint32_t HAL_SMU_GetClock(SMU_CLK clk_src);

/**
 *  \brief       Set root clock source and root/APB clock division.
 *  
 *  \param[in]   clk_sel  0: OSC_32K, 1: OSC_250M
 *  \param[in]   clk_div  set AHB clock divisor= 1~16
 *  \param[in]   apb_clk_div  set APB clock divisor= 1~16
 *  
 *  \return      none
 */
void HAL_SMU_SetRootClock(ROOT_CLK_SEL clk_sel, ROOT_CLK_DIV clk_div, APB_CLK_DIV apb_clk_div);

/**
 *  \brief       System reset (same as power on reset)
 *  
 *  \returns     none
 */
void HAL_SMU_SystemReset();

/**
 *  \brief       Reset HW IP
 *  
 *  \param[in]   IP_sel  bit-wise defined IP: SMU_IPSEL_***
 *  
 *  \returns     none
 */
void HAL_SMU_ResetIP(uint32_t IP_sel);

/**
 *  \brief       Reset HW IP to low
 *  
 *  \param[in]   IP_sel  bit-wise defined IP: SMU_IPSEL_***
 *  
 *  \returns     none
 */
void HAL_SMU_ResetLow(uint32_t IP_sel);

/**
 *  \brief       Reset HW IP to high
 *  
 *  \param[in]   IP_sel  bit-wise defined IP: SMU_IPSEL_***
 *  
 *  \returns     none
 */
void HAL_SMU_ResetHigh(uint32_t IP_sel);

/**
 *  \brief       clock gating for HW IP
 *  
 *  \param[in]   IP_sel  bit-wise defined IP: SMU_IPSEL_***
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_PowerDownIP(uint32_t IP_sel);

/**
 *  \brief       clock enable for HW IP
 *  
 *  \param[in]   IP_sel  bit-wise defined IP: SMU_IPSEL_***
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_PowerUpIP(uint32_t IP_sel);


/**
 *  \brief       Switch pad to GPIO or original function
 *  
 *  \param[in]   pad_sel        bit 3~16 for GPIO 3~~16
 *  \param[in]   gpio_enable    0: original pad function,\n
                                1: GPIO mode
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_GPIOPadMux(uint32_t pad_sel, BOOL gpio_enable);

/**
 *  \brief       Switch pad to I2C or original function
 *  
 *  \param[in]   mode       0: error,\n
 *                          1: pad 4,5,\n
 *                          2: pad 13,14,\n
 *                          3: error
 *  \param[in]   enable     0: original pad function,\n
                            1: I2C
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_I2CPadMux(uint8_t mode, BOOL enable);

/**
 *  \brief       Switch pad to UART or original function
 *  
 *  \param[in]   mode       0: pad 13,14,\n
                            1: pad 1,2,\n
                            2: error\n
                            3: error
 *  \param[in]   enable     0: original pad function,\n
                            1: UART
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_UARTPadMux(uint8_t mode, BOOL enable);

/**
 *  \brief       Switch pad to SPIS or original function.\n
 *  
 *               SPIS mode0 enable  => switch mode0 to spis and switch mode2 to original SPIM3.\n
 *               SPIS mode0 disable => not work, it is the original pad function already.\n
 *               SPIS mode2 enable  => switch mode2 to spis and switch mode0 to gpio if necessary.\n
 *               SPIS mode2 disable => switch mode2 to SPIM3. NOT automatically switch spis to mode0.
 *  
 *  \param[in]   mode       0: pad 21~24,\n
                            1: error,\n
                            2: pad 1,2,4,5,\n
                            3: error,
 *  \param[in]   enable     0: original pad function,\n
                            1: SPIS
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_SPISPadMux(uint8_t mode, BOOL enable);

/**
 *  \brief       Switch pad to PWM or original function
 *  
 *  \param[in]   pwmsel     pwm0 or pwm1
 *  \param[in]   enable     0: original pad function,\n
                            1: pwm
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_PWMPadMux(uint32_t pwmsel, BOOL enable);

/**
 *  \brief       Switch pad to Jtag or gpio
 *  
 *  \param[in]   enable     0: Jtag,\n
                            1: gpio
 *  
 *  \returns     HAL_STATUS
 */
HAL_STATUS HAL_SMU_JtagPadMux(BOOL enable);

/**
 *  \brief       Switch pad to Can Bus
 *  \param[in]   mode       2: pad SPIS MOSI MISO,\n
                            3: pad GPIO 1-2
 *  \param[in]   enable     0: SPIS,\n
                            1: Can
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_SMU_CanBusPadMux(uint8_t mode, BOOL enable);

/**
 *  \brief       Switch pad to Lin Bus
 *  \param[in]   enable     0: SPIS,\n
                            1: Lin
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_SMU_LinBusPadMux(BOOL enable);

/**
 *  \brief       DLDO 1.8V output enable/disable
 *  \param[in]   bEnable:   0: disable,\n
                            1: enable
 *  \return      none
 */
void HAL_SMU_LDO18_Enable(BOOL bEnable);

/**
 *  \brief       ALDO 3.0V output enable/disable
 *  \param[in]   bEnable:   0: disable,\n
                            1: enable
 *  \return      none
 */
void HAL_SMU_ALDO30_Enable(BOOL bEnable);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __ET171_HAL_SMU_H__ */
