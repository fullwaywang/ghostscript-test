/* Copyright (C) 1996, 1997, 1998 Aladdin Enterprises.  All rights
   reserved.  Unauthorized use, copying, and/or distribution
   prohibited.  */

/* pcfrgrnd.h - PCL foreground object */

#ifndef pcfrgrnd_INCLUDED
#  define pcfrgrnd_INCLUDED

#include "gx.h"
#include "gsstruct.h"
#include "gsrefct.h"
#include "pcstate.h"
#include "pcommand.h"
#include "pccsbase.h"
#include "pcht.h"
#include "pccrd.h"
#include "pcpalet.h"

/*
 * Foreground data structure for PCL 5c; see Chapter 3 of the "PCL 5 Color
 * Technical Reference Manual". This structure is part of the PCL state.
 *
 * The handling of foreground color in PCL 5c is somewhat unintuitive, as
 * changing parameters that affect the foreground (rendering method, color
 * palette, etc.) does not affect the foreground until the latter is
 * explicitly modified. Clearly, this definition reflected HP's particular
 * implementation: a set of textures (one per component) is generated for
 * the current color index, current color palette, and current rendering
 * method when the foreground color command is issued, and this rendered
 * representation is stored in the PCL state.
 *
 * In a system such a Ghostscript graphic library, where rendered
 * representations are (properly) not visible to the client, a fair amount
 * of information must be kept to achieve the desired behavior. In essence,
 * the foreground color maintains the foreground color, base color space,
 * halfone/transfer function, and CRD with which it was created.
 *
 * When necessary, the foreground color also builds one additional color
 * space, to work around a limitation in the graphics library. All PCL's
 * built-in patterns (including shades) and format 0 user defined patterns
 * are, in the PostScript sense, uncolored: they adopt the foreground color
 * current when they are rendered. Unfortunately, these patterns also have
 * the transparency property of colored patterns: pixels outside of the mask
 * can still be opaque. The graphics library does not currently support
 * opaque rendering of uncolored patterns, so all PCL patterns are rendered
 * as colored. The foreground will build an indexed color space with two
 * entries for this purpose.
 *
 * The foreground structure is reference-counted. It is also assigned an
 * identifier, so that we can track when it is necessary to re-render
 * uncolored patterns.
 */
struct pcl_frgrnd_s {
    rc_header       rc;
    pcl_gsid_t      id;
    byte            color[3];
    pcl_cs_base_t * pbase;
    pcl_ht_t *      pht;
    pcl_crd_t *     pcrd;
};

#ifndef pcl_frgrnd_DEFINED
#define pcl_frgrnd_DEFINED
typedef struct pcl_frgrnd_s     pcl_frgrnd_t;
#endif

#define private_st_frgrnd_t()                       \
    gs_private_st_ptrs3( st_frgrnd_t,               \
                         pcl_frgrnd_t,              \
                         "pcl foreground object",   \
                         frgrnd_enum_ptrs,          \
                         frgrnd_reloc_ptrs,         \
                         pbase,                     \
                         pht,                       \
                         pcrd                       \
                         )

/*
 * The usual init, copy,and release macros.
 */
#define pcl_frgrnd_init_from(pto, pfrom)    \
    BEGIN                                   \
    rc_increment(pfrom);                    \
    (pto) = (pfrom);                        \
    END

#define pcl_frgrnd_copy_from(pto, pfrom)            \
    BEGIN                                           \
    if ((pto) != (pfrom)) {                         \
        rc_increment(pfrom);                        \
        rc_decrement(pto, "pcl_frgrnd_copy_from");  \
        (pto) = (pfrom);                            \
    }                                               \
    END

#define pcl_frgrnd_release(pbase)           \
    rc_decrement(pbase, "pcl_frgrnd_release")

/*
 * Get the base color space type from a foreground object
 */
#define pcl_frgrnd_get_cspace(pfrgrnd)  ((pfrgrnd)->pbase->type)

/*
 * Build the default foreground. This should be called by the reset function
 * for palettes, and should only be called when the current palette is the
 * default 2-entry palette.
 */
int pcl_frgrnd_set_default_foreground(P1(pcl_state_t * pcs));

#endif  /* pcfrgrnd_INCLUDED */
