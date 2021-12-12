/* (N)compress.c - File compression ala IEEE Computer, Mar 1992.
 *
 * Authors:
 *   Spencer W. Thomas   (decvax!harpo!utah-cs!utah-gr!thomas)
 *   Jim McKie           (decvax!mcvax!jim)
 *   Steve Davies        (decvax!vax135!petsd!peora!srd)
 *   Ken Turkowski       (decvax!decwrl!turtlevax!ken)
 *   James A. Woods      (decvax!ihnp4!ames!jaw)
 *   Joe Orost           (decvax!vax135!petsd!joe)
 *   Dave Mack           (csu@alembic.acs.com)
 *   Peter Jannesen, Network Communication Systems
 *                       (peter@ncs.nl)
 *   Mike Frysinger      (vapier@gmail.com)
 */

#include "ncompress.h"

#include <cstdio>
#include <cstring>
#include <istream>
#include <ostream>

#ifndef IBUFSIZ
#  define IBUFSIZ BUFSIZ /* Default input buffer size							*/
#endif
#ifndef OBUFSIZ
#  define OBUFSIZ BUFSIZ /* Default output buffer size							*/
#endif

/* Defines for third byte of header 					*/
#define MAGIC_1  (char_type)'\037' /* First byte of compressed file				*/
#define MAGIC_2  (char_type)'\235' /* Second byte of compressed file				*/
#define BIT_MASK 0x1f /* Mask for 'number of compression bits'		*/
/* Masks 0x20 and 0x40 are free.  				*/
/* I think 0x20 should mean that there is		*/
/* a fourth header byte (for expansion).    	*/
#define BLOCK_MODE 0x80 /* Block compression if table is full and		*/
/* compression rate is dropping flush tables	*/

/* the next two codes should not be changed lightly, as they must not	*/
/* lie within the contiguous general code space.						*/
#define FIRST 257 /* first free entry 							*/
#define CLEAR 256 /* table clear output code 						*/

#define INIT_BITS 9 /* initial number of bits/code */

#define HBITS  17 /* 50% occupancy */
#define HSIZE  (1 << HBITS)
#define HMASK  (HSIZE - 1)
#define HPRIME 9941
#define BITS   16

#define CHECK_GAP 10000

typedef long int code_int;
typedef long int count_int;
typedef long int cmp_code_int;
typedef unsigned char char_type;

#define MAXCODE(n) (1L << (n))

#define output(b, o, c, n)             \
  {                                    \
    char_type *p = &(b)[(o) >> 3];     \
    long i = ((long)(c)) << ((o)&0x7); \
    p[0] |= (char_type)(i);            \
    p[1] |= (char_type)(i >> 8);       \
    p[2] |= (char_type)(i >> 16);      \
    (o) += (n);                        \
  }
#define input(b, o, c, n, m)                       \
  {                                                \
    char_type *p = &(b)[(o) >> 3];                 \
    (c) = ((((long)(p[0])) | ((long)(p[1]) << 8) | \
               ((long)(p[2]) << 16)) >>            \
              ((o)&0x7)) &                         \
        (m);                                       \
    (o) += (n);                                    \
  }

#define reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits) \
  {                                                                             \
    n_bits = INIT_BITS;                                                         \
    stcode = 1;                                                                 \
    free_ent = FIRST;                                                           \
    extcode = MAXCODE(n_bits);                                                  \
    if (n_bits < maxbits)                                                       \
      extcode++;                                                                \
  }

#define reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode) \
  {                                                                                  \
    n_bits = INIT_BITS;                                                              \
    bitmask = (1 << n_bits) - 1;                                                     \
    if (n_bits == maxbits)                                                           \
      maxcode = maxmaxcode;                                                          \
    else                                                                             \
      maxcode = MAXCODE(n_bits) - 1;                                                 \
  }

#define tab_prefixof(i)      codetab[i]
#define tab_suffixof(i)      ((char_type *)(htab))[i]
#define de_stack             ((char_type *)&(htab[HSIZE - 1]))
#define clear_htab()         memset(htab, -1, sizeof(htab))
#define clear_tab_prefixof() memset(codetab, 0, 256);

