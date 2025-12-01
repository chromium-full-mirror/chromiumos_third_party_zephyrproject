/*
 * Copyright (c) 2025 Egistec Technology Inc.
 * All rights reserved.
 *
 */

#ifndef __ET171_H__
#define __ET171_H__
#include "et171_type.h"

#define SRAM_NONCACHE_OFFSET        (0x90000000UL)
#define SRAM2NONCACHE_ADDR(addr)    ((unsigned int)(addr) | SRAM_NONCACHE_OFFSET)
#define SRAM2CACHE_ADDR(addr)       ((unsigned int)(addr) & 0x0FFFFFFF)

#ifndef BIT
#define BIT(n)                      ((unsigned int) 1 << (n))
#endif

#ifndef BITS
#define BITS(m,n)                   (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))
#endif

#ifndef __ASSEMBLER__
/*****************************************************************************
 * SMU - ET171
 ****************************************************************************/
typedef struct {
    __I  unsigned int CHIP_VER;             /* 0x00 Chip version */
    __IO unsigned int CLK_SRC;              /* 0x04 Clock source selection and division */
    __I  unsigned int BOOTSTRAP;            /* 0x08 Boot strapping modes */
    __IO unsigned int SECURE_CON;           /* 0x0C Secure control */
    __IO unsigned int RESET_VECTOR;         /* 0x10 MCU core reset vector address */
    __IO unsigned int CLK_EN;               /* 0x14 Clock enable setting */
    __IO unsigned int SW_RST;               /* 0x18 Software reset */
    __IO unsigned int AO_REG;               /* 0x1C Reserved for future use */
    __IO unsigned int POWER_MODE;           /* 0x20 Power mode setting */
    __IO unsigned int POWER_MODE2;          /* 0x24 Power mode 2 setting */
    __IO unsigned int POWER_WAKE;           /* 0x28 Power wake-up setting */
    __IO unsigned int POWER_WAKE_STATUS;    /* 0x2C Power wake-up status (W1C) */
    __IO unsigned int PAD_OE_STABLE;        /* 0x30  */
    __I  unsigned int RESERVED1[3];
    __IO unsigned int PAD_DO_STABLE;        /* 0x40  */
    __I  unsigned int RESERVED2[3];
    __IO unsigned int PAD_PD;               /* 0x50  */
    __I  unsigned int RESERVED3[3];
    __IO unsigned int PAD_DS;               /* 0x60  */
    __I  unsigned int RESERVED4[3];
    __IO unsigned int PAD_IE;               /* 0x70  */
    __I  unsigned int RESERVED5[(0x200-0x74)/4];
    __IO unsigned int ANALOG_LDO30;         /* 0x200 ANALOG LDO30 */
    __IO unsigned int ANALOG_LDO18;         /* 0x204 ANALOG LDO18 */
    __IO unsigned int ANALOG_LDO11;         /* 0x208 ANALOG LDO11 */
    __IO unsigned int ANALOG_OSC100K;       /* 0x20C ANALOG OSC100K */
    __IO unsigned int ANALOG_OSC360M;       /* 0x210 ANALOG OSC360M */
    __I  unsigned int RESERVED6[(0x600-0x214)/4];
    __IO unsigned int USB2PHY;              /* 0x600 */
} AOSMU_RegDef;

#define SPIS_MPU_CTRL_LOCK           BIT(31)
#define SPIS_MPU_CTRL_INT_EN         BIT(9)
#define SPIS_MPU_CTRL_INT_PENDING    BIT(8)
#define SPIS_MPU_CTRL_RW             0
#define SPIS_MPU_CTRL_RO             1
#define SPIS_MPU_CTRL_WO             2
#define SPIS_MPU_CTRL_NA             3 // no any r/w permission

