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
 *   Martin Valgur       (martin.valgur@gmail.com) */

#include "ncompress.h"

#include <cstdio>
#include <cstring>
#include <istream>
#include <ostream>
#include <string>

namespace ncompress
{

#ifndef IBUFSIZ
#  define IBUFSIZ BUFSIZ /* Default input buffer size */
#endif
#ifndef OBUFSIZ
#  define OBUFSIZ BUFSIZ /* Default output buffer size */
#endif

using code_int = long;
using count_int = long;
using cmp_code_int = long;
using char_type = unsigned char;
using codetab_type = unsigned short;

/* Defines for third byte of header */
const char_type BIT_MASK = 0x1fU; /* Mask for 'number of compression bits' */
/* Masks 0x20 and 0x40 are free. */
/* I think 0x20 should mean that there is */
/* a fourth header byte (for expansion). */
const char_type BLOCK_MODE = 0x80U; /* Block compression if table is full and
                                           compression rate is dropping flush tables */

/* the next two codes should not be changed lightly, as they must not */
/* lie within the contiguous general code space. */
const code_int FIRST = 257; /* first free entry */
const code_int CLEAR = 256; /* table clear output code */

const int INIT_BITS = 9; /* initial number of bits/code */

const int HBITS = 17; /* 50% occupancy */
const int HSIZE = 1 << HBITS;
const int HMASK = HSIZE - 1;
const int BITS = 16;

const long CHECK_GAP = 10000;

constexpr code_int
MAXCODE(int n)
{
  return 1L << n;
}

void
output(char_type *buf, int &bits, code_int code, int n_bits)
{
  char_type *p = &buf[bits >> 3];
  long i = static_cast<long>(code) << (bits & 0x7);
  p[0] |= static_cast<char_type>(i);
  p[1] |= static_cast<char_type>(i >> 8);
  p[2] |= static_cast<char_type>(i >> 16);
  bits += n_bits;
}

code_int
input(char_type *buf, int &bits, int n_bits, int bitmask)
{
  char_type *p = &buf[bits >> 3];
  long i = ((long)(p[0])) | ((long)(p[1]) << 8) | ((long)(p[2]) << 16);
  code_int code = (i >> (bits & 0x7)) & bitmask;
  bits += n_bits;
  return code;
}

void
reset_n_bits_for_compressor(
    int &n_bits, int &stcode, code_int &free_ent, code_int &extcode, int maxbits)
{
  n_bits = INIT_BITS;
  stcode = 1;
  free_ent = FIRST;
  extcode = MAXCODE(n_bits);
  if (n_bits < maxbits)
    extcode++;
}

void
reset_n_bits_for_decompressor(
    int &n_bits, int &bitmask, int maxbits, code_int &maxcode, code_int maxmaxcode)
{
  n_bits = INIT_BITS;
  bitmask = (1 << n_bits) - 1;
  maxcode = (n_bits == maxbits) ? maxmaxcode : MAXCODE(n_bits) - 1;
}

/* clang-format off */
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
/* clang-format on */

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
 * questions about this implementation to ames!jaw. */
void
compress(std::istream &in, std::ostream &out)
{
  char_type inbuf[IBUFSIZ + 64]; /* Input buffer */
  char_type outbuf[OBUFSIZ + 2048]; /* Output buffer */

  long bytes_in = 0; /* Total number of bytes from input */
  long bytes_out = 0; /* Total number of bytes to output */

  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

  auto clear_htab = [&]() { memset(htab, -1, sizeof(htab)); };

  int n_bits;
  int stcode;
  code_int free_ent;
  code_int extcode;
  int maxbits = BITS; /* user settable max # bits/code */
  reset_n_bits_for_compressor(n_bits, stcode, free_ent, extcode, maxbits);
  int ratio = 0;
  long checkpoint = CHECK_GAP;
  union
  {
    long code = 0;
    struct
    {
      char_type c;
      unsigned short ent;
    } e;
  } fcode;

  memset(outbuf, 0, sizeof(outbuf));
  outbuf[0] = MAGIC_1;
  outbuf[1] = MAGIC_2;
  outbuf[2] = (char_type)(maxbits | BLOCK_MODE);
  int outbits = 3 << 3;
  int boff = outbits;

  clear_htab();

  while (in.good())
  {
    in.read((char *)inbuf, IBUFSIZ);
    int rsize = (int)in.gcount();
    if (rsize <= 0)
      break;

    int rpos = 0;
    if (bytes_in == 0)
    {
      fcode.e.ent = inbuf[0];
      rpos = 1;
    }

    int rlop = 0;

    do
    {
      if (free_ent >= extcode && fcode.e.ent < FIRST)
      {
        if (n_bits < maxbits)
        {
          outbits = (outbits - 1) +
              ((n_bits << 3) - ((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
          boff = outbits;
          ++n_bits;
          extcode = (n_bits < maxbits) ? MAXCODE(n_bits) + 1 : MAXCODE(n_bits);
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
          outbits = (outbits - 1) +
              ((n_bits << 3) - ((outbits - boff - 1 + (n_bits << 3)) % (n_bits << 3)));
          boff = outbits;
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
        int i = rsize - rlop;

        if ((code_int)i > extcode - free_ent)
          i = (int)(extcode - free_ent);
        if (i > (((int)sizeof(outbuf) - 32) * 8 - outbits) / n_bits)
          i = (((int)sizeof(outbuf) - 32) * 8 - outbits) / n_bits;

        if (!stcode && (long)i > checkpoint - bytes_in)
          i = (int)(checkpoint - bytes_in);

        rlop += i;
        bytes_in += i;
      }

      {
        long hp;

        goto next;
      hfound:
        fcode.e.ent = codetab[hp];
      next:
        if (rpos >= rlop)
          goto endlop;
      next2:
        fcode.e.c = inbuf[rpos++];
        {
          long fc = fcode.code;
          hp = (((long)(fcode.e.c)) << (HBITS - 8)) ^ (long)(fcode.e.ent);

          long i = htab[hp];
          if (i == fc)
            goto hfound;
          if (i == -1)
            goto out;

          long p = primetab[fcode.e.c];
        lookup:
          hp = (hp + p) & HMASK;
          i = htab[hp];
          if (i == fc)
            goto hfound;
          if (i == -1)
            goto out;
          hp = (hp + p) & HMASK;
          i = htab[hp];
          if (i == fc)
            goto hfound;
          if (i == -1)
            goto out;
          hp = (hp + p) & HMASK;
          i = htab[hp];
          if (i == fc)
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
 * with those of the compress() routine.  See the definitions above. */
void
decompress(std::istream &in, std::ostream &out)
{
  char_type inbuf[IBUFSIZ + 64]; /* Input buffer */
  char_type outbuf[OBUFSIZ + 2048]; /* Output buffer */

  long bytes_in = 0; /* Total number of bytes from input */

  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

  auto tab_prefixof = [&codetab](code_int i) -> codetab_type & { return codetab[i]; };
  auto tab_suffixof = [&htab](
                          code_int i) -> char_type & { return ((char_type *)htab)[i]; };
  auto de_stack = [&htab]() { return (char_type *)&(htab[HSIZE - 1]); };
  auto clear_tab_prefixof = [&]() { memset(codetab, 0, 256); };

  int insize = 0;
  int rsize;
  while (insize < 3 && in.good())
  {
    in.read((char *)(inbuf + insize), IBUFSIZ);
    rsize = (int)in.gcount();
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
  bytes_in = insize;

  const int maxbits = inbuf[2] & BIT_MASK;
  const int block_mode = inbuf[2] & BLOCK_MODE;
  if (maxbits > BITS)
  {
    throw std::invalid_argument("compressed with " + std::to_string(maxbits) +
        " bits, can only handle " + std::to_string(BITS) + " bits");
  }

  int n_bits;
  int bitmask;
  code_int maxcode;
  const code_int maxmaxcode = MAXCODE(maxbits);
  reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);

  code_int oldcode = -1;
  int finchar = 0;
  int outpos = 0;
  int posbits = 3 << 3;
  code_int free_ent = block_mode ? FIRST : 256;

  clear_tab_prefixof(); /* As above, initialize the first
                           256 entries in the table. */

  for (code_int code = 255; code >= 0; --code)
    tab_suffixof(code) = (char_type)code;

  do
  {
  resetbuf:;
    {
      int o = posbits >> 3;
      int e = (o <= insize) ? insize - o : 0;

      for (int i = 0; i < e; ++i)
        inbuf[i] = inbuf[i + o];

      insize = e;
      posbits = 0;
    }

    if (insize < (int)sizeof(inbuf) - IBUFSIZ)
    {
      in.read((char *)(inbuf + insize), IBUFSIZ);
      if (in.bad())
        read_error();
      rsize = (int)in.gcount();
      insize += rsize;
    }

    int inbits =
        (rsize > 0) ? (insize - insize % n_bits) << 3 : (insize << 3) - (n_bits - 1);

    while (inbits > posbits)
    {
      if (free_ent > maxcode)
      {
        posbits = (posbits - 1) +
            ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3));

        ++n_bits;
        maxcode = (n_bits == maxbits) ? maxmaxcode : MAXCODE(n_bits) - 1;
        bitmask = (1 << n_bits) - 1;
        goto resetbuf;
      }

      code_int code = input(inbuf, posbits, n_bits, bitmask);

      if (oldcode == -1)
      {
        if (code >= 256)
        {
          throw std::invalid_argument(
              "corrupt input - oldcode: -1, code: " + std::to_string((int)(code)));
        }
        oldcode = code;
        finchar = (int)(code);
        outbuf[outpos++] = (char_type)(code);
        continue;
      }

      if (code == CLEAR && block_mode)
      {
        clear_tab_prefixof();
        free_ent = FIRST - 1;
        posbits = (posbits - 1) +
            ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3));
        reset_n_bits_for_decompressor(n_bits, bitmask, maxbits, maxcode, maxmaxcode);
        goto resetbuf;
      }

      code_int incode = code;
      char_type *stackp = de_stack();

      if (code >= free_ent) /* Special case for KwKwK string. */
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
              "corrupt input - insize: %d, posbits: %d, "
              "inbuf: %02X %02X %02X %02X %02X (%d)",
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

      finchar = tab_suffixof(code);
      *--stackp = (char_type)(finchar);

      /* And put them out in forward order */

      {
        int i = (int)(de_stack() - stackp);
        if (outpos + i >= OBUFSIZ)
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
            i = (int)(de_stack() - stackp);
          }
          while (i > 0);
        }
        else
        {
          memcpy(outbuf + outpos, stackp, i);
          outpos += i;
        }
      }

      code = free_ent;
      if (code < maxmaxcode) /* Generate the new entry. */
      {
        tab_prefixof(code) = (unsigned short)oldcode;
        tab_suffixof(code) = (char_type)finchar;
        free_ent = code + 1;
      }

      oldcode = incode; /* Remember previous code. */
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
