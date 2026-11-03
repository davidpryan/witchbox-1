/* SPDX-License-Identifier: GPL-3.0-only */
/* What every toy file includes: the knob helper and the class macro. A toy file defines its state
 * struct, its callbacks, and ends with TOY(...). toys/all.c includes them all and lists them. */
#pragma once
#include "../wb.h"

static inline param_t *add(toy_t *t, const char *name, int lo, int hi, int value, const char *help)
{
    param_t *p = &t->p[t->np++];
    param_init(p, name, lo, hi, value, 1, 0, "%d", help);
    return p;
}

static inline param_t *add_pick(toy_t *t, const char *name, int n, int value, const char *const *names,
                                const char *help)
{
    param_t *p = &t->p[t->np++];
    param_init(p, name, 0, n - 1, value, 1, names, "%d", help);
    return p;
}

/* Python's floor division and modulo for ints that may be negative */
static inline int floordiv(int a, int b) { int q = a / b; return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q; }
static inline int fmod_(int a, int b) { int m = a % b; return (m != 0 && ((m < 0) != (b < 0))) ? m + b : m; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iabs(int a) { return a < 0 ? -a : a; }

#define NONE (-1)

#define TOY(sym, mod, nm, gl, rl, hu, fms, pg, wd, st, mk, ln, ch, rn, os, pc, fr, dr, cl)       \
    const toy_class_t sym = {mod, nm, gl, wd, rl, hu, fms, pg, mk, ln, ch, rn, os, pc, fr, dr, cl, 0, \
                             (uint16_t)sizeof(st)}
#define TOY2(sym, mod, nm, gl, rl, hu, fms, pg, wd, st, mk, ln, ch, rn, os, pc, fr, dr, cl, op)   \
    const toy_class_t sym = {mod, nm, gl, wd, rl, hu, fms, pg, mk, ln, ch, rn, os, pc, fr, dr, cl, op, \
                             (uint16_t)sizeof(st)}

enum { HUE_PURPLE, HUE_BLUE, HUE_CYAN, HUE_GO, HUE_YELLOW, HUE_ORANGE, HUE_RED, HUE_PINK, HUE_GREY, NHUES };