namespace ncompress
{

static const int primetab[256] = /* Special secudary hash table.		*/
    {
      1013, -1061, 1109, -1181, 1231, -1291, 1361, -1429,
      1481, -1531, 1583, -1627, 1699, -1759, 1831, -1889,
      1973, -2017, 2083, -2137, 2213, -2273, 2339, -2383,
      2441, -2531, 2593, -2663, 2707, -2753, 2819, -2887,
      2957, -3023, 3089, -3181, 3251, -3313, 3361, -3449,
      3511, -3557, 3617, -3677, 3739, -3821, 3881, -3931,
      4013, -4079, 4139, -4219, 4271, -4349, 4423, -4493,
      4561, -4639, 4691, -4783, 4831, -4931, 4973, -5023,
      5101, -5179, 5261, -5333, 5413, -5471, 5521, -5591,
      5659, -5737, 5807, -5857, 5923, -6029, 6089, -6151,
      6221, -6287, 6343, -6397, 6491, -6571, 6659, -6709,
      6791, -6857, 6917, -6983, 7043, -7129, 7213, -7297,
      7369, -7477, 7529, -7577, 7643, -7703, 7789, -7873,
      7933, -8017, 8093, -8171, 8237, -8297, 8387, -8461,
      8543, -8627, 8689, -8741, 8819, -8867, 8963, -9029,
      9109, -9181, 9241, -9323, 9397, -9439, 9511, -9613,
      9677, -9743, 9811, -9871, 9941, -10061, 10111, -10177,
      10259, -10321, 10399, -10477, 10567, -10639, 10711, -10789,
      10867, -10949, 11047, -11113, 11173, -11261, 11329, -11423,
      11491, -11587, 11681, -11777, 11827, -11903, 11959, -12041,
      12109, -12197, 12263, -12343, 12413, -12487, 12541, -12611,
      12671, -12757, 12829, -12917, 12979, -13043, 13127, -13187,
      13291, -13367, 13451, -13523, 13619, -13691, 13751, -13829,
      13901, -13967, 14057, -14153, 14249, -14341, 14419, -14489,
      14557, -14633, 14717, -14767, 14831, -14897, 14983, -15083,
      15149, -15233, 15289, -15359, 15427, -15497, 15583, -15649,
      15733, -15791, 15881, -15937, 16057, -16097, 16189, -16267,
      16363, -16447, 16529, -16619, 16691, -16763, 16879, -16937,
      17021, -17093, 17183, -17257, 17341, -17401, 17477, -17551,
      17623, -17713, 17791, -17891, 17957, -18041, 18097, -18169,
      18233, -18307, 18379, -18451, 18523, -18637, 18731, -18803,
      18919, -19031, 19121, -19211, 19273, -19381, 19429, -19477
    };

void read_error();
void write_error();

/*
 * Compress in stream to out stream.
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */
void
compress(std::istream &in, std::ostream &out)
{
  long hp;
  int rpos;
  long fc;
  int outbits;
  int rlop;
  int rsize;
  int stcode;
  code_int free_ent;
  int boff;
  int n_bits;
  int ratio = 0;
  long checkpoint = CHECK_GAP;
  code_int extcode;
  union
  {
    long code = 0;
    struct
    {
      char_type c;
      unsigned short ent;
    } e;
  } fcode;

  int maxbits = BITS; /* user settable max # bits/code 				*/

  char_type inbuf[IBUFSIZ + 64]; /* Input buffer									*/
  char_type outbuf[OBUFSIZ + 2048]; /* Output buffer								*/

  long bytes_in = 0; /* Total number of bytes from input				*/
  long bytes_out = 0; /* Total number of bytes to output				*/

  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

  reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits);

  memset(outbuf, 0, sizeof(outbuf));
  outbuf[0] = MAGIC_1;
  outbuf[1] = MAGIC_2;
  outbuf[2] = (char)(maxbits | BLOCK_MODE);
  boff = outbits = (3 << 3);

  clear_htab();

