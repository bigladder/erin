/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_CONST_H
#define ERIN_CONST_H

#include <cstdint>
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

namespace erin
{
    // NOTE: see
    // https://herbsutter.com/2009/10/18/mailbag-shutting-up-compiler-warnings/
    // allows us to ignore unused variables that are there intentionally.
    // For example, a variable used in an assert() (that is then compiled out).
    template<class T>
    void
    ignore(T const&)
    {
    }
}

#endif
