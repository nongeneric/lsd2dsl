#include "inflate.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "globals.h"

static int readbyte(void* ptr) {
    assert(0);
}

static int flush(void* a, int b, int c, int d) {
    assert(0);
}

#ifdef FUNZIP
#define FLUSH(w) flush(__G__(ulg)(w))
#define NEXTBYTE getc(G.in) /* redefined in crypt.h if full version */
#else
#define FLUSH(w)                                                                    \
    ((G.mem_mode) ? memflush(__G__ redirSlide, (ulg)(w))                            \
                  : flush(__G__ redirSlide, (ulg)(w), 0))
#define NEXTBYTE (G.incnt-- > 0 ? (int)(*G.inptr++) : readbyte(__G))
#endif

#define PK_DISK 50 /* disk full */

static int memflush(__G__ rawbuf, size) __GDEF ZCONST uch* rawbuf;
ulg size;
{
    if (size > G.outsize)
        /* Here, PK_DISK is a bit off-topic, but in the sense of marking
           "overflow of output space", its use may be tolerated. */
        return PK_DISK; /* more data than output buffer can hold */

    memcpy((char*)G.outbufptr, (char*)rawbuf, size);
    G.outbufptr += (unsigned int)size;
    G.outsize -= size;
    G.outcnt += size;

    return 0;
}

struct huft {
    uch e; /* number of extra bits or operation */
    uch b; /* number of bits in this code or subcode */
    union {
        ush n;          /* literal, length base, or distance base */
        struct huft* t; /* pointer to next level of table */
    } v;
};

#define memzero(a, b) memset(a, 0, b)

#define INVALID_CODE 99
#define IS_INVALID_CODE(c) ((c) == INVALID_CODE)

#if (defined(DLL) && !defined(NO_SLIDE_REDIR))
#define redirSlide G.redirect_sldptr
#else
#define redirSlide G.area.Slide
#endif

#if (defined(USE_DEFLATE64) && defined(INT_16BIT))
#define UINT_D64 ulg
#else
#define UINT_D64 unsigned
#endif

#define wsize WSIZE /* wsize is a constant */

#ifndef MESSAGE /* only used twice, for fixed strings--NOT general-purpose */
#define MESSAGE(str, len, flag) fprintf(stderr, (char*)(str))
#endif

#ifndef Trace
#ifdef DEBUG
#define Trace(x) fprintf x
#else
#define Trace(x)
#endif
#endif

#ifdef PKZIP_BUG_WORKAROUND
#define MAXLITLENS 288
#else
#define MAXLITLENS 286
#endif
#if (defined(USE_DEFLATE64) || defined(PKZIP_BUG_WORKAROUND))
#define MAXDISTS 32
#else
#define MAXDISTS 30
#endif

ZCONST unsigned near mask_bits[17] = {0x0000,
                                      0x0001,
                                      0x0003,
                                      0x0007,
                                      0x000f,
                                      0x001f,
                                      0x003f,
                                      0x007f,
                                      0x00ff,
                                      0x01ff,
                                      0x03ff,
                                      0x07ff,
                                      0x0fff,
                                      0x1fff,
                                      0x3fff,
                                      0x7fff,
                                      0xffff};

ulg bb;      /* bit buffer */
unsigned bk; /* bits in bit buffer */

#ifndef CHECK_EOF
#define CHECK_EOF /* default as of 5.13/5.2 */
#endif

#ifndef CHECK_EOF
#define NEEDBITS(n)                                                                 \
    {                                                                               \
        while (k < (n)) {                                                           \
            b |= ((ulg)NEXTBYTE) << k;                                              \
            k += 8;                                                                 \
        }                                                                           \
    }
#else
#ifdef FIX_PAST_EOB_BY_TABLEADJUST
#define NEEDBITS(n)                                                                 \
    {                                                                               \
        while (k < (n)) {                                                           \
            int c = NEXTBYTE;                                                       \
            if (c == EOF) {                                                         \
                retval = 1;                                                         \
                goto cleanup_and_exit;                                              \
            }                                                                       \
            b |= ((ulg)c) << k;                                                     \
            k += 8;                                                                 \
        }                                                                           \
    }
#else
#define NEEDBITS(n)                                                                 \
    {                                                                               \
        while ((int)k < (int)(n)) {                                                 \
            int c = NEXTBYTE;                                                       \
            if (c == EOF) {                                                         \
                if ((int)k >= 0)                                                    \
                    break;                                                          \
                retval = 1;                                                         \
                goto cleanup_and_exit;                                              \
            }                                                                       \
            b |= ((ulg)c) << k;                                                     \
            k += 8;                                                                 \
        }                                                                           \
    }
