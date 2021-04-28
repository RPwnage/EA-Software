unit madRipeMD;

// RipeMD-160 Secure Hash Function 

// this code is based on Wolfgang Ehrhardt's hash units
// ( http://home.netsurf.de/wolfgang.ehrhardt )
// simplified and reformatted by madshi

// ---------------------------------------------------------------------------
// (C) Copyright 2006-2007 Wolfgang Ehrhardt
//
//  This software is provided 'as-is', without any express or implied warranty.
//  In no event will the authors be held liable for any damages arising from
//  the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software in
//     a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
// ---------------------------------------------------------------------------

interface

{$Q-}{$R-}

uses Windows;

// ---------------------------------------------------------------------------

type TDigest = packed array [0..19] of byte;

function  InitHash   : dword;
procedure UpdateHash (hash: dword; buf: pointer; len: integer);
function  CloseHash  (var hash: dword) : TDigest;

function Hash (buf: pointer; len: integer) : TDigest; overload;
function Hash (buf: AnsiString           ) : TDigest; overload;

// ---------------------------------------------------------------------------

implementation

// ---------------------------------------------------------------------------

type
  THashState   = packed array [0.. 15] of dword;
  THashBuffer  = packed array [0..127] of byte;

  THashContext = packed record
                   state : THashState;  // working hash
                   len   : int64;       // bit msg length
                   buf   : THashBuffer; // block buffer
                   index : integer;     // index in buffer
                 end;

// ---------------------------------------------------------------------------

function InitHash : dword;
// initialize context
begin
  result := LocalAlloc(LPTR, sizeOf(THashContext));
  with THashContext(pointer(result)^) do begin
    state[0] := $67452301;
    state[1] := $efcdab89;
    state[2] := $98badcfe;
    state[3] := $10325476;
    state[4] := $c3d2e1f0;
  end;
end;

// ---------------------------------------------------------------------------

procedure CalculateHash(var data: THashState; const buf: THashBuffer);
// Actual hashing function
type
  THashBuf32 = packed array [0..31] of dword;
const
  k1 = $5a827999;  // 2^30*2^(1/2)
  k2 = $6ed9eba1;  // 2^30*3^(1/2)
  k3 = $8f1bbcdc;  // 2^30*5^(1/2)
  k4 = $a953fd4e;  // 2^30*7^(1/2)
  k5 = $50a28be6;  // 2^30*2^(1/3)
  k6 = $5c4dd124;  // 2^30*3^(1/3)
  k7 = $6d703ef3;  // 2^30*5^(1/3)
  k8 = $7a6d76e9;  // 2^30*7^(1/3)
var
  a, b, c, d, e, a1, b1, c1, d1, e1 : dword;
  x : THashBuf32 absolute buf;
