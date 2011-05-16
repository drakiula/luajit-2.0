/*
** Definitions for target CPU.
** Copyright (C) 2005-2011 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_TARGET_H
#define _LJ_TARGET_H

#include "lj_def.h"
#include "lj_arch.h"

/* -- Registers and spill slots ------------------------------------------- */

/* Register type (uint8_t in ir->r). */
typedef uint32_t Reg;

/* The hi-bit is NOT set for an allocated register. This means the value
** can be directly used without masking. The hi-bit is set for a register
** allocation hint or for RID_INIT.
*/
#define RID_NONE		0x80
#define RID_MASK		0x7f
#define RID_INIT		(RID_NONE|RID_MASK)

#define ra_noreg(r)		((r) & RID_NONE)
#define ra_hasreg(r)		(!((r) & RID_NONE))

/* The ra_hashint() macro assumes a previous test for ra_noreg(). */
#define ra_hashint(r)		((r) != RID_INIT)
#define ra_gethint(r)		((Reg)((r) & RID_MASK))
#define ra_sethint(rr, r)	rr = (uint8_t)((r)|RID_NONE)
#define ra_samehint(r1, r2)	(ra_gethint((r1)^(r2)) == 0)

/* Spill slot 0 means no spill slot has been allocated. */
#define SPS_NONE		0

#define ra_hasspill(s)		((s) != SPS_NONE)

/* Combined register and spill slot (uint16_t in ir->prev). */
typedef uint32_t RegSP;

#define REGSP(r, s)		((r) + ((s) << 8))
#define REGSP_HINT(r)		((r)|RID_NONE)
#define REGSP_INIT		REGSP(RID_INIT, 0)

#define regsp_reg(rs)		((rs) & 255)
#define regsp_spill(rs)		((rs) >> 8)
#define regsp_used(rs) \
  (((rs) & ~REGSP(RID_MASK, 0)) != REGSP(RID_NONE, 0))

/* -- Register sets ------------------------------------------------------- */

/* Bitset for registers. 32 registers suffice right now.
** Note that one set holds bits for both GPRs and FPRs.
*/
typedef uint32_t RegSet;

#define RID2RSET(r)		(((RegSet)1) << (r))
#define RSET_EMPTY		0
#define RSET_RANGE(lo, hi)	((RID2RSET((hi)-(lo))-1) << (lo))

#define rset_test(rs, r)	(((rs) >> (r)) & 1)
#define rset_set(rs, r)		(rs |= RID2RSET(r))
#define rset_clear(rs, r)	(rs &= ~RID2RSET(r))
#define rset_exclude(rs, r)	(rs & ~RID2RSET(r))
#define rset_picktop(rs)	((Reg)lj_fls(rs))
#define rset_pickbot(rs)	((Reg)lj_ffs(rs))

/* -- Register allocation cost -------------------------------------------- */

/* The register allocation heuristic keeps track of the cost for allocating
** a specific register:
**
** A free register (obviously) has a cost of 0 and a 1-bit in the free mask.
**
** An already allocated register has the (non-zero) IR reference in the lowest
** bits and the result of a blended cost-model in the higher bits.
**
** The allocator first checks the free mask for a hit. Otherwise an (unrolled)
** linear search for the minimum cost is used. The search doesn't need to
** keep track of the position of the minimum, which makes it very fast.
** The lowest bits of the minimum cost show the desired IR reference whose
** register is the one to evict.
**
** Without the cost-model this degenerates to the standard heuristics for
** (reverse) linear-scan register allocation. Since code generation is done
** in reverse, a live interval extends from the last use to the first def.
** For an SSA IR the IR reference is the first (and only) def and thus
** trivially marks the end of the interval. The LSRA heuristics says to pick
** the register whose live interval has the furthest extent, i.e. the lowest
** IR reference in our case.
**
** A cost-model should take into account other factors, like spill-cost and
** restore- or rematerialization-cost, which depend on the kind of instruction.
** E.g. constants have zero spill costs, variant instructions have higher
** costs than invariants and PHIs should preferably never be spilled.
**
** Here's a first cut at simple, but effective blended cost-model for R-LSRA:
** - Due to careful design of the IR, constants already have lower IR
**   references than invariants and invariants have lower IR references
**   than variants.
** - The cost in the upper 16 bits is the sum of the IR reference and a
**   weighted score. The score currently only takes into account whether
**   the IRT_ISPHI bit is set in the instruction type.
** - The PHI weight is the minimum distance (in IR instructions) a PHI
**   reference has to be further apart from a non-PHI reference to be spilled.
** - It should be a power of two (for speed) and must be between 2 and 32768.
**   Good values for the PHI weight seem to be between 40 and 150.
** - Further study is required.
*/
#define REGCOST_PHI_WEIGHT	64

/* Cost for allocating a specific register. */
typedef uint32_t RegCost;

/* Note: assumes 16 bit IRRef1. */
#define REGCOST(cost, ref)	((RegCost)(ref) + ((RegCost)(cost) << 16))
#define regcost_ref(rc)		((IRRef1)(rc))

#define REGCOST_T(t) \
  ((RegCost)((t)&IRT_ISPHI) * (((RegCost)(REGCOST_PHI_WEIGHT)<<16)/IRT_ISPHI))
#define REGCOST_REF_T(ref, t)	(REGCOST((ref), (ref)) + REGCOST_T((t)))

/* -- Target-specific definitions ----------------------------------------- */

#if LJ_TARGET_X86ORX64
#include "lj_target_x86.h"
#else
#error "Missing include for target CPU"
#endif

/* Return the address of an exit stub. */
static LJ_AINLINE MCode *exitstub_addr(jit_State *J, ExitNo exitno)
{
  lua_assert(J->exitstubgroup[exitno / EXITSTUBS_PER_GROUP] != NULL);
  return (MCode *)((char *)J->exitstubgroup[exitno / EXITSTUBS_PER_GROUP] +
		   EXITSTUB_SPACING*(exitno % EXITSTUBS_PER_GROUP));
}

#endif
