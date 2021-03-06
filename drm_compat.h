/**
 * \file drm_compat.h
 * Backward compatability definitions for Direct Rendering Manager
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DRM_COMPAT_H_
#define _DRM_COMPAT_H_

#include "common_compat.h"

#include <linux/version.h>
#include <linux/rculist.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kgdb.h>
#include <linux/idr.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/printk.h>
#include <linux/sysfs.h>

#include <asm/pgalloc.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <linux/sched/signal.h>
#endif

/* Event tracing is not used in vmwgfx */
#define trace_drm_vblank_event_delivered(pid, pipe, seq)
#define trace_drm_vblank_event_queued(pid, pipe, seq)
#define trace_drm_vblank_event(pipe, seq)

/**
 * VM_RESERVED disappeared in 3.7, and is replaced in upstream
 * ttm_bo_vm.c with VM_DONTDUMP. Try to keep code in sync with
 * upstream
 */
#ifndef VM_DONTDUMP
#define VM_DONTDUMP VM_RESERVED
#endif

/*
 * kuid stuff, introduced in 3.5.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0))
#define kuid_t uid_t
#define from_kuid_munged(_a, _uid) (_uid)
#endif

/* set_mb__[before|after]_atomic appeared in 3.16 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0) && \
     RHEL_VERSION_CODE < RHEL_RELEASE_VERSION(7, 3))
#define smp_mb__before_atomic() smp_mb__before_atomic_inc()
#define smp_mb__after_atomic() smp_mb__after_atomic_inc()
#endif

/*
 * READ_ONCE, WRITE_ONCE appeared in 3.19.
 * This is a simplified version for scalar use only.
 */
#ifndef READ_ONCE
#define READ_ONCE(_x) (*(volatile typeof(_x) *)&(_x))
#endif
#ifndef WRITE_ONCE
#define WRITE_ONCE(_x, _val) READ_ONCE(_x) = _val
#endif

/* cpu_has_clflush disappeared in 4.7 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
#define cpu_has_clflush static_cpu_has(X86_FEATURE_CLFLUSH)
#endif

/* Replicate the functionality of ihold, except a refcount warning */
#define ihold(_inode) atomic_inc(&((_inode)->i_count))

/* RCU_INIT_POINTER is an optimized version of rcu_assign_pointer */
#ifndef RCU_INIT_POINTER
#define RCU_INIT_POINTER(_a, _b) rcu_assign_pointer(_a, _b)
#endif

/*
 * seqcount_init, contrary to __seqcount init creates a lock class of its
 * own. This should be safe, but may not be identical on lock debugging builds
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#define __seqcount_init(_a, _b, _c) seqcount_init(_a)
#endif

/* wake_up_state is not exported, but wake_up_process is. */
#define wake_up_state(_p, _s)			\
	({BUILD_BUG_ON(_s != TASK_NORMAL);	\
	  wake_up_process(_p); })

#ifndef in_dbg_master
#ifdef CONFIG_KGDB
#define in_dbg_master() \
	(raw_smp_processor_id() == atomic_read(&kgdb_active))
#else
#define in_dbg_master() (0)
#endif
#endif

#ifndef replace_fops
#define replace_fops(f, fops) \
        do {				    \
		struct file *__file = (f);	   \
		fops_put(__file->f_op);		   \
		BUG_ON(!(__file->f_op = (fops)));  \
	} while(0)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0))
#define idr_preload(__gfp)					\
	{							\
	struct idr *__idr = NULL;				\
	int __ret;						\
	do {							\
	__ret = 0;						\
	if (__idr && idr_pre_get(__idr, __gfp) == 0) {		\
		__ret = -ENOMEM;				\
	}

#define idr_preload_end()			\
	} while(__ret == -EAGAIN);		\
	}

#define idr_alloc(__idp, __ptr, __start, __end, __agfp)			\
	({								\
		int __id;						\
		__idr = __idp;						\
		if (!__ret)						\
			__ret = idr_get_new_above(__idp, __ptr, __start, &__id); \
		if (__ret == 0 && __end != 0 && __id >= __end) {	\
			__ret = -ENOSPC; idr_remove(__idp, __id);	\
		}							\
		(__ret < 0) ? __ret : __id;				\
	})

#define VMWGFX_STANDALONE_IDR_PRELOAD(__gfp) idr_preload(__gfp)
#define VMWGFX_STANDALONE_IDR_PRELOAD_END() idr_preload_end()
#else
#define VMWGFX_STANDALONE_IDR_PRELOAD(__gfp)
#define VMWGFX_STANDALONE_IDR_PRELOAD_END()
#endif

