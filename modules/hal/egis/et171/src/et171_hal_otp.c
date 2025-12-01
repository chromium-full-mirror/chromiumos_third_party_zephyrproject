/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#include <et171.h>
#include <et171_type.h>
#include <et171_hal_utils.h>
#include <et171_hal_otp.h>
#include <debug.h>

//#define TEST_OTP_INT_MODE
#ifdef TEST_OTP_INT_MODE
    #define OTPC_INT_EN OTPC_CTRL_INTR_EN
#else
    #define OTPC_INT_EN 0
#endif

//#define DEBUG_OTP_RW_TIME
//#define DEBUG_OTP_RW_CMD

// Write Time : 8us (5~10us) per bit
//#define OTP_WRITE_TIMEOUT(bit_len)   (((bit_len) * 16)/1000 + 100)       // ms: 16us * len + 1ms
//#define OTP_WRITE_TIMEOUT(word_len)   ((word_len) * 32 * 16 / 1000 + 1000)     // ms: 16us * len + 2ms
#define OTP_WRITE_WORD_TIMEOUT          10  // 10 ms
// Read Data time : 200ns (5M Hz) per WORD
//#define OTP_READ_TIMEOUT(word_len)    ((word_len) * 400 / 1000000 + 1000)    // ms: 400ns * len + 2ms
#define OTP_READ_TIMEOUT(word_len)      10  // 10 ms

#ifdef TEST_OTP_INT_MODE
unsigned int gOTP_complete_status = 0;
void otpc_irq_handler()
{
    unsigned int status = ET171_OTPC->INTR;

    status &= (OTPC_OVERWR_FLAG | OTPC_LOCK_FLAG | OTPC_INTR_FLAG);
    if (status)
    {
        ET171_OTPC->INTR = ET171_OTPC->INTR;
        gOTP_complete_status = status;
    }
}
#endif // TEST_OTP_INT_MODE