#endif
#endif

#define DUMPBITS(n)                                                                 \
    {                                                                               \
        b >>= (n);                                                                  \
        k -= (n);                                                                   \
    }

/* Tables for deflate from PKZIP's appnote.txt. */
/* - Order of the bit length code lengths */
static ZCONST unsigned border[] = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* - Copy lengths for literal codes 257..285 */
#ifdef USE_DEFLATE64
static ZCONST ush cplens64[] = {3,  4,   5,   6,   7,   8,   9,  10, 11, 13, 15,
                                17, 19,  23,  27,  31,  35,  43, 51, 59, 67, 83,
                                99, 115, 131, 163, 195, 227, 3,  0,  0};
/* For Deflate64, the code 285 is defined differently. */
#else
#define cplens32 cplens
#endif
static ZCONST ush cplens32[] = {3,  4,   5,   6,   7,   8,   9,   10, 11, 13, 15,
                                17, 19,  23,  27,  31,  35,  43,  51, 59, 67, 83,
                                99, 115, 131, 163, 195, 227, 258, 0,  0};
/* note: see note #13 above about the 258 in this list. */
/* - Extra bits for literal codes 257..285 */
#ifdef USE_DEFLATE64
static ZCONST uch cplext64[] = {0,           0, 0, 0, 0, 0, 0, 0, 1,  1,
                                1,           1, 2, 2, 2, 2, 3, 3, 3,  3,
                                4,           4, 4, 4, 5, 5, 5, 5, 16, INVALID_CODE,
                                INVALID_CODE};
#else
#define cplext32 cplext
#endif
static ZCONST uch cplext32[] = {0,           0, 0, 0, 0, 0, 0, 0, 1, 1,
                                1,           1, 2, 2, 2, 2, 3, 3, 3, 3,
                                4,           4, 4, 4, 5, 5, 5, 5, 0, INVALID_CODE,
                                INVALID_CODE};

static ZCONST ush cpdist[] = {
    1,    2,     3,     4,     5,     7,    9,   13,   17,   25,   33,   49,   65,
    97,   129,   193,   257,   385,   513,  769, 1025, 1537, 2049, 3073, 4097, 6145,
#if (defined(USE_DEFLATE64) || defined(PKZIP_BUG_WORKAROUND))
    8193, 12289, 16385, 24577, 32769, 49153};
#else
    8193, 12289, 16385, 24577};
#endif

/* - Extra bits for distance codes 0..29 (0..31 for Deflate64) */
#ifdef USE_DEFLATE64
static ZCONST uch cpdext64[] = {0,  0,  0,  0,  1,  1,  2,  2,  3,  3, 4,
                                4,  5,  5,  6,  6,  7,  7,  8,  8,  9, 9,
                                10, 10, 11, 11, 12, 12, 13, 13, 14, 14};
#else
#define cpdext32 cpdext
#endif
static ZCONST uch cpdext32[] = {0,           0,  0,  0,  1,
                                1,           2,  2,  3,  3,
                                4,           4,  5,  5,  6,
                                6,           7,  7,  8,  8,
                                9,           9,  10, 10, 11,
                                11,
#ifdef PKZIP_BUG_WORKAROUND
                                12,          12, 13, 13, INVALID_CODE,
                                INVALID_CODE};
#else
                                12, 12, 13, 13};
#endif

/* bits in base literal/length lookup table */
static ZCONST unsigned lbits = 9;
/* bits in base distance lookup table */
static ZCONST unsigned dbits = 6;

int inflate_codes(__G__ tl, td, bl, bd) __GDEF struct huft *tl,
    *td;         /* literal/length and distance decoder tables */
