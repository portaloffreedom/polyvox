%module TypeDef
%{
// #include "PolyVoxImpl/TypeDef.h"
%}

// %include "PolyVoxImpl/TypeDef.h"
%inline{
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
}