  while (in.good())
  {
    in.read((char *)inbuf, IBUFSIZ);
    rsize = in.gcount();
    if (rsize <= 0)
      break;

    if (bytes_in == 0)
    {
      fcode.e.ent = inbuf[0];
      rpos = 1;
    }
    else
      rpos = 0;

    rlop = 0;

    do
    {
      if (free_ent >= extcode && fcode.e.ent < FIRST)
      {
        if (n_bits < maxbits)
        {
          boff = outbits = (outbits - 1) + ((n_bits << 3) - ((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
          if (++n_bits < maxbits)
            extcode = MAXCODE(n_bits) + 1;
          else
            extcode = MAXCODE(n_bits);
        }
        else
        {
          extcode = MAXCODE(16) + OBUFSIZ;
          stcode = 0;
        }
      }

      if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST)
      {
        long int rat;

        checkpoint = bytes_in + CHECK_GAP;

        if (bytes_in > 0x007fffff)
        { /* shift will overflow */
          rat = (bytes_out + (outbits >> 3)) >> 8;

          if (rat == 0) /* Don't divide by zero */
            rat = 0x7fffffff;
          else
            rat = bytes_in / rat;
        }
        else
          rat = (bytes_in << 8) / (bytes_out + (outbits >> 3)); /* 8 fractional bits */
        if (rat >= ratio)
          ratio = (int)rat;
        else
        {
          ratio = 0;
          clear_htab();
          output(outbuf, outbits, CLEAR, n_bits);
          boff = outbits = (outbits - 1) + ((n_bits << 3) - ((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
          reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits);
        }
      }

      if (outbits >= (OBUFSIZ << 3))
      {
        out.write((char *)outbuf, OBUFSIZ);
        if (out.fail())
          write_error();

        outbits -= (OBUFSIZ << 3);
        boff = -(((OBUFSIZ << 3) - boff) % (n_bits << 3));
        bytes_out += OBUFSIZ;

        memcpy(outbuf, outbuf + OBUFSIZ, (outbits >> 3) + 1);
        memset(outbuf + (outbits >> 3) + 1, '\0', OBUFSIZ);
      }

      {
        int i;

        i = rsize - rlop;

        if ((code_int)i > extcode - free_ent)
          i = (int)(extcode - free_ent);
        if (i > (((int)sizeof(outbuf) - 32) * 8 - outbits) / n_bits)
          i = (((int)sizeof(outbuf) - 32) * 8 - outbits) / n_bits;

        if (!stcode && (long)i > checkpoint - bytes_in)
          i = (int)(checkpoint - bytes_in);

        rlop += i;
        bytes_in += i;
      }

      goto next;
    hfound:
      fcode.e.ent = codetab[hp];
    next:
      if (rpos >= rlop)
        goto endlop;
    next2:
      fcode.e.c = inbuf[rpos++];
      {
        long i;
        long p;
        fc = fcode.code;
        hp = ((((long)(fcode.e.c)) << (HBITS - 8)) ^ (long)(fcode.e.ent));

        if ((i = htab[hp]) == fc)
          goto hfound;
        if (i == -1)
          goto out;

        p = primetab[fcode.e.c];
      lookup:
        hp = (hp + p) & HMASK;
        if ((i = htab[hp]) == fc)
          goto hfound;
        if (i == -1)
          goto out;
        hp = (hp + p) & HMASK;
        if ((i = htab[hp]) == fc)
          goto hfound;
        if (i == -1)
          goto out;
        hp = (hp + p) & HMASK;
        if ((i = htab[hp]) == fc)
          goto hfound;
        if (i == -1)
          goto out;
        goto lookup;
      }
    out:;
      output(outbuf, outbits, fcode.e.ent, n_bits);

      {
        long fc = fcode.code;
        fcode.e.ent = fcode.e.c;

        if (stcode)
        {
          codetab[hp] = (unsigned short)free_ent++;
          htab[hp] = fc;
        }
      }

      goto next;

    endlop:
      if (fcode.e.ent >= FIRST && rpos < rsize)
        goto next2;

      if (rpos > rlop)
      {
        bytes_in += rpos - rlop;
        rlop = rpos;
      }
    }
    while (rlop < rsize);
  }

  if (in.bad())
    read_error();

  if (bytes_in > 0)
    output(outbuf, outbits, fcode.e.ent, n_bits);

  out.write((char *)outbuf, (outbits + 7) >> 3);
  if (out.fail())
    write_error();

  bytes_out += (outbits + 7) >> 3;
}

/*
 * Decompress input stream to output stream. This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */
void
decompress(std::istream &in, std::ostream &out)
{
  char_type *stackp;
  code_int code;
  int finchar;
  code_int oldcode;
  code_int incode;
  int inbits;
  int posbits;
  int outpos;
  int insize = 0;
  int bitmask;
  code_int free_ent;
  code_int maxcode;
  code_int maxmaxcode;
  int n_bits;
  int rsize;
  int block_mode;

  int maxbits = BITS; /* user settable max # bits/code 				*/

  char_type inbuf[IBUFSIZ + 64]; /* Input buffer									*/
  char_type outbuf[OBUFSIZ + 2048]; /* Output buffer								*/

  long bytes_in = 0; /* Total number of bytes from input				*/

  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

  while (insize < 3 && in.good())
  {
    in.read((char *)(inbuf + insize), IBUFSIZ);
    rsize = in.gcount();
    insize += rsize;
  }

  if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2)
  {
    if (in.bad())
      read_error();
    if (insize == 0)
      throw std::invalid_argument("input stream is empty");
    throw std::invalid_argument("not in LZW-compressed format");
  }

  maxbits = inbuf[2] & BIT_MASK;
  block_mode = inbuf[2] & BLOCK_MODE;

  if (maxbits > BITS)
  {
    throw std::invalid_argument("compressed with " + std::to_string(maxbits) +
        " bits, can only handle " + std::to_string(BITS) + " bits");
  }

  maxmaxcode = MAXCODE(maxbits);

  bytes_in = insize;
  reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
  oldcode = -1;
  finchar = 0;
  outpos = 0;
  posbits = 3 << 3;

  free_ent = ((block_mode) ? FIRST : 256);

  clear_tab_prefixof(); /* As above, initialize the first
                   256 entries in the table. */

  for (code = 255; code >= 0; --code)
    tab_suffixof(code) = (char_type)code;

  do
  {
  resetbuf:;
    {
      int i;
      int e;
      int o;

      o = posbits >> 3;
      e = o <= insize ? insize - o : 0;

      for (i = 0; i < e; ++i)
        inbuf[i] = inbuf[i + o];

      insize = e;
      posbits = 0;
    }

    if (insize < (int)sizeof(inbuf) - IBUFSIZ)
    {
      in.read((char *)(inbuf + insize), IBUFSIZ);
      if (in.bad())
        read_error();
      rsize = in.gcount();
      insize += rsize;
    }

    inbits = ((rsize > 0) ? (insize - insize % n_bits) << 3 : (insize << 3) - (n_bits - 1));

    while (inbits > posbits)
    {
      if (free_ent > maxcode)
      {
        posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));

        ++n_bits;
        if (n_bits == maxbits)
          maxcode = maxmaxcode;
        else
          maxcode = MAXCODE(n_bits) - 1;

        bitmask = (1 << n_bits) - 1;
        goto resetbuf;
      }

      input(inbuf, posbits, code, n_bits, bitmask);

      if (oldcode == -1)
      {
        if (code >= 256)
        {
          throw std::invalid_argument("corrupt input - oldcode: -1, code: " + std::to_string((int)(code)));
        }
        outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
        continue;
      }

      if (code == CLEAR && block_mode)
      {
        clear_tab_prefixof();
        free_ent = FIRST - 1;
        posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
        reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
        goto resetbuf;
      }

      incode = code;
      stackp = de_stack;

      if (code >= free_ent) /* Special case for KwKwK string.	*/
      {
        if (code > free_ent)
        {
          char_type *p;

          posbits -= n_bits;
          p = &inbuf[posbits >> 3];
          if (p == inbuf)
            p++;

          char err[200];
          snprintf(err, 200,
              "corrupt input - insize: %d, posbits: %d, inbuf: %02X %02X %02X %02X %02X (%d)",
              insize, posbits, p[-1], p[0], p[1], p[2], p[3], (posbits & 07));
          throw std::invalid_argument(err);
        }

        *--stackp = (char_type)finchar;
        code = oldcode;
      }

      while ((cmp_code_int)code >= (cmp_code_int)256)
      { /* Generate output characters in reverse order */
        *--stackp = tab_suffixof(code);
        code = tab_prefixof(code);
      }

      *--stackp = (char_type)(finchar = tab_suffixof(code));

      /* And put them out in forward order */

      {
        int i;

        if (outpos + (i = (de_stack - stackp)) >= OBUFSIZ)
        {
          do
          {
            if (i > OBUFSIZ - outpos)
              i = OBUFSIZ - outpos;

            if (i > 0)
            {
              memcpy(outbuf + outpos, stackp, i);
              outpos += i;
            }

            if (outpos >= OBUFSIZ)
            {
              out.write((char *)outbuf, outpos);
              if (out.fail())
                write_error();

              outpos = 0;
            }
            stackp += i;
          }
          while ((i = (de_stack - stackp)) > 0);
        }
        else
        {
          memcpy(outbuf + outpos, stackp, i);
          outpos += i;
        }
      }

      if ((code = free_ent) < maxmaxcode) /* Generate the new entry. */
      {
        tab_prefixof(code) = (unsigned short)oldcode;
        tab_suffixof(code) = (char_type)finchar;
        free_ent = code + 1;
      }

      oldcode = incode; /* Remember previous code.	*/
    }

    bytes_in += rsize;
  }
  while (rsize > 0);

  if (outpos > 0)
  {
    out.write((char *)outbuf, outpos);
    if (out.fail())
      write_error();
  }
}

void
read_error()
{
  throw std::ios_base::failure("reading from input stream failed");
}

void
write_error()
{
  throw std::ios_base::failure("writing to output stream failed");
}

} // namespace ncompress