begin
  // Assign old working hash to working variables
  a := data[0];
  b := data[1];
  c := data[2];
  d := data[3];
  e := data[4];

  inc(a, b xor c xor d + x[ 0]); a := a shl 11 or a shr (32 - 11) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[ 1]); e := e shl 14 or e shr (32 - 14) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[ 2]); d := d shl 15 or d shr (32 - 15) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[ 3]); c := c shl 12 or c shr (32 - 12) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[ 4]); b := b shl  5 or b shr (32 -  5) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[ 5]); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[ 6]); e := e shl  7 or e shr (32 -  7) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[ 7]); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[ 8]); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[ 9]); b := b shl 13 or b shr (32 - 13) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[10]); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[11]); e := e shl 15 or e shr (32 - 15) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[12]); d := d shl  6 or d shr (32 -  6) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[13]); c := c shl  7 or c shr (32 -  7) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[14]); b := b shl  9 or b shr (32 -  9) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[15]); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;

  inc(e, c xor (a and (b xor c)) + x[ 7] + k1); e := e shl  7 or e shr (32 -  7) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[ 4] + k1); d := d shl  6 or d shr (32 -  6) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[13] + k1); c := c shl  8 or c shr (32 -  8) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[ 1] + k1); b := b shl 13 or b shr (32 - 13) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[10] + k1); a := a shl 11 or a shr (32 - 11) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 6] + k1); e := e shl  9 or e shr (32 -  9) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[15] + k1); d := d shl  7 or d shr (32 -  7) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[ 3] + k1); c := c shl 15 or c shr (32 - 15) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[12] + k1); b := b shl  7 or b shr (32 -  7) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[ 0] + k1); a := a shl 12 or a shr (32 - 12) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 9] + k1); e := e shl 15 or e shr (32 - 15) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[ 5] + k1); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[ 2] + k1); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[14] + k1); b := b shl  7 or b shr (32 -  7) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[11] + k1); a := a shl 13 or a shr (32 - 13) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 8] + k1); e := e shl 12 or e shr (32 - 12) + d; b := b shl 10 or b shr 22;

  inc(d, b xor (e or (not a)) + x[ 3] + k2); d := d shl 11 or d shr (32 - 11) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[10] + k2); c := c shl 13 or c shr (32 - 13) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[14] + k2); b := b shl  6 or b shr (32 -  6) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[ 4] + k2); a := a shl  7 or a shr (32 -  7) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 9] + k2); e := e shl 14 or e shr (32 - 14) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[15] + k2); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[ 8] + k2); c := c shl 13 or c shr (32 - 13) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[ 1] + k2); b := b shl 15 or b shr (32 - 15) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[ 2] + k2); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 7] + k2); e := e shl  8 or e shr (32 -  8) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[ 0] + k2); d := d shl 13 or d shr (32 - 13) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[ 6] + k2); c := c shl  6 or c shr (32 -  6) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[13] + k2); b := b shl  5 or b shr (32 -  5) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[11] + k2); a := a shl 12 or a shr (32 - 12) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 5] + k2); e := e shl  7 or e shr (32 -  7) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[12] + k2); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;

  inc(c, e xor (a and (d xor e)) + x[ 1] + k3); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[ 9] + k3); b := b shl 12 or b shr (32 - 12) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[11] + k3); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[10] + k3); e := e shl 15 or e shr (32 - 15) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[ 0] + k3); d := d shl 14 or d shr (32 - 14) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[ 8] + k3); c := c shl 15 or c shr (32 - 15) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[12] + k3); b := b shl  9 or b shr (32 -  9) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[ 4] + k3); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[13] + k3); e := e shl  9 or e shr (32 -  9) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[ 3] + k3); d := d shl 14 or d shr (32 - 14) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[ 7] + k3); c := c shl  5 or c shr (32 -  5) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[15] + k3); b := b shl  6 or b shr (32 -  6) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[14] + k3); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[ 5] + k3); e := e shl  6 or e shr (32 -  6) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[ 6] + k3); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[ 2] + k3); c := c shl 12 or c shr (32 - 12) + b; e := e shl 10 or e shr 22;

  inc(b, c xor (d or (not e)) + x[ 4] + k4); b := b shl  9 or b shr (32 -  9) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[ 0] + k4); a := a shl 15 or a shr (32 - 15) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[ 5] + k4); e := e shl  5 or e shr (32 -  5) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[ 9] + k4); d := d shl 11 or d shr (32 - 11) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[ 7] + k4); c := c shl  6 or c shr (32 -  6) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[12] + k4); b := b shl  8 or b shr (32 -  8) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[ 2] + k4); a := a shl 13 or a shr (32 - 13) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[10] + k4); e := e shl 12 or e shr (32 - 12) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[14] + k4); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[ 1] + k4); c := c shl 12 or c shr (32 - 12) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[ 3] + k4); b := b shl 13 or b shr (32 - 13) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[ 8] + k4); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[11] + k4); e := e shl 11 or e shr (32 - 11) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[ 6] + k4); d := d shl  8 or d shr (32 -  8) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[15] + k4); c := c shl  5 or c shr (32 -  5) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[13] + k4); b := b shl  6 or b shr (32 -  6) + a; d := d shl 10 or d shr 22;

  // Save result of first part
  a1 := a;
  b1 := b;
  c1 := c;
  d1 := d;
  e1 := e;

  // Initialize for second part
  a := data[0];
  b := data[1];
  c := data[2];
  d := data[3];
  e := data[4];

  inc(a, b xor (c or (not d)) + x[ 5] + k5); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[14] + k5); e := e shl  9 or e shr (32 -  9) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[ 7] + k5); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[ 0] + k5); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[ 9] + k5); b := b shl 13 or b shr (32 - 13) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[ 2] + k5); a := a shl 15 or a shr (32 - 15) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[11] + k5); e := e shl 15 or e shr (32 - 15) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[ 4] + k5); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[13] + k5); c := c shl  7 or c shr (32 -  7) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[ 6] + k5); b := b shl  7 or b shr (32 -  7) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[15] + k5); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, a xor (b or (not c)) + x[ 8] + k5); e := e shl 11 or e shr (32 - 11) + d; b := b shl 10 or b shr 22;
  inc(d, e xor (a or (not b)) + x[ 1] + k5); d := d shl 14 or d shr (32 - 14) + c; a := a shl 10 or a shr 22;
  inc(c, d xor (e or (not a)) + x[10] + k5); c := c shl 14 or c shr (32 - 14) + b; e := e shl 10 or e shr 22;
  inc(b, c xor (d or (not e)) + x[ 3] + k5); b := b shl 12 or b shr (32 - 12) + a; d := d shl 10 or d shr 22;
  inc(a, b xor (c or (not d)) + x[12] + k5); a := a shl  6 or a shr (32 -  6) + e; c := c shl 10 or c shr 22;

  inc(e, b xor (c and (a xor b)) + x[ 6] + k6); e := e shl  9 or e shr (32 -  9) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[11] + k6); d := d shl 13 or d shr (32 - 13) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[ 3] + k6); c := c shl 15 or c shr (32 - 15) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[ 7] + k6); b := b shl  7 or b shr (32 -  7) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[ 0] + k6); a := a shl 12 or a shr (32 - 12) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[13] + k6); e := e shl  8 or e shr (32 -  8) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[ 5] + k6); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[10] + k6); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[14] + k6); b := b shl  7 or b shr (32 -  7) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[15] + k6); a := a shl  7 or a shr (32 -  7) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[ 8] + k6); e := e shl 12 or e shr (32 - 12) + d; b := b shl 10 or b shr 22;
  inc(d, a xor (b and (e xor a)) + x[12] + k6); d := d shl  7 or d shr (32 -  7) + c; a := a shl 10 or a shr 22;
  inc(c, e xor (a and (d xor e)) + x[ 4] + k6); c := c shl  6 or c shr (32 -  6) + b; e := e shl 10 or e shr 22;
  inc(b, d xor (e and (c xor d)) + x[ 9] + k6); b := b shl 15 or b shr (32 - 15) + a; d := d shl 10 or d shr 22;
  inc(a, c xor (d and (b xor c)) + x[ 1] + k6); a := a shl 13 or a shr (32 - 13) + e; c := c shl 10 or c shr 22;
  inc(e, b xor (c and (a xor b)) + x[ 2] + k6); e := e shl 11 or e shr (32 - 11) + d; b := b shl 10 or b shr 22;

  inc(d, b xor (e or (not a)) + x[15] + k7); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[ 5] + k7); c := c shl  7 or c shr (32 -  7) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[ 1] + k7); b := b shl 15 or b shr (32 - 15) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[ 3] + k7); a := a shl 11 or a shr (32 - 11) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 7] + k7); e := e shl  8 or e shr (32 -  8) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[14] + k7); d := d shl  6 or d shr (32 -  6) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[ 6] + k7); c := c shl  6 or c shr (32 -  6) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[ 9] + k7); b := b shl 14 or b shr (32 - 14) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[11] + k7); a := a shl 12 or a shr (32 - 12) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 8] + k7); e := e shl 13 or e shr (32 - 13) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[12] + k7); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d or (not e)) + x[ 2] + k7); c := c shl 14 or c shr (32 - 14) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c or (not d)) + x[10] + k7); b := b shl 13 or b shr (32 - 13) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b or (not c)) + x[ 0] + k7); a := a shl 13 or a shr (32 - 13) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a or (not b)) + x[ 4] + k7); e := e shl  7 or e shr (32 -  7) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e or (not a)) + x[13] + k7); d := d shl  5 or d shr (32 -  5) + c; a := a shl 10 or a shr 22;

  inc(c, a xor (d and (e xor a)) + x[ 8] + k8); c := c shl 15 or c shr (32 - 15) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[ 6] + k8); b := b shl  5 or b shr (32 -  5) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[ 4] + k8); a := a shl  8 or a shr (32 -  8) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 1] + k8); e := e shl 11 or e shr (32 - 11) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[ 3] + k8); d := d shl 14 or d shr (32 - 14) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[11] + k8); c := c shl 14 or c shr (32 - 14) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[15] + k8); b := b shl  6 or b shr (32 -  6) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[ 0] + k8); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 5] + k8); e := e shl  6 or e shr (32 -  6) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[12] + k8); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[ 2] + k8); c := c shl 12 or c shr (32 - 12) + b; e := e shl 10 or e shr 22;
  inc(b, e xor (c and (d xor e)) + x[13] + k8); b := b shl  9 or b shr (32 -  9) + a; d := d shl 10 or d shr 22;
  inc(a, d xor (b and (c xor d)) + x[ 9] + k8); a := a shl 12 or a shr (32 - 12) + e; c := c shl 10 or c shr 22;
  inc(e, c xor (a and (b xor c)) + x[ 7] + k8); e := e shl  5 or e shr (32 -  5) + d; b := b shl 10 or b shr 22;
  inc(d, b xor (e and (a xor b)) + x[10] + k8); d := d shl 15 or d shr (32 - 15) + c; a := a shl 10 or a shr 22;
  inc(c, a xor (d and (e xor a)) + x[14] + k8); c := c shl  8 or c shr (32 -  8) + b; e := e shl 10 or e shr 22;

  inc(b, c xor d xor e + x[12]); b := b shl  8 or b shr (32 -  8) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[15]); a := a shl  5 or a shr (32 -  5) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[10]); e := e shl 12 or e shr (32 - 12) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[ 4]); d := d shl  9 or d shr (32 -  9) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[ 1]); c := c shl 12 or c shr (32 - 12) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[ 5]); b := b shl  5 or b shr (32 -  5) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[ 8]); a := a shl 14 or a shr (32 - 14) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[ 7]); e := e shl  6 or e shr (32 -  6) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[ 6]); d := d shl  8 or d shr (32 -  8) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[ 2]); c := c shl 13 or c shr (32 - 13) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[13]); b := b shl  6 or b shr (32 -  6) + a; d := d shl 10 or d shr 22;
  inc(a, b xor c xor d + x[14]); a := a shl  5 or a shr (32 -  5) + e; c := c shl 10 or c shr 22;
  inc(e, a xor b xor c + x[ 0]); e := e shl 15 or e shr (32 - 15) + d; b := b shl 10 or b shr 22;
  inc(d, e xor a xor b + x[ 3]); d := d shl 13 or d shr (32 - 13) + c; a := a shl 10 or a shr 22;
  inc(c, d xor e xor a + x[ 9]); c := c shl 11 or c shr (32 - 11) + b; e := e shl 10 or e shr 22;
  inc(b, c xor d xor e + x[11]); b := b shl 11 or b shr (32 - 11) + a; d := d shl 10 or d shr 22;

  // Combine parts 1 and 2
  d       := data[1] + c1 + d;
  data[1] := data[2] + d1 + e;
  data[2] := data[3] + e1 + a;
  data[3] := data[4] + a1 + b;
  data[4] := data[0] + b1 + c;
  data[0] := d;
