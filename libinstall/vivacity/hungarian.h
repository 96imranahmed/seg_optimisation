#pragma once

// -*- mode: c, coding: utf-8 -*-

/**
* 𝓞(n³) implementation of the Hungarian algorithm
*
* Copyright (C) 2011, 2014  Mattias Andrée
*
* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar. See
* http://sam.zoy.org/wtfpl/COPYING for more details.
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define __attribute__(A) /* do nothing */
#endif


#ifndef RANDOM_DEVICE
#define RANDOM_DEVICE "/dev/urandom"
#endif

#define CELL_STR  "%li"
#define llong    int_fast64_t
#define byte     int_fast8_t
#define boolean  int_fast8_t

/*
#define null     0
#define true     1
#define false    0



#ifdef DEBUG
#  define debug(X) fprintf(stderr, "\033[31m%s\033[m\n", X)
#else
#  define debug(X) 
#endif

*/

/**
* Cell marking:  none
*/
#define UNMARKED  0L

/**
* Cell marking:  marked
*/
#define MARKED    1L

/**
* Cell marking:  prime
*/
#define PRIME     2L



/**
* Bit set, a set of fixed number of bits/booleans
*/
typedef struct
{
  /**
  * The set of all limbs, a limb consist of 64 bits
  */
  llong* limbs;

  /**
  * Singleton array with the index of the first non-zero limb
  */
  size_t* first;

  /**
  * Array the the index of the previous non-zero limb for each limb
  */
  size_t* prev;

  /**
  * Array the the index of the next non-zero limb for each limb
  */
  size_t* next;

} BitSet;


#ifdef __cplusplus
extern "C" {
#endif

  ssize_t** kuhn_match(long** table, size_t n, size_t m);
  void kuhn_reduceRows(long** t, size_t n, size_t m);
  byte** kuhn_mark(long** t, size_t n, size_t m);
  boolean kuhn_isDone(byte** marks, boolean* colCovered, size_t n, size_t m);
  size_t* kuhn_findPrime(long** t, byte** marks, boolean* rowCovered, boolean* colCovered, size_t n, size_t m);
  void kuhn_altMarks(byte** marks, size_t* altRow, size_t* altCol, ssize_t* colMarks, ssize_t* rowPrimes, size_t* prime, size_t n, size_t m);
  void kuhn_addAndSubtract(long** t, boolean* rowCovered, boolean* colCovered, size_t n, size_t m);
  ssize_t** kuhn_assign(byte** marks, size_t n, size_t m);

  BitSet new_BitSet(size_t size);
  void BitSet_set(BitSet this_, size_t i);
  void BitSet_unset(BitSet this_, size_t i);
  ssize_t BitSet_any(BitSet this_) __attribute__((pure));

  size_t lb(llong x) __attribute__((const));



  void print(long** t, size_t n, size_t m, ssize_t** assignment);

#ifdef __cplusplus
};
#endif