/* 0x04 CLK_SRC */
#define SMU_CLK_SRC_DIV_POS         4
#define SMU_CLK_SRC_DIV_MASK        0x70
#define SMU_CLK_SRC_DIV_1           0x00
#define SMU_CLK_SRC_DIV_2           0x10
#define SMU_CLK_SRC_DIV_3           0x20
#define SMU_CLK_SRC_DIV_4           0x30
#define SMU_CLK_SRC_DIV_6           0x40
#define SMU_CLK_SRC_DIV_8           0x50
#define SMU_CLK_SRC_DIV_16          0x60
#define SMU_CLK_SRC_APB_DIV_POS     1
#define SMU_CLK_SRC_APB_DIV_MASK    0x0E
#define SMU_CLK_SRC_APB_DIV_1       0x00
#define SMU_CLK_SRC_APB_DIV_2       0x02
#define SMU_CLK_SRC_APB_DIV_3       0x04
#define SMU_CLK_SRC_APB_DIV_4       0x06
#define SMU_CLK_SRC_APB_DIV_6       0x08
#define SMU_CLK_SRC_APB_DIV_8       0x0A
#define SMU_CLK_SRC_APB_DIV_16      0x0C
#define SMU_CLK_SRC_SEL             0x01
#define SMU_CLK_SRC_32K             0x00
#define SMU_CLK_SRC_250M            0x01

/* 0x08 BOOTSTRAP */
#define SMU_BOOT_STRAP_MODE_MASK    0x0F
#define SMU_BOOT_STRAP_ROM_BOOT     0x00 // 0x0: Normal function boot from ROM (0x7000_0000)
#define SMU_BOOT_STRAP_ROM_DFU      0x01 // 0x1: boot from ROM enter DFU mode
#define SMU_BOOT_STRAP_FLASH        0x02 // 0x2: Boot from SPI flash (0x8000_0000) (OTP should allow)
#define SMU_BOOT_STRAP_SRAM         0x03 // 0x3: Boot from SRAM (0x0000_0000) (OTP should allow)
#define SMU_BOOT_STRAP_SCAN_SAF     0x04 // 0x4: SCAN CP/FT
#define SMU_BOOT_STRAP_MEM_BIST     0x05 // 0x5: Memory BIST
#define SMU_BOOT_STRAP_USB2PHY_SCAN 0x06 // 0x6: USB2PHY SCAN
#define SMU_BOOT_STRAP_BYPASS_SPI   0x07 // 0x7: SPIS 4 pins go directly to SPIM1 4 pins
#define SMU_BOOT_STRAP_EXT_CLK_EN   0x08 // 0x8: Root clock from pad_io_sint (OTP should allow)
#define SMU_BOOT_STRAP_USB2PHY_BIST 0x09 // 0x9: USB2PHY BIST

/* 0x0C Secure control */
#define SMU_SECURE_SPIS_NO_ACCESS       BIT(31)
#define SMU_SECURE_BYPASS_BOOTSTRAP     BIT(17)
#define SMU_SECURE_TARGET_POS           16
#define SMU_SECURE_TARGET_MASK          (1 << SMU_SECURE_TARGET_POS)
#define SMU_SECURE_TARGET_BOOTLOADER    (1 << SMU_SECURE_TARGET_POS)
#define SMU_SECURE_TARGET_FLASH_APP     0
#define SMU_SECURE_SK_D_NO_READ_POS     13
#define SMU_SECURE_SK_D_NO_READ         BITS(13, 15)
#define SMU_SECURE_SK_D0_NO_READ        BIT(15)
#define SMU_SECURE_SK_D1_NO_READ        BIT(14)
#define SMU_SECURE_SK_D2_NO_READ        BIT(13)
#define SMU_SECURE_SK_D_DISABLE         BIT(10)
#define SMU_SECURE_SK_D_SEL_POS         8
#define SMU_SECURE_SK_D_SEL(i)          (((i)&3) << SMU_SECURE_SK_D_SEL_POS)
#define SMU_SECURE_SK_D_SEL_MASK        (3 << SMU_SECURE_SK_D_SEL_POS)
#define SMU_SECURE_SK_D_SIZE_POS        6
#define SMU_SECURE_SK_D_SIZE_128        (0 << SMU_SECURE_SK_D_SIZE_POS)
#define SMU_SECURE_SK_D_SIZE_256        (1 << SMU_SECURE_SK_D_SIZE_POS)
#define SMU_SECURE_SK_D_SIZE_192        (2 << SMU_SECURE_SK_D_SIZE_POS)
#define SMU_SECURE_SK_D_SIZE_MASK       (3 << SMU_SECURE_SK_D_SIZE_POS)
#define SMU_SECURE_FW_DEBUG_EN          BIT(5)  // for secure debug
#define SMU_SECURE_CORE_CLK_EN          BIT(4)  // for boot strapping = 4
#define SMU_SECURE_CPU_RST              BIT(3)
#define SMU_SECURE_WARM_RST             BIT(2)
#define SMU_SECURE_SYS_RST              BIT(1)