unsigned bl, bd; /* number of bits decoded by tl[] and td[] */
/* inflate (decompress) the codes in a deflated (compressed) block.
   Return an error code or zero if it all goes ok. */
{
    register unsigned e; /* table entry flag/number of extra bits */
    unsigned d;          /* index for copy */
    UINT_D64 n;          /* length for copy (deflate64: might be 64k+2) */
    UINT_D64 w;          /* current window position (deflate64: up to 64k) */
    struct huft* t;      /* pointer to table entry */
    unsigned ml, md;     /* masks for bl and bd bits */
    register ulg b;      /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local copies of globals */
    b = G.bb; /* initialize bit buffer */
    k = G.bk;
    w = G.wp; /* initialize window position */

    /* inflate the coded data */
    ml = mask_bits[bl]; /* precompute masks for speed */
    md = mask_bits[bd];
    while (1) /* do until end of block */
    {
        NEEDBITS(bl)
        t = tl + ((unsigned)b & ml);
        while (1) {
            DUMPBITS(t->b)

            if ((e = t->e) == 32) /* then it's a literal */
            {
                redirSlide[w++] = (uch)t->v.n;
                if (w == wsize) {
                    if ((retval = FLUSH(w)) != 0)
                        goto cleanup_and_exit;
                    w = 0;
                }
                break;
            }

            if (e < 31) /* then it's a length */
            {
                /* get length of block to copy */
                NEEDBITS(e)
                n = t->v.n + ((unsigned)b & mask_bits[e]);
                DUMPBITS(e)

                /* decode distance of block to copy */
                NEEDBITS(bd)
                t = td + ((unsigned)b & md);
                while (1) {
                    DUMPBITS(t->b)
                    if ((e = t->e) < 32)
                        break;
                    if (IS_INVALID_CODE(e))
                        return 1;
                    e &= 31;
                    NEEDBITS(e)
                    t = t->v.t + ((unsigned)b & mask_bits[e]);
                }
                NEEDBITS(e)
                d = (unsigned)w - t->v.n - ((unsigned)b & mask_bits[e]);
                DUMPBITS(e)

                /* do the copy */
                do {
#if (defined(DLL) && !defined(NO_SLIDE_REDIR))
                    if (G.redirect_slide) {
                        /* &= w/ wsize unnecessary & wrong if redirect */
                        if ((UINT_D64)d >= wsize)
                            return 1; /* invalid compressed data */
                        e = (unsigned)(wsize - (d > (unsigned)w ? (UINT_D64)d : w));
                    } else
#endif
                        e = (unsigned)(wsize -
                                       ((d &= (unsigned)(wsize - 1)) > (unsigned)w
                                            ? (UINT_D64)d
                                            : w));
                    if ((UINT_D64)e > n)
                        e = (unsigned)n;
                    n -= e;
#ifndef NOMEMCPY
                    if ((unsigned)w - d >= e)
                    /* (this test assumes unsigned comparison) */
                    {
                        memcpy(redirSlide + (unsigned)w, redirSlide + d, e);
                        w += e;
                        d += e;
                    } else /* do it slowly to avoid memcpy() overlap */
#endif                     /* !NOMEMCPY */
                        do {
                            redirSlide[w++] = redirSlide[d++];
                        } while (--e);
                    if (w == wsize) {
                        if ((retval = FLUSH(w)) != 0)
                            goto cleanup_and_exit;
                        w = 0;
                    }
                } while (n);
                break;
            }

            if (e == 31) /* it's the EOB signal */
            {
                /* sorry for this goto, but we have to exit two loops at once */
                goto cleanup_decode;
            }

            if (IS_INVALID_CODE(e))
                return 1;

            e &= 31;
            NEEDBITS(e)
            t = t->v.t + ((unsigned)b & mask_bits[e]);
        }
    }
cleanup_decode:

    /* restore the globals from the locals */
    G.wp = (unsigned)w; /* restore global window pointer */
    G.bb = b;           /* restore global bit buffer */
    G.bk = k;

cleanup_and_exit:
    /* done */
    return retval;
}

#define BMAX 16   /* maximum bit length of any code (16 for explode) */
#define N_MAX 288 /* maximum number of codes in any set */

static int huft_free(t) struct huft* t; /* table to free */
/* Free the malloc'ed tables built by huft_build(), which makes a linked
   list of the tables it made, with the links in a dummy first entry of
   each table. */
{
    register struct huft *p, *q;

    /* Go through linked list, freeing from the malloced (t[-1]) address. */
    p = t;
    while (p != (struct huft*)NULL) {
        q = (--p)->v.t;
        free((zvoid*)p);
        p = q;
    }
    return 0;
}

static int huft_build(__G__ b, n, s, d, e, t, m) __GDEF ZCONST
    unsigned* b; /* code lengths in bits (all assumed <= BMAX) */
