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
 *   Martin Valgur       (martin.valgur@gmail.com)
 */

#pragma once

#include <istream>
#include <ostream>

namespace ncompress
{

/**
 * Applies LZW compression to the input.
 *
 * @throws std::ios_base::failure on stream errors
 */
void compress(std::istream &in, std::ostream &out);

/**
 * Decompresses the LZW-compressed input.
 *
 * @throws std::ios_base::failure on stream errors
 * @throws std::invalid_argument on invalid or corrupted input data
 */
void decompress(std::istream &in, std::ostream &out);

static const unsigned char MAGIC_1 = 0x1fU; /* First byte of compressed file */
static const unsigned char MAGIC_2 = 0x9dU; /* Second byte of compressed file */

} // namespace ncompress