end;

// ---------------------------------------------------------------------------

procedure UpdateHash(hash: dword; buf: pointer; len: integer);
var context : ^THashContext;
begin
  if hash <> 0 then begin
    context := pointer(hash);
    inc(context.len, int64(len) shl 3);
    while len > 0 do begin
      // fill block with msg data
      context.buf[context.index]:= byte(buf^);
      inc(integer(buf));
      inc(context.index);
      dec(len);
      if context.index = 64 then begin
        // If 512 bit transferred, compress a block
        context.index := 0;
        CalculateHash(context.state, context.buf);
        while len >= 64 do begin
          move(buf^, context.buf, 64);
          CalculateHash(context.state, context.buf);
          inc(integer(buf), 64);
          dec(len, 64);
        end; 
      end;
    end;
  end;
end;

// ---------------------------------------------------------------------------

function CloseHash(var hash: dword) : TDigest;
// finalize hash calculation, clear context
var i       : integer;
    context : ^THashContext;
begin
  if hash <> 0 then begin
    context := pointer(hash);
    // 1. append bit '1' after msg
    context.buf[context.index] := $80;
    for i := context.index + 1 to 63 do
      context.buf[i] := 0;
    // 2. Compress if more than 448 bits (no room for 64 bit length)
    if context.index >= 56 then begin
      CalculateHash(context.state, context.buf);
      ZeroMemory(@context.buf, 56);
    end;
    // Write 64 bit msg length into the last bits of the last block and do a final compress
    int64(pointer(@context.buf[56])^) := context.len;
    CalculateHash(context.state, context.buf);
    // Transfer context.Hash to Digest
    Move(context.state, result, sizeof(result));
    // Clear context
    LocalFree(hash);
    hash := 0;
  end else
    ZeroMemory(@result, sizeOf(result));
end;

// ---------------------------------------------------------------------------

function Hash(buf: pointer; len: integer) : TDigest;
var hash : dword;
begin
  hash := InitHash;
  UpdateHash(hash, buf, len);
  result := CloseHash(hash);
end;

function Hash(buf: AnsiString) : TDigest;
var hash : dword;
begin
  hash := InitHash;
  UpdateHash(hash, PAnsiChar(buf), length(buf));
  result := CloseHash(hash);
end;

// ---------------------------------------------------------------------------

end.
