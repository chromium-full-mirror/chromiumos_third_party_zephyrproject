/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

/**
 * \file
 * \brief ET171 HAL OTP
 */
#ifndef __ET171_HAL_OTP_H__
#define __ET171_HAL_OTP_H__

#include "et171.h"
#include "et171_type.h"
#include "et171_hal_def.h"

/*
 *  \struct OTP_HW_OPTION
 *  \brief Define ET171 OTP option
 *
 *  OTP base + offset 0x10, 0x14
 */
typedef struct
{
    union
    {
        unsigned int _WORD[2];
        struct
        {
            // 0x10
            unsigned int otp_force_brom: 1;
            unsigned int otp_debug_disable: 1;
            unsigned int otp_debug_secure: 1;
            unsigned int otp_extclk_disable: 1;
            unsigned int : 28;
            // 0x14
            unsigned int otp_fa_en: 1;
            unsigned int otp_fa_secure_boot: 1;
            unsigned int otp_fa_force_brom: 1;
            unsigned int otp_fa_debug_disable: 1;
            unsigned int otp_fa_debug_secure: 1;
            unsigned int : 27;
        };
    };
} OTP_HW_OPTION;

/*
 *  \struct OTP_ANALOG_OPTION
 *  \brief Define ET171 OTP analog option
 *
 *  OTP base + offset 0x18
 */
typedef struct
{
    union
    {
        unsigned int _WORD;
        struct
        {
            // 0x18: analog settings with lock // 29 bits + 3 reserved
            unsigned int otp_atop_ldo30_trim: 4;
            unsigned int otp_atop_ldo_vref_trim: 3;
            unsigned int otp_atop_ldo18_trim: 4;
            unsigned int otp_atop_dldo_trim: 4;
            unsigned int otp_atop_dldo_vref_trim: 3;
            unsigned int otp_atop_osc100k_freq: 4;
            unsigned int otp_atop_osc360m_freq: 6;
            unsigned int spis_pad_mux: 1;
            unsigned int RESERVED: 3;
        };
    };
} OTP_ANALOG_OPTION;

/**
 *  \struct OTP_BROM_OPTION
 *  \brief Define ET171 OTP BROM option
 *
 *  OTP base + offset 0x1C, 0x20, 0x24
 *  refer to manual "ET171_boot_rom_design_specification - ECOB.docx" CH7
 */
typedef struct
{
    union
    {
        unsigned int _WORD[3];
        struct
        {
            // 0x1C: BROM_Setting1 with lock // 32 bits
            unsigned int secure_boot_en: 1;
            unsigned int secure_boot_signature_sel: 1; //0:HMAC, 1:ECDSA
            unsigned int secure_boot_app_first: 1;
            unsigned int secure_boot_warm_reset_disable: 1;
            unsigned int smu_root_clk_div: 3;
            unsigned int smu_apb_clk_div: 3;
            unsigned int wdt_time: 4;
            unsigned int flash_spim1_timing: 14;
            unsigned int flash_spim1_cmd: 4;
            // 0x20: BROM_Setting2  // 20 bits + 12 reserved
            unsigned int flash_spim3_timing: 14;
            unsigned int flash_spim3_cmd: 4;
            unsigned int sdcp_claim_disable: 1;
            unsigned int brom_update_disable: 1;
            unsigned int RESERVED1: 12;
            // 0x24: BROM_Setting3 without lock // 9 bits + 23 reserved
            unsigned int secure_channel_enable: 1;
            unsigned int secure_algo_SHA256_disable: 1;
            unsigned int secure_algo_HMAC_disable: 1;
            unsigned int secure_spi_slave_enable: 1;
            unsigned int anti_rollback_enable: 1;
            unsigned int download_sram_disable: 1;
            unsigned int otp_sk_d_no_read: 3;
            unsigned int RESERVED2: 23;
        };
    };
} OTP_BROM_OPTION;

/**
 *  \struct OTP_TRNG_OPTION
 *  \brief Define ET171 OTP TRNG option
 *
 *  OTP base + offset 0x28
 *  refer to manual "ET171_boot_rom_design_specification - ECOB.docx" CH7
 */
