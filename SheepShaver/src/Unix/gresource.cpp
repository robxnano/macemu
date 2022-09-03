#include "gio/gio.h"

#if defined (__ELF__) && ( __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 6))
# define SECTION __attribute__ ((section (".gresource.gresource"), aligned (8)))
#else
# define SECTION
#endif

static const SECTION union { const guint8 data[601]; const double alignment; void * const ptr;}  gresource_resource_data = {
  "\107\126\141\162\151\141\156\164\000\000\000\000\000\000\000\000"
  "\030\000\000\000\310\000\000\000\000\000\000\050\006\000\000\000"
  "\000\000\000\000\001\000\000\000\002\000\000\000\004\000\000\000"
  "\005\000\000\000\006\000\000\000\032\065\342\273\004\000\000\000"
  "\310\000\000\000\005\000\114\000\320\000\000\000\324\000\000\000"
  "\173\234\220\243\000\000\000\000\324\000\000\000\007\000\166\000"
  "\340\000\000\000\054\002\000\000\144\212\041\067\005\000\000\000"
  "\054\002\000\000\006\000\114\000\064\002\000\000\070\002\000\000"
  "\324\265\002\000\377\377\377\377\070\002\000\000\001\000\114\000"
  "\074\002\000\000\100\002\000\000\061\021\132\022\002\000\000\000"
  "\100\002\000\000\014\000\114\000\114\002\000\000\120\002\000\000"
  "\112\216\217\013\003\000\000\000\120\002\000\000\004\000\114\000"
  "\124\002\000\000\130\002\000\000\144\141\164\141\057\000\000\000"
  "\001\000\000\000\155\145\156\165\056\165\151\000\000\000\000\000"
  "\133\004\000\000\001\000\000\000\170\332\245\124\261\116\303\060"
  "\020\335\373\025\326\015\135\120\022\052\030\220\352\270\352\122"
  "\261\040\121\002\013\013\272\244\107\153\341\070\301\276\120\340"
  "\353\161\240\152\125\025\242\066\114\266\316\172\357\336\335\173"
  "\262\234\274\227\106\274\221\363\272\262\051\214\342\163\020\144"
  "\213\152\241\355\062\205\207\373\131\164\005\023\065\220\332\062"
  "\271\147\054\110\111\107\257\215\166\344\205\321\171\012\113\176"
  "\071\203\035\301\105\074\272\204\104\311\222\154\043\364\042\205"
  "\022\265\215\326\332\056\252\165\324\026\101\111\337\344\355\115"
  "\111\144\166\072\157\230\204\305\222\122\060\230\223\001\301\016"
  "\255\067\310\230\233\120\374\040\017\352\151\246\015\311\144\013"
  "\010\044\124\160\350\250\244\146\052\017\251\360\373\025\124\350"
  "\034\173\106\307\173\340\023\032\147\055\130\144\053\242\072\133"
  "\141\230\263\223\010\213\042\020\251\241\341\161\121\131\166\225"
  "\031\056\171\234\355\141\222\037\305\107\350\016\355\042\117\314"
  "\301\013\337\133\177\040\021\331\157\044\107\353\370\304\072\252"
  "\035\226\175\045\074\142\055\156\357\246\067\342\300\304\215\204"
  "\144\153\346\011\256\206\020\366\066\165\036\260\377\364\164\336"
  "\075\110\262\015\171\237\264\137\223\251\373\246\035\363\252\351"
  "\275\230\151\013\376\163\063\035\123\046\233\143\367\117\014\276"
  "\000\340\346\223\175\000\050\165\165\141\171\051\143\145\142\151"
  "\170\057\000\000\004\000\000\000\057\000\000\000\005\000\000\000"
  "\123\150\145\145\160\123\150\141\166\145\162\057\000\000\000\000"
  "\156\145\164\057\002\000\000\000" };

static GStaticResource static_resource = { gresource_resource_data.data, sizeof (gresource_resource_data.data) - 1 /* nul terminator */, NULL, NULL, NULL };

G_MODULE_EXPORT
GResource *gresource_get_resource (void);
GResource *gresource_get_resource (void)
{
  return g_static_resource_get_resource (&static_resource);
}
/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __G_CONSTRUCTOR_H__
#define __G_CONSTRUCTOR_H__

/*
  If G_HAS_CONSTRUCTORS is true then the compiler support *both* constructors and
  destructors, in a usable way, including e.g. on library unload. If not you're on
  your own.

  Some compilers need #pragma to handle this, which does not work with macros,
  so the way you need to use this is (for constructors):

  #ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
  #pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(my_constructor)
  #endif
  G_DEFINE_CONSTRUCTOR(my_constructor)
  static void my_constructor(void) {
   ...
  }

*/