/* SMU 0x14 CLK_EN, 0x18 SW_RST*/
#define SMU_RST_SYSRAM1     BIT(21)
#define SMU_RST_LIN         BIT(20)
#define SMU_RST_CAN         BIT(19)
#define SMU_RST_SPIM3       BIT(18)
#define SMU_RST_SYSRAM3     BIT(17)
#define SMU_RST_SYSRAM2     BIT(16)
#define SMU_RST_SYSRAM      BIT(15)
#define SMU_RST_SPIM2       BIT(14)
#define SMU_RST_SPIM1       BIT(13)
#define SMU_RST_SPIS        BIT(12)
#define SMU_RST_CRYPTO      BIT(11)
#define SMU_RST_DMAC        BIT(10)
#define SMU_RST_USB2        BIT(9)
#define SMU_RST_HWA         BIT(8)
#define SMU_RST_HWA2        BIT(7)
#define SMU_RST_GPIO        BIT(6)
#define SMU_RST_PITPWM      BIT(5)
#define SMU_RST_I2C         BIT(4)
#define SMU_RST_UART        BIT(3)
#define SMU_RST_OTPC        BIT(2)
#define SMU_RST_WDT         BIT(1)
#define SMU_RST_RTC         BIT(0)

#define SMU_CLKEN_SYSRAM1   SMU_RST_SYSRAM1
#define SMU_CLKEN_LIN       SMU_RST_LIN
#define SMU_CLKEN_CAN       SMU_RST_CAN
#define SMU_CLKEN_SPIM3     SMU_RST_SPIM3
#define SMU_CLKEN_SYSRAM3   SMU_RST_SYSRAM3
#define SMU_CLKEN_SYSRAM2   SMU_RST_SYSRAM2
#define SMU_CLKEN_SYSRAM    SMU_RST_SYSRAM
#define SMU_CLKEN_SPIM2     SMU_RST_SPIM2
#define SMU_CLKEN_SPIM1     SMU_RST_SPIM1
#define SMU_CLKEN_SPIS      SMU_RST_SPIS
#define SMU_CLKEN_CRYPTO    SMU_RST_CRYPTO
#define SMU_CLKEN_DMAC      SMU_RST_DMAC
#define SMU_CLKEN_USB2      SMU_RST_USB2
#define SMU_CLKEN_HWA       SMU_RST_HWA
#define SMU_CLKEN_HWA2      SMU_RST_HWA2
#define SMU_CLKEN_GPIO      SMU_RST_GPIO
#define SMU_CLKEN_PITPWM    SMU_RST_PITPWM
#define SMU_CLKEN_I2C       SMU_RST_I2C
#define SMU_CLKEN_UART      SMU_RST_UART
#define SMU_CLKEN_OTPC      SMU_RST_OTPC
#define SMU_CLKEN_WDT       SMU_RST_WDT
#define SMU_CLKEN_RTC       SMU_RST_RTC

/* 0x1C AO_REG */
#define SMU_AO_SPIS2_CSN_NO_WAKE        BIT(30)
#define SMU_AO_WDT_ON_MASK              BITS(16, 17) // ET171C
#define SMU_AO_WDT_ON_POS               16
#define SMU_AO_WDT_ON                   BITS(16, 17)
#define SMU_AO_WDT_ON_1                 BIT(17)
#define SMU_AO_WDT_ON_0                 BIT(16)
#define SMU_AO_SPIS_CSN_NO_WAKE         BIT(15)
#define SMU_AO_RESET_EVENT_MASK         BITS(12, 14) // ET171C
#define SMU_AO_RESET_EVENT_POS          12
#define SMU_AO_RESET_EVENT_RSTN_PIN     BIT(14)
#define SMU_AO_RESET_EVENT_POR          BIT(13)
#define SMU_AO_RESET_EVENT_WDT          BIT(12)

#define SMU_AO_BYPASS_BROM_UPDATE       BIT(1)
#define SMU_AO_SLEEP_MODE               BIT(0)

