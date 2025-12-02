#include <zephyr/logging/log.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>

// et171 core
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

// sxsymcrypt
#include <sxsymcrypt/trng.h>
#include <sxsymcrypt/statuscodes.h>

#define DT_DRV_COMPAT egis_et171_trng

LOG_MODULE_REGISTER(entropy, CONFIG_ENTROPY_LOG_LEVEL);

// ========================= entropy implementation
static struct {
    struct sx_trng_config trng_config;
} entropy_ctx;

static int entropy_egis_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
    uint32_t start, timeout = k_ms_to_cyc_floor32(50);
    size_t chunksz, remain = len;
    struct sx_trng ctx;
    int result, retry = 0;

retry_entry:
    if (sx_trng_init(&ctx, &entropy_ctx.trng_config) != SX_OK)
    {
        return -EIO;
    }

    start = k_cycle_get_32();
    while (remain > 0) {
        chunksz = remain > TRNG_MAX_CHUNK_SZ ? TRNG_MAX_CHUNK_SZ : remain;
        result = sx_trng_get2(&ctx, buf, &chunksz);
        //result = sx_trng_get(&ctx, (char *)buf, chunksz);
        if (result == SX_ERR_HW_PROCESSING) {
            if ((uint32_t)(k_cycle_get_32() - start) <= timeout) {
                continue;
            }
            return -EBUSY;
        }
        if (result != SX_OK)
        {
            if (retry < 2)
            {
                retry++;
                goto retry_entry;
            }
            return -EIO;
        }
        buf += chunksz;
        remain -= chunksz;
    }

    return 0;
}

static int entropy_egis_init(const struct device *dev)
{
    HAL_SMU_ResetIP(SMU_IPSEL_CRYPTO);

    OTP_TRNG_OPTION trng_setting;
    HAL_OTP_GetTRNGOptions(&trng_setting);
    entropy_ctx.trng_config.wakeup_level = 0;
    entropy_ctx.trng_config.init_wait = trng_setting.InitWaitVal;
    entropy_ctx.trng_config.off_time_delay = 0;
    entropy_ctx.trng_config.sample_clock_div = trng_setting.ClkDiv;
    entropy_ctx.trng_config.control_bitmask = 0;
    if (trng_setting.Ctrl_CondBypass)
    {
        entropy_ctx.trng_config.control_bitmask |= SX_TRNG_CONDITIONING_BYPASS;
    }
    if (trng_setting.Ctrl_HealthTestBypass)
    {
        entropy_ctx.trng_config.control_bitmask |= SX_TRNG_HEALTH_TEST_BYPASS;
    }
    if (trng_setting.Ctrl_AIS31Bypass)
    {
        entropy_ctx.trng_config.control_bitmask |= SX_TRNG_AIS31_TEST_BYPASS;
    }
    if (trng_setting.Ctrl_ForceRun)
    {
        entropy_ctx.trng_config.control_bitmask |= SX_TRNG_OSCILLATORS_FORCE_RUN;
    }
    if (trng_setting.Ctrl_LFSREn)
    {
        entropy_ctx.trng_config.control_bitmask |= SX_TRNG_LFSR_EN;
    }

    return 0;
}

static DEVICE_API(entropy, entropy_egis_api_funcs) = {
    .get_entropy = entropy_egis_get_entropy
};

DEVICE_DT_INST_DEFINE(0, entropy_egis_init, NULL, NULL, NULL,
        PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &entropy_egis_api_funcs);
