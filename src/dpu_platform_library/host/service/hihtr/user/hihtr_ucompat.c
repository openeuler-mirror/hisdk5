/*
 * Copyright (c) 2025 Hisilicon Limited. All rights reserved.
 */

#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "hihtr_ucompat.h"

#if !defined(ILOG_HAVE_ILOG32)
int ilog32(uint32_t _v)
{
    int ret;
    int m;
    ret=_v>0;
    m=(_v>0xFFFFU)<<NUM_4;
    _v>>=m;
    ret|=m;
    m=(_v>0xFFU)<<NUM_3;
    _v>>=m;
    ret|=m;
    m=(_v>0xFU)<<NUM_2;
    _v>>=m;
    ret|=m;
    m=(_v>NUM_3)<<1;
    _v>>=m;
    ret|=m;
    ret+=_v>1;
    return ret;
}

int ilog64(uint64_t _v)
{
    uint32_t v;
    int      ret;
    int      m;
    ret=_v>0;
    m=(_v>0xFFFFFFFFU)<<NUM_5;
    v=(uint32_t)(_v>>m);
    ret|=m;
    m=(v>0xFFFFU)<<NUM_4;
    v>>=m;
    ret|=m;
    m=(v>0xFFU)<<NUM_3;
    v>>=m;
    ret|=m;
    m=(v>0xFU)<<NUM_2;
    v>>=m;
    ret|=m;
    m=(v>NUM_3)<<1;
    v>>=m;
    ret|=m;
    ret+=v>1;
    return ret;
}
#endif

#if defined(VERBS_DRIVER_NEED_VERBS_LOG)
__attribute__((format(printf, 3, 4)))
void __verbs_log(struct verbs_context *ctx, uint32_t level,
		 const char *fmt, ...)
{
	va_list args;

	int tmp = errno;
	va_start(args, fmt);
	(void)vfprintf(stderr, fmt, args);
	va_end(args);
	errno = tmp;
}
#endif

#if !defined(BITMAP_HAVE_BITMAP_FIND_FIRST_BIT)
static unsigned long __word_ffs(const unsigned long *word)
{
	unsigned long i;

	for (i = 0; i < BITS_PER_LONG; i++) {
		if (bitmap_test_bit((bitmap *)word, i))
			return i;
	}

	return i;
}

static unsigned long word_ffs(const unsigned long *word, unsigned long bmp_index, unsigned long end)
{
	unsigned long setbit;

	setbit = __word_ffs(word);
	setbit += bmp_index * BITS_PER_LONG;
	if (setbit >= end)
		return end;

	return setbit;
}

unsigned long bitmap_find_first_bit(const bitmap *bmp, unsigned long start, unsigned long end)
{
	unsigned long first_word;
	unsigned long mask;
	unsigned long end_idx = BMP_WORD_INDEX(end);
	unsigned long curr_idx = BMP_WORD_INDEX(start);

	ASSERT(start <= end);

	mask = BMP_FIRST_WORD_MASK(start);

	first_word = bmp[curr_idx].w & mask;
	if (first_word)
		return word_ffs(&first_word, curr_idx, end);

	for (curr_idx++; curr_idx <= end_idx; curr_idx++) {
		if (!bmp[curr_idx].w)
			continue;

		return word_ffs(&bmp[curr_idx].w, curr_idx, end);
	}

	return end;
}
#endif

#if defined(UTIL_NEED_GET_RANDOM)
#ifndef SYS_getrandom
#  if defined(__x86_64__)
#    define SYS_getrandom 318
#  elif defined(__i386__)
#    define SYS_getrandom 355
#  elif defined(__aarch64__)
#    define SYS_getrandom 278
#  else
#    error "Please add SYS_getrandom number for your arch"
#  endif
#endif

#define GRND_NONBLOCK  0x0001
#define GRND_INSECURE  0x0004

static inline ssize_t getrandom(void *buf, size_t buflen, unsigned int flags)
{
    /* 1. 优先用系统调用（CentOS 7 内核 3.10 实际支持 318 号） */
    long ret = syscall(SYS_getrandom, buf, buflen, flags);
    if (ret >= 0)
        return ret;

    /* 2. 系统调用被裁掉（容器 seccomp 等）再读 /dev/urandom */
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;

    size_t done = 0;
    while (done < buflen) {
        ret = read(fd, (char *)buf + done, buflen - done);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            close(fd);
            return -1;
        }
        done += ret;
    }
    close(fd);
    return done;
}