typedef struct
{
    union
    {
        unsigned int _WORD;
        struct
        {
            // 0x28: TRNG settings with lock // 29 bits + 3 reserved
            unsigned int InitWaitVal: 16;
            unsigned int ClkDiv: 8;
            unsigned int Ctrl_CondBypass : 1;       // control bit 3
            unsigned int Ctrl_HealthTestBypass : 1; // control bit 12
            unsigned int Ctrl_AIS31Bypass : 1;      // control bit 13
            unsigned int Ctrl_ForceRun : 1;         // control bit 11
            unsigned int Ctrl_LFSREn : 1;           // control bit 1
            unsigned int RESERVED: 3;
        };
    };
} OTP_TRNG_OPTION;

#define CP_UID_LEN 9

/*
 *  \struct ET171OTP_TypeDef
 *  \brief typedefine ET171 OTP
 *
 */
typedef struct
{
    unsigned int        UID[4];                     // 0x00~0x0F: CP UID 72-bit (reserved 128-bit space)
    OTP_HW_OPTION       HW_CTRL;                    // 0x10, 0x14: OTP HW control and FA control
    OTP_ANALOG_OPTION   ANALOG;                     // 0x18: analog settings
    OTP_BROM_OPTION     BROM_CTRL;                  // 0x1C, 0x20, 0x24: OTP options for boot ROM
    OTP_TRNG_OPTION     TRNG_CTRL;                  // 0x28: TRNG settings
    unsigned int        RESERVED0[(0x130-0x2C)/4];  // 0x2C~0x12F: reserved 65 words
    unsigned int        RESERVED1[3][8];            // 0x130~0x18F: reserved 96 words
    unsigned int        RESERVED2[3][8];            // 0x190~0x1EF: reserved 96 words
    unsigned int        OTP_LOCK[4];                // 0x1F0~0x1FF: OTP lock bit for each words
} ET171OTP_TypeDef;


// ET171 OTP 128*32-bit, default 0, write to 1, over-burn protection
#define MAX_OTP_WORD_SIZE    (128)

/**
 *  \brief       Burst write OTP 32-bit words
 *  \param[in]   word_addr      0~127
 *  \param[in]   word_len       1~128 words
 *  \param[in]   pData          pointer of 32-bit array
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_OTP_Write32(uint32_t word_addr, uint32_t word_len, uint32_t *pData);

/**
 *  \brief       Burst read OTP 32-bit words
 *  \param[in]   word_addr      0~127
 *  \param[in]   word_len       1~128 words
 *  \param[out]  pData          pointer of 32-bit array
 *  \return      HAL_STATUS
 */
HAL_STATUS HAL_OTP_Read32(uint32_t word_addr, uint32_t word_len, uint32_t *pData);

/**
 *  \brief       Load OTP analog config options to AOSMU registers
 *               It won't read OTP again.
 *  \return      none
 */
void HAL_OTP_LoadAnalogConfig();

/**
 *  \brief       Get OTP boot options from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  boot_config read boot rom options (some options are depends on HW setting and FA setting)
 *  \return      none
 */
void HAL_OTP_GetBootOptions(OTP_BROM_OPTION *boot_config);

/**
 *  \brief       Get OTP TRNG options from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  trng_setting read TRNG_Setting options
 *  \return      none
 */
void HAL_OTP_GetTRNGOptions(OTP_TRNG_OPTION *trng_setting);

/**
 *  \brief       Get CP UID from OTP shadow ram
 *               It won't read OTP again
 *  \param[out]  UID  output first 16 bytes data in OTP
 *  \return      none
 */
void HAL_OTP_GetUID(uint8_t UID[16]);

/**
 *  \brief       Get root clock from OTP
 *  \returns     none
 */
void HAL_OTP_GetRootClock();

// for verification
/**
 *  \brief       Get analog config from OTP
 *  \returns     OTP_ANALOG_OPTION structure
 */
OTP_ANALOG_OPTION HAL_OTP_GetAnalogConfig();

// Only for HAL OTP driver use internally
void HAL_OTP_LoadAnalogConfig2(OTP_ANALOG_OPTION otp_analog);
HAL_STATUS HAL_OTP_WriteAnalogFromReg();

#endif /* __ET171_HAL_OTP_H__ */
