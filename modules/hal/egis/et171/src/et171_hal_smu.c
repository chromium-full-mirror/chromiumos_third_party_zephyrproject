/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#include <et171.h>
#include <et171_type.h>
#include <et171_hal_smu.h>
#include <debug.h>

#ifdef CFG_FPGA
#   define ET171_SOURCE_CLK (60*MHz)
#   define ET171_32K_CLK (32*KHz)
#else
#   define ET171_SOURCE_CLK (205*MHz)
#   define ET171_32K_CLK (102560 / 3) // 100KHz / 3
#endif

#ifndef ROM_HAL
uint32_t __g_et171_root_clock = ET171_SOURCE_CLK;
uint32_t __g_et171_exten_clock = ET171_32K_CLK;

#define IDLE_MODE_CLK_SRC    (SMU_CLK_SRC_32K | SMU_CLK_SRC_DIV_1 | SMU_CLK_SRC_APB_DIV_1)

// Root clock = cpu clock = AHB clock = SPI clock
// Root clock /2 = APB clock = UART clock
uint32_t HAL_SMU_GetClock(SMU_CLK clk_src)
{
    uint32_t root_clk;
    uint32_t apb_clk_div;
#ifdef CFG_FPGA
    root_clk = __g_et171_root_clock;
    apb_clk_div = 1;
#else // #ifdef CFG_FPGA
    const uint8_t clk_src_250M = (SMU_CLK_SRC_250M == (ET171_AOSMU->CLK_SRC & SMU_CLK_SRC_SEL));
    if (clk_src_250M && clk_src != SMU_CLK_DOWN_CNT)
    {
        root_clk = __g_et171_root_clock;
    }
    else
    {
        // When MCU use external clk source it will refer to original 100K.
        root_clk = __g_et171_exten_clock * 3;
    }

    switch(ET171_AOSMU->CLK_SRC & SMU_CLK_SRC_DIV_MASK)
    {
        case SMU_CLK_SRC_DIV_1:
        default:
            break;
        case SMU_CLK_SRC_DIV_2:
            root_clk /= 2;
            break;
        case SMU_CLK_SRC_DIV_3:
            root_clk /= 3;
            break;
        case SMU_CLK_SRC_DIV_4:
            root_clk /= 4;
            break;
        case SMU_CLK_SRC_DIV_6:
            root_clk /= 6;
            break;
        case SMU_CLK_SRC_DIV_8:
            root_clk /= 8;
            break;
        case SMU_CLK_SRC_DIV_16:
            root_clk /= 16;
            break;
    }

    switch(ET171_AOSMU->CLK_SRC & SMU_CLK_SRC_APB_DIV_MASK)
    {
        case SMU_CLK_SRC_APB_DIV_1:
            apb_clk_div = 1;
            break;
        case SMU_CLK_SRC_APB_DIV_2:
        default:
            apb_clk_div = 2;
            break;
        case SMU_CLK_SRC_APB_DIV_3:
            apb_clk_div = 3;
            break;
        case SMU_CLK_SRC_APB_DIV_4:
            apb_clk_div = 4;
            break;
        case SMU_CLK_SRC_APB_DIV_6:
            apb_clk_div = 6;
            break;
        case SMU_CLK_SRC_APB_DIV_8:
            apb_clk_div = 8;
            break;
        case SMU_CLK_SRC_APB_DIV_16:
            apb_clk_div = 16;
            break;
    }
#endif // #ifdef CFG_FPGA
    switch (clk_src)
    {
        case SMU_CLK_ROOT:
//        case SMU_CLK_CPU:
//        case SMU_CLK_AHB:
//        case SMU_CLK_SPI:
        case SMU_CLK_DOWN_CNT:
        default:
            return root_clk;
        case SMU_CLK_APB:
//        case SMU_CLK_UART:
            return (root_clk)/(apb_clk_div*2); // eq (root_clk/2)/apb_clk_div;
        case SMU_CLK_PITPWM_EXT:
        case SMU_CLK_WDT_EXT:
            if (!clk_src_250M) {
                // If MUC use 32K clk source then the ext will refer to APB / 3.
                
                // Because of the exteranl referance signal muse be 3 times slower than pclk for PIT & WDT.
                // When root_clock = __g_et171_exten_clock*3 then apb = (__g_et171_exten_clock*3/2)/apb_clk_div = (__g_et171_exten_clock*3)/(apb_clk_div*2)
                
                // So apb is always < (ext32k*3)
                // apb / (ext32k*3)
                // = ((__g_et171_exten_clock*3)/(apb_clk_div*2)) / (__g_et171_exten_clock*3)
                // = 1 / (apb_clk_div*2)               
                // because (apb_clk_div >= 1) so apb / (ext32k*3) < 1 => "apb is always < (ext32k*3)"

                // And PIT & WDT use 3 tick to check thier exteranl referance signal
                // That means the the finial freq would be the greatest common factor of APB / 3 & Ext32K. (when apb_clk_div is a power of 2)
                // [APB / 3, Ext32K]
                // = [(__g_et171_exten_clock*3)/(apb_clk_div*2) / 3, __g_et171_exten_clock]
                // = [__g_et171_exten_clock/(apb_clk_div*2),  __g_et171_exten_clock ] // because apb_clk_div is >= 1
                // = __g_et171_exten_clock/(apb_clk_div*2) = APB / 3

                return (root_clk)/(apb_clk_div*6) ; // eq ((root_clk/2)/apb_clk_div)/3;
            }
        case SMU_CLK_32K:
//        case SMU_CLK_RTC:
            return __g_et171_exten_clock;
    }
}