#ifndef __GTK_DOC_IGNORE__

#if  __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR(_func) static void __attribute__((constructor)) _func (void);
#define G_DEFINE_DESTRUCTOR(_func) static void __attribute__((destructor)) _func (void);

#elif defined (_MSC_VER) && (_MSC_VER >= 1500)
/* Visual studio 2008 and later has _Pragma */

/*
 * Only try to include gslist.h if not already included via glib.h,
 * so that items using gconstructor.h outside of GLib (such as
 * GResources) continue to build properly.
 */
#ifndef __G_LIB_H__
#include "gslist.h"
#endif

#include <stdlib.h>

#define G_HAS_CONSTRUCTORS 1

/* We do some weird things to avoid the constructors being optimized
 * away on VS2015 if WholeProgramOptimization is enabled. First we
 * make a reference to the array from the wrapper to make sure its
 * references. Then we use a pragma to make sure the wrapper function
 * symbol is always included at the link stage. Also, the symbols
 * need to be extern (but not dllexport), even though they are not
 * really used from another object file.
 */

/* We need to account for differences between the mangling of symbols
 * for x86 and x64/ARM/ARM64 programs, as symbols on x86 are prefixed
 * with an underscore but symbols on x64/ARM/ARM64 are not.
 */
#ifdef _M_IX86
#define G_MSVC_SYMBOL_PREFIX "_"
#else
#define G_MSVC_SYMBOL_PREFIX ""
#endif

#define G_DEFINE_CONSTRUCTOR(_func) G_MSVC_CTOR (_func, G_MSVC_SYMBOL_PREFIX)
#define G_DEFINE_DESTRUCTOR(_func) G_MSVC_DTOR (_func, G_MSVC_SYMBOL_PREFIX)

#define G_MSVC_CTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _wrapper(void) { _func(); g_slist_find (NULL,  _array ## _func); return 0; } \
  __pragma(comment(linker,"/include:" _sym_prefix # _func "_wrapper")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _wrapper;

#define G_MSVC_DTOR(_func,_sym_prefix) \
  static void _func(void); \
  extern int (* _array ## _func)(void);              \
  int _func ## _constructor(void) { atexit (_func); g_slist_find (NULL,  _array ## _func); return 0; } \
   __pragma(comment(linker,"/include:" _sym_prefix # _func "_constructor")) \
  __pragma(section(".CRT$XCU",read)) \
  __declspec(allocate(".CRT$XCU")) int (* _array ## _func)(void) = _func ## _constructor;

#elif defined (_MSC_VER)

#define G_HAS_CONSTRUCTORS 1

/* Pre Visual studio 2008 must use #pragma section */
#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _wrapper(void) { _func(); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (*p)(void) = _func ## _wrapper;

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  section(".CRT$XCU",read)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void); \
  static int _func ## _constructor(void) { atexit (_func); return 0; } \
  __declspec(allocate(".CRT$XCU")) static int (* _array ## _func)(void) = _func ## _constructor;

#elif defined(__SUNPRO_C)

/* This is not tested, but i believe it should work, based on:
 * http://opensource.apple.com/source/OpenSSL098/OpenSSL098-35/src/fips/fips_premain.c
 */

#define G_HAS_CONSTRUCTORS 1

#define G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA 1
#define G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA 1

#define G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(_func) \
  init(_func)
#define G_DEFINE_CONSTRUCTOR(_func) \
  static void _func(void);

#define G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(_func) \
  fini(_func)
#define G_DEFINE_DESTRUCTOR(_func) \
  static void _func(void);

#else

/* constructors not supported for this compiler */

#endif

#endif /* __GTK_DOC_IGNORE__ */
#endif /* __G_CONSTRUCTOR_H__ */

#ifdef G_HAS_CONSTRUCTORS

#ifdef G_DEFINE_CONSTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_CONSTRUCTOR_PRAGMA_ARGS(gresourceresource_constructor)
#endif
G_DEFINE_CONSTRUCTOR(gresourceresource_constructor)
#ifdef G_DEFINE_DESTRUCTOR_NEEDS_PRAGMA
#pragma G_DEFINE_DESTRUCTOR_PRAGMA_ARGS(gresourceresource_destructor)
#endif
G_DEFINE_DESTRUCTOR(gresourceresource_destructor)

#else
#warning "Constructor not supported on this compiler, linking in resources will not work"
#endif

static void gresourceresource_constructor (void)
{
  g_static_resource_init (&static_resource);
}

static void gresourceresource_destructor (void)
{
  g_static_resource_fini (&static_resource);
}
