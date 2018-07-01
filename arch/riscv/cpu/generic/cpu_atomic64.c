/**
 * Copyright (c) 2018 Anup Patel.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file cpu_atomic64.c
 * @author Anup Patel (anup@brainfault.org)
 * @brief RISC-V specific 64 bits synchronization mechanisms.
 *
 */

#include <vmm_error.h>
#include <vmm_types.h>
#include <vmm_compiler.h>
#include <arch_cpu_irq.h>
#include <arch_barrier.h>
#include <arch_atomic64.h>

u64 __lock arch_atomic64_read(atomic64_t *atom)
{
	u64 ret = (*(volatile long *)&atom->counter);
	arch_rmb();
	return ret;
}

void __lock arch_atomic64_write(atomic64_t *atom, u64 value)
{
	atom->counter = value;
	arch_wmb();
}

void __lock arch_atomic64_add(atomic64_t *atom, u64 value)
{
	__asm__ __volatile__ (
		"	amoadd.d zero, %1, %0"
		: "+A" (atom->counter)
		: "r" (value)
		: "memory");
}

void __lock arch_atomic64_sub(atomic64_t *atom, u64 value)
{
	__asm__ __volatile__ (
		"	amoadd.d zero, %1, %0"
		: "+A" (atom->counter)
		: "r" (-value)
		: "memory");
}

u64 __lock arch_atomic64_add_return(atomic64_t *atom, u64 value)
{
	u64 ret;

	__asm__ __volatile__ (
		"	amoadd.d.aqrl  %1, %2, %0"
		: "+A" (atom->counter), "=r" (ret)
		: "r" (value)
		: "memory");

	return ret;
}

u64 __lock arch_atomic64_sub_return(atomic64_t *atom, u64 value)
{
	u64 ret;

	__asm__ __volatile__ (
		"	amoadd.d.aqrl  %1, %2, %0"
		: "+A" (atom->counter), "=r" (ret)
		: "r" (-value)
		: "memory");

	return ret;
}

#define __cmpxchg(ptr, old, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	__typeof__(*(ptr)) __ret;					\
	register unsigned int __rc;					\
	switch (size) {							\
	case 4:								\
		__asm__ __volatile__ (					\
			"0:	lr.w %0, %2\n"				\
			"	bne  %0, %z3, 1f\n"			\
			"	sc.w.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	default:							\
		break;							\
	}								\
	__ret;								\
})

#define cmpxchg(ptr, o, n)						\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg((ptr),				\
				       _o_, _n_, sizeof(*(ptr)));	\
})

u64 __lock arch_atomic64_cmpxchg(atomic64_t *atom, u64 oldval, u64 newval)
{
	return cmpxchg(&atom->counter, oldval, newval);
}