void HAL_SMU_SetRootClock(ROOT_CLK_SEL clk_sel, ROOT_CLK_DIV clk_div, APB_CLK_DIV apb_clk_div)
{
    ASSERT(clk_sel < 2
           && ((clk_div & (~SMU_CLK_SRC_DIV_MASK)) == 0)
           && ((apb_clk_div & (~SMU_CLK_SRC_APB_DIV_MASK)) == 0));
    uint32_t clk_setting = clk_sel | clk_div | apb_clk_div;
    if (ET171_AOSMU->CLK_SRC == clk_setting)
    {
        return;
    }

    ET171_AOSMU->CLK_SRC = clk_setting;
}

void HAL_SMU_SystemReset()
{
#ifdef CFG_ET171A
    ET171_SMU2->DBG_MUX = 0x9;
    ET171_SMU2->DBG_IP = 0xA;
#endif
    ET171_AOSMU->SECURE_CON |= SMU_SECURE_SYS_RST;
    while (1);
}

void HAL_SMU_ResetIP(uint32_t IP_sel)
{
    ASSERT((IP_sel & (~0x3FFFF)) == 0);
#ifndef CFG_FPGA
    ET171_AOSMU->CLK_EN &= ~IP_sel;
    ET171_AOSMU->SW_RST &= ~IP_sel;
    ET171_AOSMU->SW_RST |= IP_sel;
    ET171_AOSMU->CLK_EN |= IP_sel;
#endif //#ifndef CFG_FPGA
    // check HW IP version reg and return error code
}

void HAL_SMU_ResetLow(uint32_t IP_sel)
{
#ifndef CFG_FPGA
    ET171_AOSMU->CLK_EN &= ~IP_sel;
    ET171_AOSMU->SW_RST &= ~IP_sel;
#endif //#ifndef CFG_FPGA
}

void HAL_SMU_ResetHigh(uint32_t IP_sel)
{
#ifndef CFG_FPGA
    ET171_AOSMU->SW_RST |= IP_sel;
    ET171_AOSMU->CLK_EN |= IP_sel;
#endif //#ifndef CFG_FPGA
}

HAL_STATUS HAL_SMU_PowerDownIP(uint32_t IP_sel)
{
    ASSERT((IP_sel & (~0x3FFFF)) == 0);
#ifndef CFG_FPGA
    ET171_AOSMU->CLK_EN &= ~IP_sel;
    //ET171_AOSMU->SW_RST &= ~IP_sel;
#endif //#ifndef CFG_FPGA
    // check HW IP version reg and return error code
    return HAL_OK;
}

HAL_STATUS HAL_SMU_PowerUpIP(uint32_t IP_sel)
{
    ASSERT((IP_sel & (~0x3FFFF)) == 0);
    //ET171_AOSMU->SW_RST |= IP_sel;
    ET171_AOSMU->CLK_EN |= IP_sel;
    // check HW IP version reg and return error code
    return HAL_OK;
}
#endif // ROM_HAL