/* 0x20 POWER_MODE */
#define SMU_POWER_PHY_P33READY_09V      BIT(31)
#define SMU_POWER_PHY_P09READY_33V_09V  BIT(30)
#define SMU_POWER_USB2_ISO_EN           BIT(29)
#define SMU_POWER_USB2_PWR_EN           BIT(28)
#define SMU_POWER_SRAM_RET_CTRL_POS     16
#define SMU_POWER_SRAM_RET_CTRL_MASK    BITS(16,27)
#define SMU_POWER_SEL_AOGPIO0_WAKE_POS  14
#define SMU_POWER_SEL_AOGPIO0_WAKE_MASK (0x3 << SMU_POWER_SEL_AOGPIO0_WAKE_POS)
#define SMU_POWER_SEL_AOGPIO1_WAKE_POS  12
#define SMU_POWER_SEL_AOGPIO1_WAKE_MASK (0x3 << SMU_POWER_SEL_AOGPIO1_WAKE_POS)
#define SMU_POWER_SEL_AOGPIO2_WAKE_POS  10
#define SMU_POWER_SEL_AOGPIO2_WAKE_MASK (0x3 << SMU_POWER_SEL_AOGPIO2_WAKE_POS)
#define SMU_POWER_FOD_INT_POLARITY_POS  9
#define SMU_POWER_FOD_INT_POLARITY_HIGH 0
#define SMU_POWER_FOD_INT_POLARITY_LOW  BIT(9)
#define SMU_POWER_FOD_INT_POLARITY_MASK BIT(9) 
#define SMU_POWER_FORCE_POWER_DOWN      BIT(2)
#define SMU_POWER_FORCE_SLEEP           BIT(1)
#define SMU_POWER_WFI_SLEEP             BIT(0)
#define SMU_POWER_WFI_IDLE              0

/* 0x24 POWER_MODE 2*/
#define SMU_POWER_PWR2RST_MASK          BITS(24, 31)
#define SMU_POWER_DLDO_DELTA_POS        20
#define SMU_POWER_DLDO_DELTA_MASK       BITS(20, 23)
#define SMU_POWER_CNT_INIT_MASK         BITS(0, 19)

/* 0x28 Power wake-up setting */
#define SMU_POWER_ALL_INT_EN_MASK       BITS(0,6)
#define SMU_POWER_FOD_INT_EN            BIT(6)
#define SMU_POWER_USB_INT_EN            BIT(5)
#define SMU_POWER_AOGPIO0_INT_EN        BIT(4)
#define SMU_POWER_AOGPIO1_INT_EN        BIT(3)
#define SMU_POWER_AOGPIO2_INT_EN        BIT(2)
#define SMU_POWER_SPIS_INT_EN           BIT(1)
#define SMU_POWER_CNT_INT_EN            BIT(0)

/* 0x2C Power wake-up status (W1C) */
#define SMU_POWER_ALL_INT_STATUS_MASK   BITS(0,6)
#define SMU_POWER_FOD_INT_STATUS        BIT(6)
#define SMU_POWER_USB_INT_STATUS        BIT(5)
#define SMU_POWER_AOGPIO0_INT_STATUS    BIT(4)
#define SMU_POWER_AOGPIO1_INT_STATUS    BIT(3)
#define SMU_POWER_AOGPIO2_INT_STATUS    BIT(2)
#define SMU_POWER_SPIS_INT_STATUS       BIT(1)
#define SMU_POWER_CNT_INT_STATUS        BIT(0)