#ifndef DIR_ROUND_CLOSEST_ULL
#define DIV_ROUND_CLOSEST_ULL(x, divisor)(				\
		{							\
			typeof(divisor) __d = divisor;			\
			unsigned long long _tmp = (x) + (__d) / 2;      \
			do_div(_tmp, __d);                              \
			_tmp;                                           \
		}                                                       \
		)
#endif

#ifndef __ATTR_RW
#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),	\
				_name##_show, _name##_store)
#endif

#define DEVICE_ATTR_RW(_name)						\
	struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
#define DEVICE_ATTR_RO(_name)						\
	struct device_attribute dev_attr_##_name = __ATTR_RO(_name)

#ifndef kobj_to_dev
#define kobj_to_dev kobj_to_dev_drm
static inline struct device *kobj_to_dev(struct kobject *kobj)
{
	return container_of(kobj, struct device, kobj);
}
#endif

#ifndef hlist_next_rcu
#define hlist_next_rcu(node)    (*((struct hlist_node __rcu **)(&(node)->next)))
#endif

#ifndef hlist_add_behind_rcu
#define hlist_add_behind_rcu hlist_add_behind_rcu_drm
static inline void hlist_add_behind_rcu(struct hlist_node *n,
                                         struct hlist_node *prev)
{
         n->next = prev->next;
         n->pprev = &prev->next;
         rcu_assign_pointer(hlist_next_rcu(prev), n);
         if (n->next)
                 n->next->pprev = &n->next;
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 1, 0)) &&	\
	(RHEL_VERSION_CODE < RHEL_RELEASE_VERSION(6, 5))
int ida_simple_get(struct ida *ida, unsigned int start,
		   unsigned int end, gfp_t gfp_mask);
void ida_simple_remove(struct ida *ida, unsigned int id);
#define VMW_IDA_SIMPLE
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0))
extern void *memdup_user_nul(const void __user *, size_t);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 7, 0))
#define u64_to_user_ptr(x) (		\
{					\
	typecheck(u64, x);		\
	(void __user *)(uintptr_t)x;	\
}					\
)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0))
#define lock_acquire_shared_recursive(l, s, t, n, i)	lock_acquire(l, s, t, 2, 1, n, i)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 5, 0))
#define GLOBAL_ROOT_UID ((kuid_t) 0)
#endif


/* Timer interface changed in 4.14 */
#ifndef from_timer
#define from_timer(var, callback_timer, timer_fieldname) \
	container_of(callback_timer, typeof(*var), timer_fieldname)
#define timer_setup(timer, callback, flags)			\
	setup_timer((timer), ((void (*)(unsigned long)) callback), ((unsigned long) timer))
#endif

/*
 * if get_unused_fd_flags() is defined as a macro, it typically evaluates
 * to alloc_fd(), which is not exported. So redefine the macro to call a
 * compat function that emulates get_unused_fd_flags() for O_CLOEXEC
 * only, and warn on other flags.
 */
#ifdef get_unused_fd_flags
#undef get_unused_fd_flags
#define VMW_COMPAT_FD_FLAGS
int vmw_get_unused_fd_flags(unsigned int flags);
#define get_unused_fd_flags(__arg) vmw_get_unused_fd_flags(__arg)
#else
#undef VMW_COMPAT_FD_FLAGS
#endif

/*
 * If dev_printk is defined as a macro, that macro requires @level to be
 * given as an immediate string constant rather as a const char pointer.
 * So redefine the macro to fit the drm_dev_printk usage.
 */
#ifdef dev_printk
#undef dev_printk
#define dev_printk(level, dev, format, arg...)			\
  printk("%s%s %s: " format, level, dev_driver_string(dev),	\
	 dev_name(dev), ## arg)
#endif

/*
 * struct va_format was added together with the linux/printk.h header in
 * 2.6.36. Check whether the header is present; if not, define
 * struct va_format.
 */
#ifndef __KERNEL_PRINTK__
struct va_format {
	const char *fmt;
	va_list *va;
};
#endif

/*
 * The list_first_entry_or_null() macro  was added in 3.11.
 */
#ifndef list_first_entry_or_null
#define list_first_entry_or_null(ptr, type, member) ({ \
	struct list_head *head__ = (ptr); \
	struct list_head *pos__ = READ_ONCE(head__->next); \
	pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})
#endif

#endif