unsigned n;      /* number of codes (assumed <= N_MAX) */
unsigned s;      /* number of simple-valued codes (0..s-1) */
ZCONST ush* d;   /* list of base values for non-simple codes */
ZCONST uch* e;   /* list of extra bits for non-simple codes */
struct huft** t; /* result: starting table */
unsigned* m;     /* maximum lookup bits, returns actual */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return zero on success, one if
   the given code set is incomplete (the tables are still built in this
   case), two if the input is invalid (all zero length codes or an
   oversubscribed set of lengths), and three if not enough memory.
   The code with value 256 is special, and the tables are constructed
   so that no bits beyond that code are fetched when that code is
   decoded. */
{
    unsigned a;              /* counter for codes of length k */
    unsigned c[BMAX + 1];    /* bit length count table */
    unsigned el;             /* length of EOB code (value 256) */
    unsigned f;              /* i repeats in table every f entries */
    int g;                   /* maximum code length */
    int h;                   /* table level */
    register unsigned i;     /* counter, current code */
    register unsigned j;     /* counter */
    register int k;          /* number of bits in current code */
    int lx[BMAX + 1];        /* memory for l[-1..BMAX-1] */
    int* l = lx + 1;         /* stack of bits per table */
    register unsigned* p;    /* pointer into c[], b[], or v[] */
    register struct huft* q; /* points to current table */
    struct huft r;           /* table entry for structure assignment */
    struct huft* u[BMAX];    /* table stack */
    unsigned v[N_MAX];       /* values in order of bit length */
    register int w;          /* bits before this table == (l * h) */
    unsigned x[BMAX + 1];    /* bit offsets, then code stack */
    unsigned* xp;            /* pointer into x */
    int y;                   /* number of dummy codes added */
    unsigned z;              /* number of entries in current table */

    /* Generate counts for each bit length */
    el = n > 256 ? b[256] : BMAX; /* set length of EOB code, if any */
    memzero((char*)c, sizeof(c));
    p = (unsigned*)b;
    i = n;
    do {
        c[*p]++;
        p++; /* assume all entries <= BMAX */
    } while (--i);
    if (c[0] == n) /* null input--all zero length codes */
    {
        *t = (struct huft*)NULL;
        *m = 0;
        return 0;
    }

    /* Find minimum and maximum length, bound *m by those */
    for (j = 1; j <= BMAX; j++)
        if (c[j])
            break;
    k = j; /* minimum code length */
    if (*m < j)
        *m = j;
    for (i = BMAX; i; i--)
        if (c[i])
            break;
    g = i; /* maximum code length */
    if (*m > i)
        *m = i;

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1)
        if ((y -= c[j]) < 0)
            return 2; /* bad input: more codes than bits */
    if ((y -= c[i]) < 0)
        return 2;
    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;
    xp = x + 2;
    while (--i) { /* note that i == g from above */
        *xp++ = (j += *p++);
    }

    /* Make a table of values in order of bit lengths */
    memzero((char*)v, sizeof(v));
    p = (unsigned*)b;
    i = 0;
    do {
        if ((j = *p++) != 0)
            v[x[j]++] = i;
    } while (++i < n);
    n = x[g]; /* set n to length of v */

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;              /* first Huffman code is zero */
    p = v;                     /* grab values in bit order */
    h = -1;                    /* no tables yet--level -1 */
    w = l[-1] = 0;             /* no bits decoded yet */
    u[0] = (struct huft*)NULL; /* just to keep compilers happy */
    q = (struct huft*)NULL;    /* ditto */
    z = 0;                     /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++) {
        a = c[k];
        while (a--) {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + l[h]) {
                w += l[h++]; /* add bits already decoded */

                /* compute minimum size table less than or equal to *m bits */
                z = (z = g - w) > *m ? *m : z;      /* upper limit */
                if ((f = 1 << (j = k - w)) > a + 1) /* try a k-w bit table */
                {               /* too few codes for k-w bit table */
                    f -= a + 1; /* deduct codes from patterns left */
                    xp = c + k;
                    while (++j < z) /* try smaller tables up to z bits */
                    {
                        if ((f <<= 1) <= *++xp)
                            break; /* enough codes to use up j bits */
                        f -= *xp;  /* else deduct codes from patterns */
                    }
                }
                if ((unsigned)w + j > el && (unsigned)w < el)
                    j = el - w; /* make EOB code end at table */
                z = 1 << j;     /* table entries for j-bit table */
                l[h] = j;       /* set table size in stack */

                /* allocate and link in new table */
                if ((q = (struct huft*)malloc((z + 1) * sizeof(struct huft))) ==
                    (struct huft*)NULL) {
                    if (h)
                        huft_free(u[0]);
                    return 3; /* not enough memory */
                }
#ifdef DEBUG
                G.hufts += z + 1; /* track memory usage */
#endif
                *t = q + 1; /* link to list for huft_free() */
                *(t = &(q->v.t)) = (struct huft*)NULL;
                u[h] = ++q; /* table starts after link */

                /* connect to last table, if there is one */
                if (h) {
                    x[h] = i;            /* save pattern for backing up */
                    r.b = (uch)l[h - 1]; /* bits to dump before this table */
                    r.e = (uch)(32 + j); /* bits in this table */
                    r.v.t = q;           /* pointer to this table */
                    j = (i & ((1 << w) - 1)) >> (w - l[h - 1]);
                    u[h - 1][j] = r; /* connect to last table */
                }
            }

            /* set up table entry in r */
            r.b = (uch)(k - w);
            if (p >= v + n)
                r.e = INVALID_CODE; /* out of values--invalid code */
            else if (*p < s) {
                r.e = (uch)(*p < 256 ? 32 : 31); /* 256 is end-of-block code */
                r.v.n = (ush)*p++;               /* simple code is just the value */
            } else {
                r.e = e[*p - s]; /* non-simple--look up in lists */
                r.v.n = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f)
                q[j] = r;

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i & j; j >>= 1)
                i ^= j;
            i ^= j;

            /* backup over finished tables */
            while ((i & ((1 << w) - 1)) != x[h])
                w -= l[--h]; /* don't need to update q */
        }
    }

    /* return actual size of base table */
    *m = l[0];

    /* Return true (1) if we were given an incomplete table */
    return y != 0 && g != 1;
}