/* 0x30 PAD_OE_STABLE*/
/* 0x40 PAD_DO_STABLE*/
/* 0x50 PAD_PD */
/* 0x60 PAD_DS */
/* 0x70 PAD_IE */
#define PAD(i)   BIT(i)
// pad name for default multi-function mode 0
#define PAD0_SPIM3_HOLDN    BIT(0)  //                         DBG0, boot_strapping[0]
#define PAD1_SPIM3_CK       BIT(1)  // UART_RX   SPIS_CK       DBG1
#define PAD2_SPIM3_MOSI     BIT(2)  // UART_TX   SPIS_MOSI     DBG2, boot_strapping[1]
#define PAD3_SPIM3_WPN      BIT(3)  //                         DBG3, boot_strapping[2]
#define PAD4_SPIM3_MISO     BIT(4)  //           SPIS_MISO     DBG4, boot_strapping[3]
#define PAD5_SPIM3_CSN      BIT(5)  //           SPIS_CSN      DBG5
#define PAD_SPIM3           BITS(0, 5)
#define PAD6_JTAG_TCK       BIT(6)  // GPIO3
#define PAD7_JTAG_TMS       BIT(7)  // GPIO4
#define PAD8_JTAG_TDI       BIT(8)  // GPIO5
#define PAD9_JTAG_TDO       BIT(9)  // GPIO6
#define PAD_JTAG            BITS(6, 9)
#define PAD10_AO_GPIO0      BIT(10) // (GPIO0)                 DBG6
#define PAD11_AO_GPIO1      BIT(11) // (GPIO1), I2C_SCL PWM2    CAN_TX  DBG7
#define PAD12_AO_GPIO2      BIT(12) // (GPIO2), I2C_SDA PWM3    CAN_RX  DBG8
#define PAD13_UART_RX       BIT(13) // GPIO7    I2C_SCL PWM0   DBG9
#define PAD14_UART_TX       BIT(14) // GPIO8    I2C_SDA PWM1   DBG10
#define PAD_SPIM1_MODE0     BITS(15, 20)
#define PAD15_SPIM1_WPN     BIT(15)
#define PAD16_SPIM1_MISO    BIT(16)
#define PAD17_SPIM1_CSN     BIT(17)
#define PAD18_SPIM1_HOLDN   BIT(18)
#define PAD19_SPIM1_CK      BIT(19)
#define PAD20_SPIM1_MOSI    BIT(20)
#define PAD_SPIS_MODE0      BITS(21, 24)
#define PAD21_SPIS_MOSI     BIT(21) // GPIO13, CAN_TX
#define PAD22_SPIS_MISO     BIT(22) // GPIO14, CAN_RX
#define PAD23_SPIS_CK       BIT(23) // GPIO15, LIN_TX
#define PAD24_SPIS_CSN      BIT(24) // GPIO16, LIN_RX
#define PAD25_SINT          BIT(25) // (GPIO9)
#define PAD26_SPIM2_MISO    BIT(26) //                         DBG11
#define PAD27_SRSTN         BIT(27) // (GPIO10)                DBG12
#define PAD28_SPIM2_MOSI    BIT(28) //                         DBG13
#define PAD29_SPIM2_CSN     BIT(29) // GPIO11                  DBG14
#define PAD30_SPIM2_CK      BIT(30)
#define PAD_SPIM2           (PAD26_SPIM2_MISO | PAD28_SPIM2_MOSI | PAD29_SPIM2_CSN | PAD30_SPIM2_CK)

#define SMU_GPIO_PIN_TO_PAD(gpio_pin)														 	 \
(																								 \
		((gpio_pin) & BITS( 0, 2)) ? (gpio_pin << 10			)/* GPIO_0~2   => PAD_10~12    */\
    :	((gpio_pin) & BITS( 3, 6)) ? (gpio_pin <<  3			)/* GPIO_3~6   => PAD_6~9	   */\
    :	((gpio_pin) & BITS( 7, 8)) ? (gpio_pin <<  6			)/* GPIO_7~8   => PAD_13~14    */\
    :	((gpio_pin) & BITS( 9,11)) ? ((gpio_pin * gpio_pin) << 7)/* GPIO_9~11  => PAD_25,27,29 */\
    :	((gpio_pin) & BIT(12)    ) ? (BIT(5)					)/* GPIO_12    => PAD_5        */\
	:	((gpio_pin) & BITS(13,16)) ? (gpio_pin << 8				)/* GPIO_13~16 => PAD_21~24    */\
    :	0																						 \
)
#define SMU_GPIO_NUM_TO_PAD(gpio_num) SMU_GPIO_PIN_TO_PAD(BIT(gpio_num))


// 0x200
#define SMU_ATOP_LDO1_SUSPEND_EN        BIT(23)

/* 0x200 ANALOG LDO30 */
#define SMU_ATOP_LDO_VERF_TRIM_POS      28
#define SMU_ATOP_LDO_VERF_TRIM_MASK     BITS(28,30)
#define SMU_ATOP_LDO30_TRIM_POS         24
#define SMU_ATOP_LDO30_TRIM_MASK        BITS(24,27)
#define SMU_ATOP_LDO_OCP_EN             BIT(23)
#define SMU_ATOP_PD_LDO30               BIT(22)
#define SMU_ATOP_SLEEP_LDO30_EN         BIT(21)
#define SMU_ATOP_BPLDO30_EN             BIT(20)

/* 0x204 ANALOG LDO18 */
#define SMU_ATOP_LDO18_TRIM_POS         28
#define SMU_ATOP_LDO18_TRIM_MASK        BITS(28,31)
#define SMU_ATOP_PD_LDO18               BIT(27)
#define SMU_ATOP_SLEEP_LDO18_EN         BIT(26)