HAL_STATUS HAL_SMU_GPIOPadMux(uint32_t pad_sel, BOOL gpio_enable)
{
    ASSERT((pad_sel & 0xFFFE0007) == 0);
    
    if ((pad_sel & 0xFFFE0007) != 0)
    {
        return HAL_INVALID_PARAM;
    }
    if (gpio_enable)
    {
        if (pad_sel & 0x00000078)   // gpio 3~6
        {
            ET171_SMU2->PAD_MUXA |= (pad_sel * pad_sel) << 6;
        }
        if (pad_sel & 0x00000180)   // gpio 7~8
        {
            ET171_SMU2->PAD_MUXA |= (pad_sel * pad_sel) << 12;
        }
        if (pad_sel & 0x00000800)   // gpio 11
        {
            ET171_SMU2->PAD_MUXB |= 0x04000000;
        }
        if (pad_sel & 0x00001000)   // gpio 12
        {
            ET171_SMU2->PAD_MUXA |= 0x00000C00;
        }
        if (pad_sel & 0x0001E000)   // gpio 13~16
        {
            pad_sel >>= 8;
            ET171_SMU2->PAD_MUXB |= pad_sel * pad_sel;
        }
    }
    else
    {
        if (pad_sel & 0x00000078)   // gpio 3~6
        {
            ET171_SMU2->PAD_MUXA &= ~((pad_sel * pad_sel) << 6);
        }
        if (pad_sel & 0x00000180)   // gpio 7~8
        {
            ET171_SMU2->PAD_MUXA &= ~((pad_sel * pad_sel) << 12);
        }
        if (pad_sel & 0x00000800)   // gpio 11
        {
            ET171_SMU2->PAD_MUXB &= ~0x04000000;
        }
        if (pad_sel & 0x00001000)   // gpio 12
        {
            ET171_SMU2->PAD_MUXA &= ~0x00000C00;
        }
        if (pad_sel & 0x0001E000)   // gpio 13~16
        {
            pad_sel >>= 8;
            ET171_SMU2->PAD_MUXB &= ~(pad_sel * pad_sel);
        }
    }
    return HAL_OK;
}
HAL_STATUS HAL_SMU_I2CPadMux(uint8_t mode, BOOL enable)
{
    ASSERT((mode == 1) || (mode == 2));
    
    if ((mode == 0) || (mode == 3))
    {
        return HAL_INVALID_PARAM;
    }
    if (mode == 1)
    {
        ET171_SMU2->PAD_MUXA &= ~0x03C00000;
        if (enable)
        {
            ET171_SMU2->PAD_MUXA |= 0x01400000;
        }
    }
    else
    {
        ET171_SMU2->PAD_MUXA &= ~0x3C000000;
        if (enable)
        {
            ET171_SMU2->PAD_MUXA |= 0x28000000;
        }
    }
    return HAL_OK;
}
HAL_STATUS HAL_SMU_UARTPadMux(uint8_t mode, BOOL enable)
{
    ASSERT(mode < 2);
    if (mode >=2)
    {
        return HAL_INVALID_PARAM;
    }
    if (mode == 1)
    {
        ET171_SMU2->PAD_MUXA &= ~0x0000003C;
        if (enable)
        {
            ET171_SMU2->PAD_MUXA |= 0x00000014;
        }
        if ((ET171_SMU2->PAD_MUXA & 0x3C000000) == 0)
        {
            ET171_SMU2->PAD_MUXA |= 0x14000000;
        }
    }
    else
    {
        ET171_SMU2->PAD_MUXA &= ~0x3C000000;
    }
    return HAL_OK;
}
#ifndef ROM_HAL
HAL_STATUS HAL_SMU_SPISPadMux(uint8_t mode, BOOL enable)
{
    ASSERT((mode == 0) || (mode == 2));
    
    if (mode & 0x01)
    {
        return HAL_INVALID_PARAM;
    }
    if (mode == 2)
    {
        ET171_SMU2->PAD_MUXA &= ~0x00000F3C;
        if (enable)
        {
            ET171_SMU2->PAD_MUXA |= 0x00000A28;
            if ((ET171_SMU2->PAD_MUXB & 0x0003FC00) == 0)
            {
                ET171_SMU2->PAD_MUXB |= 0x00015400;
            }
        }
    }
    else
    {
        ET171_SMU2->PAD_MUXA &= ~0x00000F3C;
        ET171_SMU2->PAD_MUXB &= ~0x0003FC00;
    }
    return HAL_OK;
}
#endif // ROM_HAL
HAL_STATUS HAL_SMU_PWMPadMux(uint32_t pwmsel, BOOL enable)
{
    ASSERT(pwmsel <= 3);
    
    if (pwmsel > 3)
    {
        return HAL_INVALID_PARAM;
    }

    // 0: pwm0: 0x0C000000
    // 1: pwm1: 0x30000000
    // 2: pwm2: 0x00800000
    // 3: pwm3: 0x02000000
    uint32_t bitmask;
    if (pwmsel < 2)
    {
        bitmask = (3 << (pwmsel<<1)) << 26;
    }
    else
    {
        bitmask = (2 << ((pwmsel&0x01)<<1)) << 22;
    }

    ET171_SMU2->PAD_MUXA &= ~bitmask;
    if (enable)
    {
        ET171_SMU2->PAD_MUXA |= bitmask;
        static const uint8_t pad_no[] = { 13, 14, 11, 12 };
        const uint32_t PAD_pin = pad_no[pwmsel];
        ET171_AOSMU->PAD_DO_STABLE = PAD_pin;
        ET171_AOSMU->PAD_OE_STABLE = PAD_pin;
        ET171_AOSMU->PAD_IE &= ~PAD_pin;
    }
    return HAL_OK;
}