static int inflate_stored(Uz_Globs* __G);
static int inflate_fixed(Uz_Globs* __G);

static int inflate_dynamic(__G) __GDEF
/* decompress an inflated type 2 (dynamic Huffman codes) block. */
{
    unsigned i; /* temporary variables */
    unsigned j;
    unsigned l;                           /* last length */
    unsigned m;                           /* mask for bit lengths table */
    unsigned n;                           /* number of lengths to get */
    struct huft* tl = (struct huft*)NULL; /* literal/length code table */
    struct huft* td = (struct huft*)NULL; /* distance code table */
    struct huft* th; /* temp huft table pointer used in tables decoding */
    unsigned bl;     /* lookup bits for tl */
    unsigned bd;     /* lookup bits for td */
    unsigned nb;     /* number of bit length codes */
    unsigned nl;     /* number of literal/length codes */
    unsigned nd;     /* number of distance codes */
    unsigned ll[MAXLITLENS + MAXDISTS]; /* lit./length and distance code lengths */
    register ulg b;                     /* bit buffer */
    register unsigned k;                /* number of bits in bit buffer */
    int retval = 0; /* error code returned: initialized to "no error" */

    /* make local bit buffer */
    Trace((stderr, "\ndynamic block"));
    b = G.bb;
    k = G.bk;

    /* read in table lengths */
    NEEDBITS(5)
    nl = 257 + ((unsigned)b & 0x1f); /* number of literal/length codes */
    DUMPBITS(5)
    NEEDBITS(5)
    nd = 1 + ((unsigned)b & 0x1f); /* number of distance codes */
    DUMPBITS(5)
    NEEDBITS(4)
    nb = 4 + ((unsigned)b & 0xf); /* number of bit length codes */
    DUMPBITS(4)
    if (nl > MAXLITLENS || nd > MAXDISTS)
        return 1; /* bad lengths */

    /* read in bit-length-code lengths */
    for (j = 0; j < nb; j++) {
        NEEDBITS(3)
        ll[border[j]] = (unsigned)b & 7;
        DUMPBITS(3)
    }
    for (; j < 19; j++)
        ll[border[j]] = 0;

    /* build decoding table for trees--single level, 7 bit lookup */
    bl = 7;
    retval = huft_build(__G__ ll, 19, 19, NULL, NULL, &tl, &bl);
    if (bl == 0) /* no bit lengths */
        retval = 1;
    if (retval) {
        if (retval == 1)
            huft_free(tl);
        return retval; /* incomplete code set */
    }

    /* read in literal and distance code lengths */
    n = nl + nd;
    m = mask_bits[bl];
    i = l = 0;
    while (i < n) {
        NEEDBITS(bl)
        j = (th = tl + ((unsigned)b & m))->b;
        DUMPBITS(j)
        j = th->v.n;
        if (j < 16)          /* length of code in bits (0..15) */
            ll[i++] = l = j; /* save last length in l */
        else if (j == 16)    /* repeat last length 3 to 6 times */
        {
            NEEDBITS(2)
            j = 3 + ((unsigned)b & 3);
            DUMPBITS(2)
            if ((unsigned)i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = l;
        } else if (j == 17) /* 3 to 10 zero length codes */
        {
            NEEDBITS(3)
            j = 3 + ((unsigned)b & 7);
            DUMPBITS(3)
            if ((unsigned)i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = 0;
            l = 0;
        } else /* j == 18: 11 to 138 zero length codes */
        {
            NEEDBITS(7)
            j = 11 + ((unsigned)b & 0x7f);
            DUMPBITS(7)
            if ((unsigned)i + j > n) {
                huft_free(tl);
                return 1;
            }
            while (j--)
                ll[i++] = 0;
            l = 0;
        }
    }

    /* free decoding table for trees */
    huft_free(tl);

    /* restore the global bit buffer */
    G.bb = b;
    G.bk = k;

    /* build the decoding tables for literal/length and distance codes */
    bl = lbits;
#ifdef USE_DEFLATE64
    retval = huft_build(__G__ ll, nl, 257, G.cplens, G.cplext, &tl, &bl);
#else
    retval = huft_build(__G__ ll, nl, 257, cplens, cplext, &tl, &bl);
#endif
    if (bl == 0) /* no literals or lengths */
        retval = 1;
    if (retval) {
        if (retval == 1) {
            // if (!uO.qflag)
            MESSAGE((uch*)"(incomplete l-tree)  ", 21L, 1);
            huft_free(tl);
        }
        return retval; /* incomplete code set */
    }
#ifdef FIX_PAST_EOB_BY_TABLEADJUST
    /* Adjust the requested distance base table size so that a distance code
       fetch never tries to get bits behind an immediatly following end-of-block
       code. */
    bd = (dbits <= bl + 1 ? dbits : bl + 1);
#else
    bd = dbits;
#endif
#ifdef USE_DEFLATE64
    retval = huft_build(__G__ ll + nl, nd, 0, cpdist, G.cpdext, &td, &bd);
#else
    retval = huft_build(__G__ ll + nl, nd, 0, cpdist, cpdext, &td, &bd);
#endif
#ifdef PKZIP_BUG_WORKAROUND
    if (retval == 1)
        retval = 0;
#endif
    if (bd == 0 && nl > 257) /* lengths but no distances */
        retval = 1;
    if (retval) {
        if (retval == 1) {
            // if (!uO.qflag)
            MESSAGE((uch*)"(incomplete d-tree)  ", 21L, 1);
            huft_free(td);
        }
        huft_free(tl);
        return retval;
    }

    /* decompress until an end-of-block code */
    retval = inflate_codes(__G__ tl, td, bl, bd);

cleanup_and_exit:
    /* free the decoding tables, return */
    if (tl != (struct huft*)NULL)
        huft_free(tl);
    if (td != (struct huft*)NULL)
        huft_free(td);
    return retval;
}

static int inflate_block(__G__ e) __GDEF int* e; /* last block flag */
/* decompress an inflated block */
{
    unsigned t;          /* block type */
    register ulg b;      /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local bit buffer */
    b = G.bb;
    k = G.bk;

    /* read in last block bit */
    NEEDBITS(1)
    *e = (int)b & 1;
    DUMPBITS(1)

    /* read in block type */
    NEEDBITS(2)
    t = (unsigned)b & 3;
    DUMPBITS(2)

    /* restore the global bit buffer */
    G.bb = b;
    G.bk = k;

    /* inflate that block type */
    if (t == 2)
        return inflate_dynamic(__G);
    if (t == 0)
        return inflate_stored(__G);
    if (t == 1)
        return inflate_fixed(__G);

    /* bad block type */
    retval = 2;

cleanup_and_exit:
    return retval;
}

static int inflate(__G__ is_defl64) __GDEF int is_defl64;
/* decompress an inflated entry */
{
    int e; /* last block flag */
    int r; /* result code */
#ifdef DEBUG
    unsigned h = 0; /* maximum struct huft's malloc'ed */
#endif

#if (defined(DLL) && !defined(NO_SLIDE_REDIR))
    if (G.redirect_slide)
        wsize = G.redirect_size, redirSlide = G.redirect_buffer;
    else
        wsize = WSIZE, redirSlide = slide; /* how they're #defined if !DLL */
#endif

    /* initialize window, bit buffer */
    G.wp = 0;
    G.bk = 0;
    G.bb = 0;

#ifdef USE_DEFLATE64
    if (is_defl64) {
        G.cplens = cplens64;
        G.cplext = cplext64;
        G.cpdext = cpdext64;
        G.fixed_tl = G.fixed_tl64;
        G.fixed_bl = G.fixed_bl64;
        G.fixed_td = G.fixed_td64;
        G.fixed_bd = G.fixed_bd64;
    } else {
        G.cplens = cplens32;
        G.cplext = cplext32;
        G.cpdext = cpdext32;
        G.fixed_tl = G.fixed_tl32;
        G.fixed_bl = G.fixed_bl32;
        G.fixed_td = G.fixed_td32;
        G.fixed_bd = G.fixed_bd32;
    }
#else  /* !USE_DEFLATE64 */
    if (is_defl64) {
        /* This should not happen unless UnZip is built from object files
         * compiled with inconsistent option setting.  Handle this by
         * returning with "bad input" error code.
         */        Trace((stderr, "\nThis inflate() cannot handle Deflate64!\n"));
        return 2;
    }
#endif /* ?USE_DEFLATE64 */

    /* decompress until the last block */
    do {
#ifdef DEBUG
        G.hufts = 0;
#endif
        if ((r = inflate_block(__G__ & e)) != 0)
            return r;
#ifdef DEBUG
        if (G.hufts > h)
            h = G.hufts;
#endif
    } while (!e);

    Trace((stderr,
           "\n%u bytes in Huffman tables (%u/entry)\n",
           h * (unsigned)sizeof(struct huft),
           (unsigned)sizeof(struct huft)));

#ifdef USE_DEFLATE64
    if (is_defl64) {
        G.fixed_tl64 = G.fixed_tl;
        G.fixed_bl64 = G.fixed_bl;
        G.fixed_td64 = G.fixed_td;
        G.fixed_bd64 = G.fixed_bd;
    } else {
        G.fixed_tl32 = G.fixed_tl;
        G.fixed_bl32 = G.fixed_bl;
        G.fixed_td32 = G.fixed_td;
        G.fixed_bd32 = G.fixed_bd;
    }
#endif

    /* flush out redirSlide and return (success, unless final FLUSH failed) */
    return (FLUSH(G.wp));
}

static int inflate_free(__G) __GDEF {
    if (G.fixed_tl != (struct huft*)NULL) {
        huft_free(G.fixed_td);
        huft_free(G.fixed_tl);
        G.fixed_td = G.fixed_tl = (struct huft*)NULL;
    }
    return 0;
}

static int inflate_stored(__G) __GDEF
/* "decompress" an inflated type 0 (stored) block. */
{
    UINT_D64 w;          /* current window position (deflate64: up to 64k!) */
    unsigned n;          /* number of bytes in block */
    register ulg b;      /* bit buffer */
    register unsigned k; /* number of bits in bit buffer */
    int retval = 0;      /* error code returned: initialized to "no error" */

    /* make local copies of globals */
    Trace((stderr, "\nstored block"));
    b = G.bb; /* initialize bit buffer */
    k = G.bk;
    w = G.wp; /* initialize window position */

    /* go to byte boundary */
    n = k & 7;
    DUMPBITS(n);

    /* get the length and its complement */
    NEEDBITS(16)
    n = ((unsigned)b & 0xffff);
    DUMPBITS(16)
    NEEDBITS(16)
    if (n != (unsigned)((~b) & 0xffff))
        return 1; /* error in compressed data */
    DUMPBITS(16)

    /* read and output the compressed data */
    while (n--) {
        NEEDBITS(8)
        redirSlide[w++] = (uch)b;
        if (w == wsize) {
            if ((retval = FLUSH(w)) != 0)
                goto cleanup_and_exit;
            w = 0;
        }
        DUMPBITS(8)
    }

    /* restore the globals from the locals */
    G.wp = (unsigned)w; /* restore global window pointer */
    G.bb = b;           /* restore global bit buffer */
    G.bk = k;

cleanup_and_exit:
    return retval;
}

// duden
unsigned lit_table[286] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000,
    0x0000, 0x000F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0005, 0x000F, 0x000F, 0x000F,
    0x000F, 0x000F, 0x000F, 0x000A, 0x0008, 0x0008, 0x000C, 0x000F, 0x0007, 0x0007, 0x0007, 0x000B,
    0x0008, 0x0007, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x0008, 0x000A, 0x0008,
    0x000F, 0x000C, 0x0000, 0x000F, 0x0008, 0x0008, 0x0008, 0x0009, 0x0009, 0x0008, 0x0009, 0x0008,
    0x0009, 0x0009, 0x000A, 0x0008, 0x0009, 0x0008, 0x0009, 0x0009, 0x0008, 0x000F, 0x0009, 0x0007,
    0x0009, 0x000A, 0x0009, 0x0009, 0x000F, 0x000F, 0x0009, 0x000A, 0x000B, 0x000A, 0x0000, 0x0000,
    0x0000, 0x0005, 0x0007, 0x0008, 0x0006, 0x0005, 0x0007, 0x0006, 0x0006, 0x0005, 0x000A, 0x0007,
    0x0006, 0x0006, 0x0005, 0x0006, 0x0007, 0x000C, 0x0005, 0x0005, 0x0006, 0x0006, 0x0008, 0x0007,
    0x000A, 0x0009, 0x0007, 0x000F, 0x000F, 0x000F, 0x0000, 0x0000, 0x0000, 0x0000, 0x000F, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0006, 0x000C, 0x000A, 0x0009, 0x000B, 0x0000, 0x000F, 0x000F,
    0x000F, 0x000F, 0x0000, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F,
    0x000F, 0x0000, 0x0000, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000C, 0x000F, 0x000F,
    0x000F, 0x000F, 0x000F, 0x000F, 0x000E, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x0009, 0x000F,
    0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000C, 0x000C, 0x000F, 0x000F, 0x000F,
    0x000C, 0x000F, 0x000F, 0x0000, 0x000C, 0x000B, 0x0000, 0x000A, 0x000F, 0x000F, 0x000F, 0x000F,
    0x0009, 0x000F, 0x0000, 0x000F, 0x000F, 0x000A, 0x000F, 0x000F, 0x000F, 0x000F, 0x000C, 0x000F,
    0x000A, 0x000A, 0x000F, 0x000F, 0x000F, 0x0000, 0x0009, 0x000F, 0x000A, 0x000F, 0x000F, 0x000F,
    0x0009, 0x000F, 0x000F, 0x000F, 0x000C, 0x0003, 0x0003, 0x0004, 0x0005, 0x0006, 0x0006, 0x0007,
    0x0007, 0x0007, 0x0008, 0x0009, 0x000A, 0x000A, 0x000C, 0x000E, 0x000F, 0x000F, 0x000F, 0x000F,
    0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

// duden
unsigned dist_table[30] = {
    0x000D, 0x000D, 0x000C, 0x000B, 0x000A, 0x0009, 0x0008, 0x0007, 0x0006, 0x0006, 0x0005, 0x0005,
    0x0005, 0x0005, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0004, 0x0003, 0x0004, 0x0003, 0x0004,
    0x0004, 0x0006, 0x0000, 0x0000, 0x0000, 0x0000,
};

static int inflate_fixed(__G) __GDEF
/* decompress an inflated type 1 (fixed Huffman codes) block.  We should
   either replace this with a custom decoder, or at least precompute the
   Huffman tables. */
{
    /* if first time, set up tables for fixed blocks */
    Trace((stderr, "\nliteral block"));
    if (G.fixed_tl == (struct huft*)NULL) {
        int i;           /* temporary variable */

        G.fixed_bl = 9;

#ifdef USE_DEFLATE64
        if ((i = huft_build(
                 __G__ l, 288, 257, G.cplens, G.cplext, &G.fixed_tl, &G.fixed_bl)) !=
            0)
#else
        if ((i = huft_build(
                 __G__ lit_table, 286, 257, cplens, cplext, &G.fixed_tl, &G.fixed_bl)) != 0)
#endif
        {
            G.fixed_tl = (struct huft*)NULL;
            return i;
        }

        G.fixed_bd = 5;
#ifdef USE_DEFLATE64
        if ((i = huft_build(
                 __G__ l, MAXDISTS, 0, cpdist, G.cpdext, &G.fixed_td, &G.fixed_bd)) >
            1)
#else
        if ((i = huft_build(
                 __G__ dist_table, 30, 0, cpdist, cpdext, &G.fixed_td, &G.fixed_bd)) >
            1)
#endif
        {
            huft_free(G.fixed_tl);
            G.fixed_td = G.fixed_tl = (struct huft*)NULL;
            return i;
        }
    }

    /* decompress until an end-of-block code */
    return inflate_codes(__G__ G.fixed_tl, G.fixed_td, G.fixed_bl, G.fixed_bd);
}

int duden_inflate(const void* input,
                  unsigned inputSize,
                  void* output,
                  unsigned* outputSize) {
    Uz_Globs globs = {};
    Uz_Globs* pG = &globs;

    G.inptr = G.inbuf = (const uch*)input;
    G.incnt = inputSize;
    G.mem_mode = 1;
    G.outbuf = G.outbufptr = (uch*)output;
    G.outsize = *outputSize;

    int res = inflate(pG, 0);
    *outputSize = G.outcnt;

    return res;
}