/* 0x208 ANALOG LDO11 */
#define SMU_ATOP_DLDO_VERF_TRIM_POS     28
#define SMU_ATOP_DLDO_VERF_TRIM_MASK    BITS(28,30)
#define SMU_ATOP_DLDO_TRIM_POS          24
#define SMU_ATOP_DLDO_TRIM_MASK         BITS(24,27)
#define SMU_ATOP_PD_DLDO                BIT(23)
#define SMU_ATOP_SLEEP_DLDO_EN          BIT(22)

/* 0x20C ANALOG OSC100K */
#define SMU_ATOP_OSC100K_MON            BIT(30)
#define SMU_ATOP_OSC100K_DIV2           BIT(29)
#define SMU_ATOP_OSC100K_EN             BIT(28)
#define SMU_ATOP_OSC100K_FREQ_POS       24
#define SMU_ATOP_OSC100K_FREQ_MASK      BITS(24,27)

/* 0x210 ANALOG OSC360M */
#define SMU_ATOP_OSC360M_MON            BIT(31)
#define SMU_ATOP_OSC360M_EN             BIT(30)
#define SMU_ATOP_OSC360M_DIV2           BIT(29)
#define SMU_ATOP_OSC360M_POSTDIV2       BIT(28)
#define SMU_ATOP_OSC360M_FREQ_POS       20
#define SMU_ATOP_OSC360M_FREQ_MASK      BITS(20,25)

/* USB2PHY 0x600 */
#define SMU_USB2PHY_PLL_EN              BIT(21)
#define SMU_USB2PHY_WAKEUP              BIT(20) // Wakeup USB PHY

/*****************************************************************************
 * SMU2 - ET171
 ****************************************************************************/

typedef struct {
    __I  unsigned int RESERVED0[0x100/4];
    __IO unsigned int PAD_MUXA;             /* 0x100 Pad mux A*/
    __IO unsigned int PAD_MUXB;             /* 0x104 Pad mux B*/
    __I  unsigned int RESERVED1[2];
    __IO unsigned int DBG_MUX;              /* 0x110 Debug mux for HW IP*/
    __IO unsigned int DBG_IP;               /* 0x114 Debug HW IP */
    __I  unsigned int RESERVED2[(0x214-0x118)/4];
    __IO unsigned int ANALOG_MPX;           /* 0x214 ANALOG MPX */
    __IO unsigned int ANALOG_MISC;          /* 0x218 ANALOG PLL */
    __IO unsigned int THERMO;               /* 0x21C ANALOG THERMO */
    __I  unsigned int RESERVED3[(0x300-0x220)/4];
    __IO unsigned int MEMORY_MSE;           /* 0x300 SRAM/ROM margin enable */
    __IO unsigned int MEMORY_MS;            /* 0x304 SRAM/ROM margin setting */
    __I  unsigned int RESERVED4[(0x400-0x308)/4];
    __IO unsigned int SPIS_MPU_CTRL;        /* 0x400 SPIS MPU control register */
    __IO unsigned int SPIS_MPU_STATUS;      /* 0x404 SPIS MPU status */
    __IO unsigned int SPIS_MPU_ADDR[8];     /* 0x408 ~0x424 SPIS MPU region 0~3 address range */
    __I  unsigned int RESERVED5[(0x500-0x428)/4];
    __IO unsigned int FREQ_MEASURE;         /* 0x500 Frequency Measure */
    __IO unsigned int TEST_RESULT;          /* 0x504 Test Result */
    __I  unsigned int RESERVED6[(0x600-0x508)/4];
    __IO unsigned int USB2PHY[8];           /* 0x600 ~ 0x61C */
} SMU2_RegDef;

/*****************************************************************************
 * PIT - AE350
 ****************************************************************************/
typedef struct {
	__IO unsigned int CTRL;                 /* PIT Channel Control Register */
	__IO unsigned int RELOAD;               /* PIT Channel Reload Register */
	__IO unsigned int COUNTER;              /* PIT Channel Counter Register */
	__IO unsigned int RESERVED[1];
} PIT_CHANNEL_REG;