HAL_STATUS HAL_SMU_JtagPadMux(BOOL enable)
{
    ET171_SMU2->PAD_MUXA &= ~0x000FF000;
    if (!enable)
    {
        ET171_SMU2->PAD_MUXA |= 0x00055000;
    }
    return HAL_OK;
}

HAL_STATUS HAL_SMU_CanBusPadMux(uint8_t mode, BOOL enable)
{
    ASSERT((mode == 2) || (mode == 3));
    if (mode == 2)
    {
        ET171_SMU2->PAD_MUXB &= ~0x00003C00;
        if (enable)
        {
            ET171_SMU2->PAD_MUXB |= 0x00002800;
        }
    }
    else if (mode == 3)
    {
        ET171_SMU2->PAD_MUXA &= ~0x03C00000;
        if (enable)
        {
            ET171_SMU2->PAD_MUXA |= 0x03C00000;
        }
    }
    else
    {
        return HAL_INVALID_PARAM;
    }
    return HAL_OK;
}

HAL_STATUS HAL_SMU_LinBusPadMux(BOOL enable)
{
    ET171_SMU2->PAD_MUXB &= ~0x0003C000;
    if (enable)
    {
        ET171_SMU2->PAD_MUXB |= 0x00028000;
    }
    return HAL_OK;
}

/**
 *  \brief       warm reset to address (system reset except reset vector(0x10) and SMU register(0x1c)
 *  \param[in]   __reset_vector  reset address, there is a global variable "reset_vector" can be used.
 *  \return      none
 */
#ifndef ROM_HAL
void HAL_SMU_WarmReset(uint32_t __reset_vector)
{
    ET171_AOSMU->SECURE_CON &= ~SMU_SECURE_TARGET_MASK;
    ET171_AOSMU->RESET_VECTOR = (unsigned int)__reset_vector;
    ET171_AOSMU->SECURE_CON |= SMU_SECURE_WARM_RST;
    while (1);
}
#endif // ROM_HAL

void HAL_SMU_LDO18_Enable(BOOL bEnable)
{
    if (bEnable)
    {
        ET171_AOSMU->ANALOG_LDO18 &= ~SMU_ATOP_PD_LDO18;
    }
    else
    {
        ET171_AOSMU->ANALOG_LDO18 |= SMU_ATOP_PD_LDO18;
    }
}


/**
 *  \brief       get pad 0~30 multi-function 0~3
 *  \return      multi-function 0~3
 */
unsigned int HAL_SMU_Get_pad_func(unsigned int pad_index)
{
    ASSERT(pad_index < 31);
    if (pad_index < 16)
    {
        return ((ET171_SMU2->PAD_MUXA >> (pad_index * 2)) & 0x3);
    }
    else
    {
        return ((ET171_SMU2->PAD_MUXB >> ((pad_index - 16) * 2)) & 0x3);
    }
}

/**
 *  \brief       set pad 0~30 multi-function 0~3
 *  \return      none
 */
void HAL_SMU_Set_pad_func(unsigned int pad_index, unsigned int func)
{
    ASSERT(pad_index < 31);
    volatile unsigned int *pReg;
    unsigned int reg;
    if (pad_index < 16)
    {
        pReg = &(ET171_SMU2->PAD_MUXA);
    }
    else
    {
        pad_index -= 16;
        pReg = &(ET171_SMU2->PAD_MUXB);
    }
    reg = *pReg;
    reg &= ~(3 << (pad_index * 2));
    reg |= (func << (pad_index * 2));
    *pReg = reg;
}

void HAL_SMU_ALDO30_Enable(BOOL bEnable)
{
    if (bEnable)
    {
        ET171_AOSMU->ANALOG_LDO30 |= SMU_ATOP_LDO_OCP_EN;
        ET171_AOSMU->ANALOG_LDO30 &= ~SMU_ATOP_PD_LDO30;
    }
    else
    {
        ET171_AOSMU->ANALOG_LDO30 |= SMU_ATOP_PD_LDO30;
        ET171_AOSMU->ANALOG_LDO30 &= ~SMU_ATOP_LDO_OCP_EN;
    }
}
