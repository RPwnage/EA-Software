/*
**********************************************************************
** md5.h -- Header file for implementation of MD5                   **
** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
** Created: 2/17/90 RLR                                             **
** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
** Revised (for MD5): RLR 4/27/91                                   **
**   -- G modified to have y&~z instead of y&z                      **
**   -- FF, GG, HH modified to add in last register done            **
**   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
**   -- distinct additive constant for each step                    **
**   -- round 4 added, working mod 7                                **
**********************************************************************
*/

/*
**********************************************************************
** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
**                                                                  **
** License to copy and use this software is granted provided that   **
** it is identified as the "RSA Data Security, Inc. MD5 Message     **
** Digest Algorithm" in all material mentioning or referencing this **
** software or this function.                                       **
**                                                                  **
** License is also granted to make and use derivative works         **
** provided that such works are identified as "derived from the RSA **
** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
** material mentioning or referencing the derived work.             **
**                                                                  **
** RSA Data Security, Inc. makes no representations concerning      **
** either the merchantability of this software or the suitability   **
** of this software for any particular purpose.  It is provided "as **
** is" without express or implied warranty of any kind.             **
**                                                                  **
** These notices must be retained in any copies of any part of this **
** documentation and/or software.                                   **
**********************************************************************
*/

#include <stdio.h>
#include <wchar.h>

/* typedef a 32 bit type */
typedef unsigned long int UINT4;

/*
Copyright (C) 1999, 2002 Aladdin Enterprises.  All rights reserved.

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

L. Peter Deutsch
ghost@aladdin.com

*/
/* $Id: md5.h,v 1.4 2002/04/13 19:20:28 lpd Exp $ */
/*
Independent implementation of MD5 (RFC 1321).

This code implements the MD5 Algorithm defined in RFC 1321, whose
text is available at
http://www.ietf.org/rfc/rfc1321.txt
The code is derived from the text of the RFC, including the test suite
(section A.5) but excluding the rest of Appendix A.  It does not include
any code or documentation that is identified in the RFC as being
copyrighted.

The original and principal author of md5.h is L. Peter Deutsch
<ghost@aladdin.com>.  Other authors are noted in the change history
that follows (in reverse chronological order):

2002-04-13 lpd Removed support for non-ANSI compilers; removed
references to Ghostscript; clarified derivation from RFC 1321;
now handles byte order either statically or dynamically.
1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
added conditionalization for C++ compilation from Martin
Purschke <purschke@bnl.gov>.
1999-05-03 lpd Original version.
*/

#ifndef md5_INCLUDED
#define md5_INCLUDED

typedef unsigned char md5_byte_t; /* 8-bit byte */
typedef unsigned int md5_word_t; /* 32-bit word */

/* Define the state of the MD5 Algorithm. */
typedef struct md5_state_s {
	md5_word_t count[2];	/* message length in bits, lsw first */
	md5_word_t abcd[4];		/* digest buffer */
	md5_byte_t buf[64];		/* accumulate block */
} md5_state_t;

extern void calculate_md5(const void* data, unsigned int dataLength, char md5[33]);

extern void md5_init(md5_state_t *pms);

extern void md5_append(md5_state_t *pms, const md5_byte_t *data, int nbytes);

extern void md5_finish(md5_state_t *pms, md5_byte_t digest[16]);
/*
**********************************************************************
** End of md5.h                                                     **
******************************* (cut) ********************************
*/
#endif