typedef struct {
	__I  unsigned int IDREV;                /* 0x00 ID and Revision Register */
	     unsigned int RESERVED[3];          /* 0x04 ~ 0x0C Reserved */
	__I  unsigned int CFG;                  /* 0x10 Configuration Register */
	__IO unsigned int INTEN;                /* 0x14 Interrupt Enable Register */
	__IO unsigned int INTST;                /* 0x18 Interrupt Status Register */
	__IO unsigned int CHNEN;                /* 0x1C Channel Enable Register */
	PIT_CHANNEL_REG   CHANNEL[4];           /* 0x20 ~ 0x50 Channel #n Registers */
} PIT_RegDef;

/*0x100 Pad MUX A*/

/*0x104 Pad MUX B*/

// 0x21C
#define SMU_THERMO_PD                   BIT(31)
#define SMU_THERMO_OUT                  BIT(30)
#define SMU_THERMO_TDAC_MASK            BITS(8, 11)
#define SMU_THERMO_TDAC_POS             8
#define SMU_THERMO_DAC_MASK             BITS(0, 4)

// 0x500
#define SMU_FREQMEA_REF_CLK_SEL_POS         30
#define SMU_FREQMEA_REF_CLK_USB2PHY_48M     0
#define SMU_FREQMEA_REF_CLK_SPIS_CK         (0x1 << SMU_FREQMEA_REF_CLK_SEL_POS)
#define SMU_FREQMEA_REF_CLK_JTAG_TCK        (0x2 << SMU_FREQMEA_REF_CLK_SEL_POS)
#define SMU_FREQMEA_REF_CLK_EXT_CLK         (0x3 << SMU_FREQMEA_REF_CLK_SEL_POS)
#define SMU_FREQMEA_TST_CLK_SEL_POS         29
#define SMU_FREQMEA_TST_CLK_RO_100K         0
#define SMU_FREQMEA_TST_CLK_RO_250M         (0x1 << SMU_FREQMEA_TST_CLK_SEL_POS)
#define SMU_FREQMEA_TST_CLK_MODE_POS        28
#define SMU_FREQMEA_TST_CLK_ONESHOT         0
#define SMU_FREQMEA_TST_CLK_CONTINUOUS      (0x1 << SMU_FREQMEA_TST_CLK_MODE_POS)

// 0x504
#define SMU_TEST_RESULT_MEASURE_EN      BIT(31)
#define SMU_TEST_RESULT_TEST_CLK_MASK   0xFFFFFF

/*****************************************************************************
 * ET171 OTPC
 ****************************************************************************/
typedef struct {
    __IO unsigned int CTRL;                 /* 0x00 OTP control register*/
    __IO unsigned int INTR;                 /* 0x04 OTP INT register */
    __IO unsigned int ADDR_LEN;             /* 0x08 address and length*/
    __IO unsigned int WDATA;                /* 0x0C OTP write data buffer 32-bit*/
    __IO unsigned int CFG[2];               /* 0x10, 0x14 configuration */
    __I  unsigned int RESERVED0[2];         /* 0x18, 0x1C reserved */
    __IO unsigned int DBG[2];               /* 0x20, 0x24 debug register to control OTP IO */
    __I  unsigned int RESERVED1[(0x1000-0x28)/4];        /* 0x28~ 0xFFC reserved */
    __IO unsigned int OTP[128];             /* 0x1000 OTP shadow ram 128*32bit read only*/

} OTPC_RegDef;

/* 0x00 CTRL */
#define OTPC_CTRL_DBG_EN        BIT(8)
#define OTPC_CTRL_INTR_EN       BIT(7)
#define OTPC_CTRL_OP_MASK       0x0000000F
#define OTPC_CTRL_OP_READ       1
#define OTPC_CTRL_OP_WRITE      2

/* 0x04 INTR */
#define OTPC_TIMEOUT_FLAG       BIT(31) //for OTPC driver timeout flash
#define OTPC_RELOAD_READY       BIT(15) //RO
#define OTPC_OVERWR_FLAG        BIT(2)  //W1C
#define OTPC_LOCK_FLAG          BIT(1)  //W1C
#define OTPC_INTR_FLAG          BIT(0)  //W1C

/* 0x08 ADDR_LEN */
#define OTPC_LEN_POS            16
#define OTPC_LEN_MASK           BIT_MASK(27,16)
#define OTPC_ADDR_POS           0
#define OTPC_ADDR_MASK          BIT_MASK(11,0)
/* 0x20 OTP_DEB_01 */
#define OTPC_DBG_PDSTB          BIT(24)

