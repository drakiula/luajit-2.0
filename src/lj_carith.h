/*
** C data arithmetic.
** Copyright (C) 2005-2011 Mike Pall. See Copyright Notice in luajit.h
*/

#ifndef _LJ_CARITH_H
#define _LJ_CARITH_H

#include "lj_obj.h"

#if LJ_HASFFI

LJ_FUNC int lj_carith_op(lua_State *L, MMS mm);

LJ_FUNC uint64_t lj_carith_powi64(uint64_t x, uint64_t k, int isunsigned);

#endif

#endif