unsigned int get_random(void)
{
	static unsigned int seed;
	ssize_t sz;

	if (!seed) {
		sz = getrandom(&seed, sizeof(seed),
			       GRND_NONBLOCK | GRND_INSECURE);
		if (sz < 0)
			sz = getrandom(&seed, sizeof(seed), GRND_NONBLOCK);

		if (sz != sizeof(seed))
			seed = time(NULL);
	}

	return rand_r(&seed);
}
#endif

#ifdef ALLOC_CONTEXT_HAVE_PRIVATE_DATA
struct verbs_context *compat_hihtr_alloc_context(struct ibv_device *ibdev,
						    int cmd_fd,
						    void *private_data)
{
    return hihtr_alloc_context(ibdev, cmd_fd, private_data);
}
#else
struct verbs_context *compat_hihtr_alloc_context(struct ibv_device *ibdev,
						    int cmd_fd)
{
    return hihtr_alloc_context(ibdev, cmd_fd, NULL);
}
#endif

int compat_ibv_cmd_modify_qp_ex(struct ibv_qp *qp,
                                struct ibv_qp_attr *attr,
                                int attr_mask,
                                struct ibv_modify_qp_ex *cmd,
								size_t cmd_core_size, size_t cmd_size,
                                struct ib_uverbs_ex_modify_qp_resp *resp,
								size_t resp_core_size, size_t resp_size)
{
#if !defined(IBV_CMD_MODIFY_QP_EX_HAVE_CMD_CORE_SZ)
    return ibv_cmd_modify_qp_ex(qp, attr, attr_mask, cmd,
				   cmd_size, resp, resp_size);
#else
    return ibv_cmd_modify_qp_ex(qp, attr, attr_mask, cmd,
				   cmd_core_size, cmd_size, resp, resp_core_size, resp_size);
#endif
}

#if !defined(HAVE_IBV_CMD_QUERY_DEV_ANY)
int compat_ibv_cmd_query_device_ex(struct ibv_context *context,
                            const struct ibv_query_device_ex_input *query_input,
                            struct ibv_device_attr_ex *device_attr, size_t attr_size,
                            uint64_t *raw_fw_version,
                            struct ibv_query_device_ex *query_cmd,
                            size_t cmd_size,
                            struct ib_uverbs_ex_query_device_resp *query_resp,
                            size_t resp_size)
{
#if defined(IBV_CMD_QUERY_DEVICE_EX_HAVE_CMD_CORE_SZ)
	return ibv_cmd_query_device_ex(context, query_input, device_attr, attr_size,
                            raw_fw_version, query_cmd, cmd_size, cmd_size,
                            query_resp, resp_size, resp_size);
#else
	return ibv_cmd_query_device_ex(context, query_input, device_attr, attr_size,
                            raw_fw_version, query_cmd, cmd_size,
                            query_resp, resp_size);
#endif
}
#endif

int compat_ibv_cmd_create_qp_ex2(struct ibv_context *context,
                          struct verbs_qp *qp, int vqp_size,
                          struct ibv_qp_init_attr_ex *qp_init_attr,
                          struct ibv_create_qp_ex *create_cmd,
                          size_t cmd_size, size_t cmd_core_size,
                          struct ib_uverbs_ex_create_qp_resp *create_resp,
                          size_t resp_size, size_t resp_core_size)
{
#if defined(IBV_CMD_CREATE_QP_EX2_HAVE_CMD_CORE_SZ)
	return ibv_cmd_create_qp_ex2(context, qp, vqp_size, qp_init_attr,
				    create_cmd, cmd_core_size, cmd_size,
				    create_resp, resp_core_size, resp_size);
#elif defined(CREATE_QP_EX2_HAVE_VQP_SZ)
	return ibv_cmd_create_qp_ex2(context, qp, vqp_size, qp_init_attr,
				    create_cmd, cmd_size,
				    create_resp, resp_size);
#else
	return ibv_cmd_create_qp_ex2(context, qp, qp_init_attr,
				    create_cmd, cmd_size,
				    create_resp, resp_size);
#endif
}