// Silex Insight
typedef struct {
    __IO unsigned int ControlReg;      // 0x0000 R/W ControlReg   Control register    0x00040000
    __IO unsigned int FIFOLevelReg;    // 0x0004 R/W FIFOLevelReg FIFO level register    0x00000000
    __IO unsigned int FIFOThreshReg;   // 0x0008 R/W FIFOThreshReg FIFO threshold register 2**(g_log21fodepth-2) - 1
    __I  unsigned int FODepthReg;      // 0x000C R FIFODepthReg FIFO depth register 2**g_log21fodepth
    __IO unsigned int Key0Reg;         // 0x0010 R/W Key0Reg Key register (MSB)    0x00000000
    __IO unsigned int Key1Reg;         // 0x0014 R/W Key1Reg Key register    0x00000000
    __IO unsigned int Key2Reg;         // 0x0018 R/W Key2Reg Key register    0x00000000
    __IO unsigned int Key3Reg;         // 0x001C R/W Key3Reg Key register (LSB)    0x00000000
    __IO unsigned int TestDataReg;     // 0x0020 W TestDataReg Test data register
    __IO unsigned int RepThresReg;     // 0x0024 R/W RepThresReg Repetition Count Threshold register    0x00000029
    __IO unsigned int PropThresReg;    // 0x0028 R/W PropThresReg Adaptive Proportion Threshold register (1024-sample window) 0x00000319
    __I  unsigned int RESERVED;        // 0x002C RESERVED
    __IO unsigned int StatusReg;       // 0x0030 R/W StatusReg Status register    0x00000000
    __IO unsigned int InitWaitVal;     // 0x0034 R/W InitWaitVal Initial wait counter value    0x0000FFFF
    __IO unsigned int DisableOsc0;     // 0x0038 R/W DisableOsc0 Disable oscillator rings    0x00000000
    __IO unsigned int DisableOsc1;     // 0x003C R/W DisableOsc1 Disable oscillator rings    0x00000000
    __IO unsigned int SwOffTmrVal;     // 0x0040 R/W SwOffTmrVal Switch off timer value    0x0000FFFF
    __IO unsigned int ClkDiv;          // 0x0044 R/W ClkDiv Sample clock divider    0x00000000
    __IO unsigned int AIS31Conf0;      // 0x0048 R/W AIS31Conf0 AIS31 con1guration register 0    0x43401040
    __IO unsigned int AIS31Conf1;      // 0x004C R/W AIS31Conf1 AIS31 con1guration register 1    0x03C00680
    __IO unsigned int AIS31Conf2;      // 0x0050 R/W AIS31Conf2 AIS31 con1guration register 2    0x04400340
    __IO unsigned int AIS31Status;     // 0x0054 R/W AIS31Status AIS31 status register    0x00000000
    __I  unsigned int HwConfig;        // 0x0058 R HwConfig Hardware configuration register    0x00000137
} TRNG_CTRL_RegDef;

/*****************************************************************************
 * PLMT - AE350
 ****************************************************************************/
 typedef struct {
	__IO unsigned long long MTIME;          /* 0x00 Machine Time */
	__IO unsigned long long MTIMECMP;       /* 0x08 Machine Time Compare */
} PLMT_RegDef;

/*****************************************************************************
 * Memory Map
 ****************************************************************************/

 #define _IO_(addr)              (addr)
 
#define AOSMU_BASE              _IO_(0xF0100000)
#define SMU2_BASE               _IO_(0xF0E00000)

// ET170/ET171
#define PIT_BASE                _IO_(0xF0400000)
#define OTPC_BASE               _IO_(0xF1000000)
#define PLMT_BASE               _IO_(0xE6000000)

/*****************************************************************************
 * Peripheral device declaration
 ****************************************************************************/

// ET171
#define AE350_PLMT              ((PLMT_RegDef *) PLMT_BASE)
#define AE350_PIT               ((PIT_RegDef *)  PIT_BASE)
#define ET171_OTPC              ((OTPC_RegDef *)  OTPC_BASE)
#define ET171_AOSMU             ((AOSMU_RegDef* ) AOSMU_BASE)
#define ET171_SMU2              ((SMU2_RegDef* ) SMU2_BASE)

#endif  /* __ASSEMBLER__ */

#endif /* __ET171_H__ */
