/*
 * Copyright (c) 2025 Egis Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/kernel.h>
#include <errno.h>

// et171 core
#include <et171_hal/et171.h>
#include <et171_hal/et171_hal_smu.h>
#include <et171_hal/et171_hal_otp.h>

// sxsymcrypt
#include <sxsymcrypt/hash.h>
#include <sxsymcrypt/sha2.h>
#include <sxsymcrypt/statuscodes.h>


#define DT_DRV_COMPAT egis_et171_sha

LOG_MODULE_REGISTER(et171_crypto, CONFIG_CRYPTO_LOG_LEVEL);

struct et171_crypto_ctx {
    struct {
        struct sxhash ctx;
        uint32_t tmp_size;
        char tmp_msg[128];
    } hash;
};

static inline int et171_crpyto_query_hw_caps(const struct device *dev)
{
    return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int et171_hash_handler(struct hash_ctx *ctx, struct hash_pkt *pkt,
				bool finish)
{
    struct et171_crypto_ctx *data = ctx->device->data;

    int r = SX_OK;
    if (pkt->in_len) {
        r = sx_hash_feed(&data->hash.ctx, pkt->in_buf, pkt->in_len);
        if (r == SX_ERR_FEED_COUNT_EXCEEDED) {
            size_t tail_size = 0;
            char msg_buf[128];

            LOG_INF("sx_hash_feed count exceeded");
            r = sx_hash_spit_tail(&data->hash.ctx, msg_buf, &tail_size);
            if (r != SX_OK) {
                LOG_ERR("sx_hash_spit_tail fail %d", r);
                goto exit;
            }

            r = sx_hash_save_state(&data->hash.ctx);
            if (r == SX_ERR_WRONG_EMPTY) {
                r = SX_OK;
            } else if (r == SX_OK) {
                r = sx_hash_wait(&data->hash.ctx);
                if (r != SX_OK) {
                    LOG_ERR("sx_hash_wait( save ) fail %d", r);
                    goto exit;
                }
                r = sx_hash_resume_state(&data->hash.ctx);
                if (r != SX_OK) {
                    LOG_ERR("sx_hash_resume_state( save ) fail %d", r);
                    goto exit;
                }
            }
            if (tail_size) {
                memcpy(data->hash.tmp_msg, msg_buf, tail_size);
                data->hash.tmp_size = tail_size;
                r = sx_hash_feed(&data->hash.ctx, data->hash.tmp_msg, tail_size);
                if (r != SX_OK) {
                    LOG_ERR("sx_hash_feed( tmp ) fail %d", r);
                    goto exit;
                }
            } else {
                data->hash.tmp_size = 0;
            }
            r = sx_hash_feed(&data->hash.ctx, pkt->in_buf, pkt->in_len);
        }
        if (r != SX_OK) {
            LOG_ERR("sx_hash_feed( pkt ) fail %d", r);
            goto exit;
        }
    }

    if (finish) {
        r = sx_hash_digest(&data->hash.ctx, pkt->out_buf);
        if (r != SX_OK) {
            LOG_ERR("sx_hash_digest() fail %d", r);
            goto exit;
        }
        r = sx_hash_wait(&data->hash.ctx);
        if (r != SX_OK) {
            LOG_ERR("sx_hash_wait( digest ) fail %d", r);
            goto exit;
        }
    }

exit:
    if (r != SX_OK) {
        sx_hash_abandon(&data->hash.ctx);
        return -EINVAL;
    }
    return 0;
}

static int et171_hash_session_free(const struct device *dev,
				struct hash_ctx *ctx)
{
    struct et171_crypto_ctx *data = dev->data;

    sx_hash_abandon(&data->hash.ctx);

    return 0;
}

static int et171_hash_begin_session(const struct device *dev,
				struct hash_ctx *ctx, enum hash_algo algo)
{
    struct et171_crypto_ctx *data = dev->data;
    const struct sxhashalg *alg = NULL;
    switch (algo)
    {
    case CRYPTO_HASH_ALGO_SHA224:
        alg = &sxhashalg_sha2_224;
        break;
    case CRYPTO_HASH_ALGO_SHA256:
        alg = &sxhashalg_sha2_256;
        break;
    case CRYPTO_HASH_ALGO_SHA384:
        alg = &sxhashalg_sha2_384;
        break;
    case CRYPTO_HASH_ALGO_SHA512:
        alg = &sxhashalg_sha2_512;
        break;
    default:
        LOG_ERR("Unsupported algo");
        return -EINVAL;
    }

    if (ctx->flags & ~(et171_crpyto_query_hw_caps(dev))) {
        LOG_ERR("Unsupported flag");
        return -EINVAL;
    }

    int r = sx_hash_create(&data->hash.ctx, alg, sizeof(data->hash.ctx));
    if (r != SX_OK) {
        LOG_ERR("sx_hash_create( alg = %d ) fail %d", algo, r);
        return -EINVAL;
    }

    ctx->hash_hndlr = et171_hash_handler;

    return 0;
}

static int et171_crpyto_init(const struct device *dev)
{
    return 0;
}

static DEVICE_API(crypto, et171_crpyto_api) = {
	.hash_begin_session = et171_hash_begin_session,
	.hash_free_session = et171_hash_session_free,
	.query_hw_caps = et171_crpyto_query_hw_caps,
};

static struct et171_crypto_ctx et171_crypto_ctx_inst;

DEVICE_DT_INST_DEFINE(0, &et171_crpyto_init, NULL, &et171_crypto_ctx_inst, NULL, POST_KERNEL,
			CONFIG_CRYPTO_INIT_PRIORITY, &et171_crpyto_api);
