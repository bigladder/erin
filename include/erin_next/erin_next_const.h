/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_CONST_H
#define ERIN_CONST_H

// TODO[mok]: add #define for the index type; currently size_t but not necessary
// to be so big; we might try a uint16_t or uint32_t; possibly in pair with enum
// for type:
// struct CompId { ComponentType Type; uint16_t Id; uint16_T SubtypeId; };
// define flow type to switch easily
// TODO[mok]: things to try
// - unsigned has modulo wrap around which is NOT what we want
// - to prevent that, we have to compare to max which causes if statements
//   to get interwoven with addition. We might want to try using signed
//   with -1 meaning infinity or no limit?
#define flow_t uint64_t

#endif
