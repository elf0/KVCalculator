#ifndef VMI_H
#define VMI_H

enum DataType{
  dtI8, dtI16, dtI32, dtI64, dtF32, dtF64
};

enum Instruction{
  iExit = 0
  ,iSelectRegister,iSelectRegisters
  ,iLoadU8

  , iI8ToI16, iI8ToI32, iI8ToI64, iI8ToF32, iI8ToF64
  , iI16ToI8, iI16ToI32, iI16ToI64, iI16ToF32, iI16ToF64
  , iI32ToI8, iI32ToI16, iI32ToI64, iI32ToF32, iI32ToF64
  , iI64ToI8, iI64ToI16, iI64ToI32, iI64ToF32, iI64ToF64
  , iF32ToI32, iF64ToI64

  , iAnd, iOr, iNot
  , iU8_BitAnd, iU8_BitOr, iU8_BitNot, iU8_Xor, iU8_ShiftLeft, iU8_ShiftRight
  , iU16_BitAnd, iU16_BitOr, iU16_BitNot, iU16_Xor, iU16_ShiftLeft, iU16_ShiftRight
  , iU32_BitAnd, iU32_BitOr, iU32_BitNot, iU32_Xor, iU32_ShiftLeft, iU32_ShiftRight
  , iU64_BitAnd, iU64_BitOr, iU64_BitNot, iU64_Xor, iU64_ShiftLeft, iU64_ShiftRight

  , iI8_Add, iI8_Sub, iI8_Mul, iI8_Div
  , iI8_Mod, iI8_ShiftRight
  //  , iI8_Equal, iI8_NotEqual, iI8_Less, iI8_LessEqual
  //  , iI8_Greater, iI8_GreaterEqual, iI8_In, iI8_If
  , iI16_Add, iI16_Sub, iI16_Mul, iI16_Div
  , iI16_Mod, iI16_ShiftRight
  //  , iI16_Equal, iI16_NotEqual, iI16_Less, iI16_LessEqual
  //  , iI16_Greater, iI16_GreaterEqual, iI16_In, iI16_If
  , iI32_Add, iI32_Sub, iI32_Mul, iI32_Div, iI32_Mod
  , iI32_ShiftRight
  , iI32_Equal, iI32_NotEqual, iI32_Less, iI32_LessEqual
  , iI32_Greater, iI32_GreaterEqual, iI32_In, iI32_If
  , iI64_Add, iI64_Sub, iI64_Mul, iI64_Div, iI64_Mod
  , iI64_ShiftRight
  , iI64_Equal, iI64_NotEqual, iI64_Less, iI64_LessEqual
  , iI64_Greater, iI64_GreaterEqual, iI64_In, iI64_If
  //  , iI64_AddRegisters, iI64_SubRegisters, iI64_MulRegisters, iI64_DivRegisters
  //  , iI64_ModRegisters
  , iF32_Add, iF32_Sub, iF32_Mul, iF32_Div
  , iF32_Equal, iF32_NotEqual, iF32_Less, iF32_LessEqual
  , iF32_Greater, iF32_GreaterEqual, iF32_In, iF32_If
  , iF64_Add, iF64_Sub, iF64_Mul, iF64_Div
  , iF64_Equal, iF64_NotEqual, iF64_Less, iF64_LessEqual
  , iF64_Greater, iF64_GreaterEqual, iF64_In, iF64_If
  , iOpen, iClose, iInput, iOutput, iAllocate, iFree
  , iByte_Skip, iByte_SkipUntil, iByte_SkipIn
  , iMap_New, iMap_Delete, iMap_Add, iMap_Remove, iMap_Find
  //  , iU16_Skip, iU16_SkipUntil
  //  , iU32_Skip, iU32_SkipUntil
  //  , iU64_Skip, iU64_SkipUntil
};

#endif