#ifndef ROM_HAL
static uint32_t OTPC_polling_ready(uint32_t timeout)
{
    timeout = ms_to_tick(timeout);
#ifndef TEST_OTP_INT_MODE
    unsigned int gOTP_complete_status;
#endif //TEST_OTP_INT_MODE
    uint32_t ret = 0;
    register uint32_t start = HAL_read_mcycle32();

    do {
#ifdef TEST_OTP_INT_MODE
        if (gOTP_complete_status)
        {
#else // TEST_OTP_INT_MODE
        gOTP_complete_status = ET171_OTPC->INTR;
        if (gOTP_complete_status & (OTPC_OVERWR_FLAG | OTPC_LOCK_FLAG | OTPC_INTR_FLAG))
        {
            ET171_OTPC->INTR = gOTP_complete_status;
#endif //TEST_OTP_INT_MODE

#ifdef DEBUG_OTP_RW_TIME
            unsigned int t = tick_to_us(HAL_read_mcycle32() - start);
            DBG_PRINTF("spend: %d us\n", t);
#endif // DEBUG_OTP_RW_TIME
            ret = gOTP_complete_status;
            gOTP_complete_status = 0;
            return ret;
        }
    } while (HAL_read_mcycle32() - start < timeout);

    return ret | OTPC_TIMEOUT_FLAG;
}

/**
 *  \brief       Burst Write OTP 32-bit words
 *  \param[in]   word_addr      0~127
 *  \param[in]   word_len       1~128 words
 *  \param[in]   pData          pointer of 32-bit array
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_OTP_Write32(uint32_t word_addr, uint32_t word_len, uint32_t *pData)
{
    ASSERT(word_addr < 128 && word_len > 0 && (word_addr+word_len <= 128) && pData != NULL);
    uint32_t i;
    uint32_t otpc_flag;

#ifdef DEBUG_OTP_RW_CMD
    DBG_PRINTF("HAL_OTP_Write32: word index: %d, word len: %d\ndata:\n", (unsigned int)word_addr, (unsigned int)word_len);
    DBG_PRINT_BUF((uint8_t*)pData, 4*word_len);
#endif

    // clear INT, LOCK, OVER write
    ET171_OTPC->INTR = ET171_OTPC->INTR;

#ifdef TEST_OTP_INT_MODE
    // Priority must be set > 0 to trigger the interrupt
    __nds__plic_set_priority(IRQ_OTPC_SOURCE, 1);

    // Enable PLIC interrupt GPIO source
    __nds__plic_enable_interrupt(IRQ_OTPC_SOURCE);

    // Enable the Machine-External bit in MIE
    set_csr(NDS_MIE, MIP_MEIP);

    // Enable GIE
    set_csr(NDS_MSTATUS, MSTATUS_MIE);
    gOTP_complete_status = 0;
#endif

    otpc_flag = 0;
    for (i = word_addr; i < word_addr + word_len; i++)
    {
        ET171_OTPC->ADDR_LEN = i << 5;
        ET171_OTPC->WDATA = *pData++;
        ET171_OTPC->CTRL = OTPC_INT_EN | OTPC_CTRL_OP_WRITE;
        otpc_flag |= OTPC_polling_ready(OTP_WRITE_WORD_TIMEOUT);
    }

    if (otpc_flag & OTPC_TIMEOUT_FLAG)
    {
        return HAL_TIMEOUT;
    }
    else if (otpc_flag & OTPC_LOCK_FLAG)
    {
        return HAL_OTP_LOCKED;
    }
    return HAL_OK;
}

/**
 *  \brief       Burst read OTP 32-bit words
 *  \param[in]   word_addr      0~127
 *  \param[in]   word_len       1~128 words
 *  \param[out]  pData          pointer of 32-bit array
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_OTP_Read32(uint32_t word_addr, uint32_t word_len, uint32_t *pData)
{
    ASSERT(word_addr < 128 && word_len > 0 && (word_addr+word_len <= 128));
    uint32_t otpc_flag;
    uint32_t i;

    // clear INT, LOCK, OVER write
    ET171_OTPC->INTR = ET171_OTPC->INTR;

#ifdef TEST_OTP_INT_MODE
    // Priority must be set > 0 to trigger the interrupt
    __nds__plic_set_priority(IRQ_OTPC_SOURCE, 1);

    // Enable PLIC interrupt GPIO source
    __nds__plic_enable_interrupt(IRQ_OTPC_SOURCE);

    // Enable the Machine-External bit in MIE
    set_csr(NDS_MIE, MIP_MEIP);

    // Enable GIE
    set_csr(NDS_MSTATUS, MSTATUS_MIE);
    gOTP_complete_status = 0;
#endif

    ET171_OTPC->ADDR_LEN = ((word_len-1) << 16) | (word_addr << 5);

    ET171_OTPC->CTRL = OTPC_INT_EN | OTPC_CTRL_OP_READ;

    otpc_flag = OTPC_polling_ready(OTP_READ_TIMEOUT(word_len));

    if (pData != NULL)
    {
        // Copy OTP shadow RAM even if OTPC read fail
        for (i = word_addr; i < word_addr+word_len; i++)
        {
            *pData++ = ET171_OTPC->OTP[i];
        }
#ifdef DEBUG_OTP_RW_CMD
        DBG_PRINTF("HAL_OTP_Read32: word index: %d, word len: %d\ndata:\n", (unsigned int)word_addr, (unsigned int)word_len);
        DBG_PRINT_BUF((uint8_t*)(pData-word_len), 4*word_len);
#endif
    }
    if (otpc_flag & OTPC_TIMEOUT_FLAG)
    {
        return HAL_TIMEOUT;
    }
    return HAL_OK;
}
#endif // ROM_HAL

// Only for HAL OTP driver use internally
#define ET171_OTP ((ET171OTP_TypeDef*)(ET171_OTPC->OTP))
#ifndef ROM_HAL
void HAL_OTP_LoadAnalogConfig2(OTP_ANALOG_OPTION otp_analog)
{
    unsigned int reg;

    // Trim OSC360M
    reg = ET171_AOSMU->ANALOG_OSC360M & (~SMU_ATOP_OSC360M_FREQ_MASK);
    reg |= (otp_analog.otp_atop_osc360m_freq << SMU_ATOP_OSC360M_FREQ_POS);
    ET171_AOSMU->ANALOG_OSC360M = reg;
    // Trim DLDO11
    reg = ET171_AOSMU->ANALOG_LDO11 & (~(SMU_ATOP_DLDO_TRIM_MASK | SMU_ATOP_DLDO_VERF_TRIM_MASK));
    reg |= (otp_analog.otp_atop_dldo_trim << SMU_ATOP_DLDO_TRIM_POS) | (otp_analog.otp_atop_dldo_vref_trim << SMU_ATOP_DLDO_VERF_TRIM_POS);
    ET171_AOSMU->ANALOG_LDO11 = reg;
    // Trim OSC100K
    reg = ET171_AOSMU->ANALOG_OSC100K & (~SMU_ATOP_OSC100K_FREQ_MASK);
    reg |= (otp_analog.otp_atop_osc100k_freq << SMU_ATOP_OSC100K_FREQ_POS);
    ET171_AOSMU->ANALOG_OSC100K = reg;
    // Trim LDO18
    reg = ET171_AOSMU->ANALOG_LDO18 & (~SMU_ATOP_LDO18_TRIM_MASK);
    reg |= (otp_analog.otp_atop_ldo18_trim << SMU_ATOP_LDO18_TRIM_POS);
    ET171_AOSMU->ANALOG_LDO18 = reg;
    // Trim LDO30
    reg = ET171_AOSMU->ANALOG_LDO30 & (~(SMU_ATOP_LDO30_TRIM_MASK | SMU_ATOP_LDO_VERF_TRIM_MASK));
    reg |= (otp_analog.otp_atop_ldo30_trim << SMU_ATOP_LDO30_TRIM_POS) | (otp_analog.otp_atop_ldo_vref_trim << SMU_ATOP_LDO_VERF_TRIM_POS);
    ET171_AOSMU->ANALOG_LDO30 = reg;

}

HAL_STATUS HAL_OTP_WriteAnalogFromReg()
{
    OTP_ANALOG_OPTION otp_analog = {0};
    // Trim OSC360M
    otp_analog.otp_atop_osc360m_freq = (ET171_AOSMU->ANALOG_OSC360M & SMU_ATOP_OSC360M_FREQ_MASK) >> SMU_ATOP_OSC360M_FREQ_POS;
    // Trim DLDO11
    otp_analog.otp_atop_dldo_trim = (ET171_AOSMU->ANALOG_LDO11 & SMU_ATOP_DLDO_TRIM_MASK) >> SMU_ATOP_DLDO_TRIM_POS;
    otp_analog.otp_atop_dldo_vref_trim = (ET171_AOSMU->ANALOG_LDO11 & SMU_ATOP_DLDO_VERF_TRIM_MASK) >> SMU_ATOP_DLDO_VERF_TRIM_POS;
    // Trim OSC100K
    otp_analog.otp_atop_osc100k_freq = (ET171_AOSMU->ANALOG_OSC100K & SMU_ATOP_OSC100K_FREQ_MASK) >> SMU_ATOP_OSC100K_FREQ_POS;
    // Trim LDO18
    otp_analog.otp_atop_ldo18_trim = (ET171_AOSMU->ANALOG_LDO18 & SMU_ATOP_LDO18_TRIM_MASK) >> SMU_ATOP_LDO18_TRIM_POS;
    // Trim LDO30
    otp_analog.otp_atop_ldo30_trim = (ET171_AOSMU->ANALOG_LDO30 & SMU_ATOP_LDO30_TRIM_MASK) >> SMU_ATOP_LDO30_TRIM_POS;
    otp_analog.otp_atop_ldo_vref_trim = (ET171_AOSMU->ANALOG_LDO30 & SMU_ATOP_LDO_VERF_TRIM_MASK) >> SMU_ATOP_LDO_VERF_TRIM_POS;

    return HAL_OTP_Write32(0x18/4, 1, (uint32_t*)&(otp_analog._WORD));
}
#endif // ROM_HAL

OTP_ANALOG_OPTION HAL_OTP_GetAnalogConfig()
{
    return ET171_OTP->ANALOG;
}

extern uint32_t __g_et171_root_clock;
void HAL_OTP_GetRootClock()
{
    const uint32_t root_clock = ET171_OTP->UID[3];

    if (root_clock)
    {
        if (__g_et171_root_clock != root_clock) {
            __g_et171_root_clock = root_clock;
            HAL_measure_ext_clock(NULL);
        }
    }
    #if defined(CFG_FPGA) && defined(ROM_HAL)
    __g_et171_root_clock = 60 * MHz;
    #endif
}

#ifndef ROM_HAL
void HAL_OTP_LoadAnalogConfig()
{
    HAL_OTP_LoadAnalogConfig2(HAL_OTP_GetAnalogConfig());
}


/**
 *  \brief       Get OTP boot options from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  boot_config read boot rom options including OTP_FA_EN
 *  \return      HAL_STATUS
 */
void HAL_OTP_GetBootOptions(OTP_BROM_OPTION *boot_config)
{
    ASSERT(boot_config != NULL);
    boot_config->_WORD[0] = ET171_OTP->BROM_CTRL._WORD[0];
    boot_config->_WORD[1] = ET171_OTP->BROM_CTRL._WORD[1];
    boot_config->_WORD[2] = ET171_OTP->BROM_CTRL._WORD[2];

    if (ET171_OTP->HW_CTRL.otp_fa_en)
    {
        boot_config->secure_boot_en = ET171_OTP->HW_CTRL.otp_fa_secure_boot;
    }
}

/**
 *  \brief       Get OTP TRNG options from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  trng_setting read TRNG_Setting options
 *  \return      none
 */
void HAL_OTP_GetTRNGOptions(OTP_TRNG_OPTION *trng_setting)
{
    ASSERT(trng_setting != NULL);
    trng_setting->_WORD = ET171_OTP->TRNG_CTRL._WORD;
}

/**
 *  \brief       Get CP UID from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  UID  output first 16 bytes data in OTP
 *  \return      HAL_STATUS
 */
void HAL_OTP_GetUID(uint8_t UID[16])
{
    unsigned int *pDst = (unsigned int*)UID;
    int i;
    for (i = 0; i < 4; i++)
    {
        *pDst++ = ET171_OTP->UID[i];
    }
}

#endif // ROM_HAL
