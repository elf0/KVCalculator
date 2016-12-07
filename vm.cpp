//License: Public Domain
//Author: elf
//EMail: elf@elf0.org

#include <stdio.h>
#include <map>
//#include <string>

#include "MMFile.h"
//#include "String2U.h"
#include "vmi.h"

//CPU definition
typedef struct{
  union{
    B bTrue;
    E8 e8;
    U8 u8;
    U16 u16;
    U32 u32;
    U64 u64;
    U8 szu8[8];
    U16 szu16[4];
    U32 szu32[2];
    I8 i8;
    I16 i16;
    I32 i32;
    I64 i64;
    F32 f32;
    F64 f64;
    Byte *ptr;
  };
}Register;


//typedef bool (*MapComparer)(U64 left, U64 right);
struct Map{
  //  Map(MapComparer less): _map(less){}
  //  class Comparer {
  //  public:
  //    Comparer(MapComparer less) : _less(less){}
  //    bool operator()(U64 left, U64 right)const{
  //      return _less(left, right);
  //    }
  //  private:
  //    MapComparer _less;
  //  };
public:
  B Add(Byte *pBegin, U32 uSize, U64 uValue){
    return _map.emplace(Key{uSize, pBegin}, uValue).second;
  }

  B Remove(Byte *pBegin, U32 uSize){
    return _map.erase(Key{uSize, pBegin});
  }

  U64 Find(Byte *pBegin, U32 uSize){
    auto iter = _map.find(Key{uSize, pBegin});
    if(iter == _map.end())
      return 0;
    return iter->second;
  }

private:
  struct Key{
    U32 uSize;
    Byte *pBegin;
  };

  class Less {
  public:
    bool operator()(const Key &left, const Key &right)const{
      Byte *pL = left.pBegin;
      Byte *pR = right.pBegin;

      if(left.uSize < right.uSize){
        Byte *pLEnd = pL + left.uSize;
        while(pL != pLEnd){
          if(*pL < *pR)
            return true;

          if(*pL > *pR)
            return false;

          ++pL;
          ++pR;
        }

        return true;
      }

      Byte *pREnd = pR + right.uSize;
      while(pR != pREnd){
        if(*pL < *pR)
          return true;

        if(*pL > *pR)
          return false;

        ++pL;
        ++pR;
      }

      return false;
    }
  };
  std::map<Key, U64, Less> _map;
};

#define BUFFER_SIZE 65535
static Byte g_szBuffer[BUFFER_SIZE + 1];

//static U8 g_uTypeRegister = dtI64;
#define REGISTERS_COUNT 256
static Register g_szRegisters[REGISTERS_COUNT];
static Register *g_szrSelectors[REGISTERS_COUNT] = {
  &g_szRegisters[0], &g_szRegisters[1], &g_szRegisters[2], &g_szRegisters[3],
  &g_szRegisters[4], &g_szRegisters[5], &g_szRegisters[6], &g_szRegisters[7],
};
//static int g_iInputFile = 0;
//static int g_iOutputFile = 1;
//static Register *g_prSource = nullptr;
//static B g_bBooleanRegister = false;
//static E8 g_eErrorRegister = false;

inline
static E8 Execute(const Byte *pBegin, const Byte *pEnd, const Byte *pData, const Byte *pDataEnd);

int main(int argc, char *argv[]){
  if(argc != 2)
    return 1;

  MMFile file;
  if(MMFile_OpenForRead(&file, (C*)argv[1]))
    return 2;

  //  g_uLine = 0;
  Byte *pBegin = file.pBegin;
  Byte *pEnd = pBegin + file.file.meta.st_size;
  E8 e = Execute(pBegin, pEnd, 0, 0);

  MMFile_Close(&file);
  return e;
}

inline
static E8 ExecuteInstruction(const Byte **ppBegin, const Byte *pEnd);

inline
static E8 Execute(const Byte *pBegin, const Byte *pEnd, const Byte *pData, const Byte *pDataEnd){
  //TODO: Remove these in furture
  g_szRegisters[254].u64 = BUFFER_SIZE;
  g_szRegisters[255].ptr = g_szBuffer;
  //  g_szrSelectors[1]->ptr = g_szBuffer;
  //  g_szrSelectors[2]->u64 = BUFFER_SIZE;

  E8 e;
  //  const Byte *pD = pData;
  const U8 *pI = pBegin;
  while(pI != pEnd){
    e = ExecuteInstruction(&pI, pEnd);
    if(e)
      return e;
  }

  return 0;
}

inline
static E8 ExecuteInstruction(const Byte **ppBegin, const Byte *pEnd){
  const Byte *pI = *ppBegin;

  switch(*pI++){
  default:
    fprintf(stderr, "Error: Invalid instruction \"%d\"!\n", pI[-1]);
    return 1;
  case iExit:
    return g_szrSelectors[0]->e8;

  case iSelectRegister:{
    const U8 *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 index = *pI++;
    g_szrSelectors[index] = &g_szRegisters[*pI];
    pI = pE;
  }break;
  case iSelectRegisters:{
    if(pI == pEnd)
      goto UNEXPECTED_END;
    U8 uPairs = *pI++;

    const U8 *pE = pI + (uPairs << 1);
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uTarget;
    while(pI != pE){
      uTarget = *pI++;
      g_szrSelectors[uTarget] = &g_szRegisters[*pI++];
    }
  }break;

  case iLoadU8:{
    const U8 *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Register *pR = g_szrSelectors[*pI++];
    pR->u8 = *pI;
    pI = pE;
  }break;

  case iAnd:{
    g_szrSelectors[0]->bTrue = g_szrSelectors[1]->bTrue && g_szrSelectors[2]->bTrue;
  }break;
  case iOr:{
    g_szrSelectors[0]->bTrue = g_szrSelectors[1]->bTrue || g_szrSelectors[2]->bTrue;
  }break;
  case iNot:{
    g_szrSelectors[0]->bTrue = !g_szrSelectors[1]->bTrue;
  }break;

  case iU8_BitAnd:{
    g_szrSelectors[0]->u8 = g_szrSelectors[1]->u8 & g_szrSelectors[2]->u8;
  }break;
  case iU8_BitOr:{
    g_szrSelectors[0]->u8 = g_szrSelectors[1]->u8 | g_szrSelectors[2]->u8;
  }break;
  case iU8_BitNot:{
    g_szrSelectors[0]->u8 = ~g_szrSelectors[1]->u8;
  }break;
  case iU8_Xor:{
    g_szrSelectors[0]->u8 = g_szrSelectors[1]->u8 ^ g_szrSelectors[2]->u8;
  }break;
  case iU8_ShiftLeft:{
    g_szrSelectors[0]->u8 = g_szrSelectors[1]->u8 << g_szrSelectors[2]->u8;
  }break;
  case iU8_ShiftRight:{
    g_szrSelectors[0]->u8 = g_szrSelectors[1]->u8 >> g_szrSelectors[2]->u8;
  }break;

  case iU16_BitAnd:{
    g_szrSelectors[0]->u16 = g_szrSelectors[1]->u16 & g_szrSelectors[2]->u16;
  }break;
  case iU16_BitOr:{
    g_szrSelectors[0]->u16 = g_szrSelectors[1]->u16 | g_szrSelectors[2]->u16;
  }break;
  case iU16_BitNot:{
    g_szrSelectors[0]->u16 = ~g_szrSelectors[1]->u16;
  }break;
  case iU16_Xor:{
    g_szrSelectors[0]->u16 = g_szrSelectors[1]->u16 ^ g_szrSelectors[2]->u16;
  }break;
  case iU16_ShiftLeft:{
    g_szrSelectors[0]->u16 = g_szrSelectors[1]->u16 << g_szrSelectors[2]->u16;
  }break;
  case iU16_ShiftRight:{
    g_szrSelectors[0]->u16 = g_szrSelectors[1]->u16 >> g_szrSelectors[2]->u16;
  }break;

  case iU32_BitAnd:{
    g_szrSelectors[0]->u32 = g_szrSelectors[1]->u32 & g_szrSelectors[2]->u32;
  }break;
  case iU32_BitOr:{
    g_szrSelectors[0]->u32 = g_szrSelectors[1]->u32 | g_szrSelectors[2]->u32;
  }break;
  case iU32_BitNot:{
    g_szrSelectors[0]->u32 = ~g_szrSelectors[1]->u32;
  }break;
  case iU32_Xor:{
    g_szrSelectors[0]->u32 = g_szrSelectors[1]->u32 ^ g_szrSelectors[2]->u32;
  }break;
  case iU32_ShiftLeft:{
    g_szrSelectors[0]->u32 = g_szrSelectors[1]->u32 << g_szrSelectors[2]->u32;
  }break;
  case iU32_ShiftRight:{
    g_szrSelectors[0]->u32 = g_szrSelectors[1]->u32 >> g_szrSelectors[2]->u32;
  }break;

  case iU64_BitAnd:{
    g_szrSelectors[0]->u64 = g_szrSelectors[1]->u64 & g_szrSelectors[2]->u64;
  }break;
  case iU64_BitOr:{
    g_szrSelectors[0]->u64 = g_szrSelectors[1]->u64 | g_szrSelectors[2]->u64;
  }break;
  case iU64_BitNot:{
    g_szrSelectors[0]->u64 = ~g_szrSelectors[1]->u64;
  }break;
  case iU64_Xor:{
    g_szrSelectors[0]->u64 = g_szrSelectors[1]->u64 ^ g_szrSelectors[2]->u64;
  }break;
  case iU64_ShiftLeft:{
    g_szrSelectors[0]->u64 = g_szrSelectors[1]->u64 << g_szrSelectors[2]->u64;
  }break;
  case iU64_ShiftRight:{
    g_szrSelectors[0]->u64 = g_szrSelectors[1]->u64 >> g_szrSelectors[2]->u64;
  }break;

  case iI8_Add:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 + g_szrSelectors[2]->i8;
  }break;
  case iI8_Sub:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 - g_szrSelectors[2]->i8;
  }break;
  case iI8_Mul:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 * g_szrSelectors[2]->i8;
  }break;
  case iI8_Div:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 / g_szrSelectors[2]->i8;
  }break;
  case iI8_Mod:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 % g_szrSelectors[2]->i8;
  }break;
  case iI8_ShiftRight:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 >> g_szrSelectors[2]->u8;
  }break;

  case iI16_Add:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 + g_szrSelectors[2]->i16;
  }break;
  case iI16_Sub:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 - g_szrSelectors[2]->i16;
  }break;
  case iI16_Mul:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 * g_szrSelectors[2]->i16;
  }break;
  case iI16_Div:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 / g_szrSelectors[2]->i16;
  }break;
  case iI16_Mod:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 % g_szrSelectors[2]->i16;
  }break;
  case iI16_ShiftRight:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 >> g_szrSelectors[2]->u16;
  }break;

  case iI32_Add:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 + g_szrSelectors[2]->i32;
  }break;
  case iI32_Sub:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 - g_szrSelectors[2]->i32;
  }break;
  case iI32_Mul:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 * g_szrSelectors[2]->i32;
  }break;
  case iI32_Div:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 / g_szrSelectors[2]->i32;
  }break;
  case iI32_Mod:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 % g_szrSelectors[2]->i32;
  }break;
  case iI32_ShiftRight:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i32 >> g_szrSelectors[2]->u32;
  }break;

    //  case iI64_AddRegisters:{
    //    const U8 *pE = pI + 3;
    //    if(pE > pEnd)
    //      goto UNEXPECTED_END;

    //    U8 uResult = *pI++;
    //    I64 iLeft = g_szRegisters[*pI++].i64;
    //    g_szRegisters[uResult].i64 = iLeft + g_szRegisters[*pI].i64;
    //    pI = pE;
    //  }break;
    //  case iI64_SubRegisters:{
    //    const U8 *pE = pI + 3;
    //    if(pE > pEnd)
    //      goto UNEXPECTED_END;

    //    U8 uResult = *pI++;
    //    I64 iLeft = g_szRegisters[*pI++].i64;
    //    g_szRegisters[uResult].i64 = iLeft - g_szRegisters[*pI].i64;
    //    pI = pE;
    //  }break;
    //  case iI64_MulRegisters:{
    //    const U8 *pE = pI + 3;
    //    if(pE > pEnd)
    //      goto UNEXPECTED_END;

    //    U8 uResult = *pI++;
    //    I64 iLeft = g_szRegisters[*pI++].i64;
    //    g_szRegisters[uResult].i64 = iLeft * g_szRegisters[*pI].i64;
    //    pI = pE;
    //  }break;
    //  case iI64_DivRegisters:{
    //    const U8 *pE = pI + 3;
    //    if(pE > pEnd)
    //      goto UNEXPECTED_END;

    //    U8 uResult = *pI++;
    //    I64 iLeft = g_szRegisters[*pI++].i64;
    //    g_szRegisters[uResult].i64 = iLeft / g_szRegisters[*pI].i64;
    //    pI = pE;
    //  }break;
    //  case iI64_ModRegisters:{
    //    const U8 *pE = pI + 3;
    //    if(pE > pEnd)
    //      goto UNEXPECTED_END;

    //    U8 uResult = *pI++;
    //    I64 iLeft = g_szRegisters[*pI++].i64;
    //    g_szRegisters[uResult].i64 = iLeft % g_szRegisters[*pI].i64;
    //    pI = pE;
    //  }break;

  case iI64_Add:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 + g_szrSelectors[2]->i64;
  }break;
  case iI64_Sub:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 - g_szrSelectors[2]->i64;
  }break;
  case iI64_Mul:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 * g_szrSelectors[2]->i64;
  }break;
  case iI64_Div:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 / g_szrSelectors[2]->i64;
  }break;
  case iI64_Mod:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 % g_szrSelectors[2]->i64;
  }break;
  case iI64_ShiftRight:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i64 >> g_szrSelectors[2]->u64;
  }break;
  case iI64_Equal:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 == g_szrSelectors[2]->i64);
  }break;
  case iI64_NotEqual:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 != g_szrSelectors[2]->i64);
  }break;
  case iI64_Less:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 < g_szrSelectors[2]->i64);
  }break;
  case iI64_LessEqual:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 <= g_szrSelectors[2]->i64);
  }break;
  case iI64_Greater:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 > g_szrSelectors[2]->i64);
  }break;
  case iI64_GreaterEqual:{
    g_szrSelectors[0]->bTrue = (g_szrSelectors[1]->i64 >= g_szrSelectors[2]->i64);
  }break;
  case iI64_If:{
    g_szrSelectors[0]->i64 = (g_szrSelectors[1]->bTrue? g_szrSelectors[2]->i64 : g_szrSelectors[3]->i64);
  }break;
  case iI64_In:{
    I64 iValue = g_szrSelectors[1]->i64;
    g_szrSelectors[0]->bTrue = (iValue >= g_szrSelectors[2]->i64 && iValue < g_szrSelectors[3]->i64);
  }break;

  case iF32_Add:{
    g_szrSelectors[0]->f32 = g_szrSelectors[1]->f32 + g_szrSelectors[2]->f32;
  }break;
  case iF32_Sub:{
    g_szrSelectors[0]->f32 = g_szrSelectors[1]->f32 - g_szrSelectors[2]->f32;
  }break;
  case iF32_Mul:{
    g_szrSelectors[0]->f32 = g_szrSelectors[1]->f32 * g_szrSelectors[2]->f32;
  }break;
  case iF32_Div:{
    g_szrSelectors[0]->f32 = g_szrSelectors[1]->f32 / g_szrSelectors[2]->f32;
  }break;
  case iF32_If:{
    g_szrSelectors[0]->f32 = (g_szrSelectors[1]->bTrue? g_szrSelectors[2]->f32 : g_szrSelectors[3]->f32);
  }break;
  case iF32_In:{
    F32 fValue = g_szrSelectors[1]->f32;
    g_szrSelectors[0]->bTrue = (fValue >= g_szrSelectors[2]->f32 && fValue < g_szrSelectors[3]->f32);
  }break;

  case iF64_Add:{
    g_szrSelectors[0]->f64 = g_szrSelectors[1]->f64 + g_szrSelectors[2]->f64;
  }break;
  case iF64_Sub:{
    g_szrSelectors[0]->f64 = g_szrSelectors[1]->f64 - g_szrSelectors[2]->f64;
  }break;
  case iF64_Mul:{
    g_szrSelectors[0]->f64 = g_szrSelectors[1]->f64 * g_szrSelectors[2]->f64;
  }break;
  case iF64_Div:{
    g_szrSelectors[0]->f64 = g_szrSelectors[1]->f64 / g_szrSelectors[2]->f64;
  }break;
  case iF64_If:{
    g_szrSelectors[0]->f64 = (g_szrSelectors[1]->bTrue? g_szrSelectors[2]->f64 : g_szrSelectors[3]->f64);
  }break;
  case iF64_In:{
    F64 fValue = g_szrSelectors[1]->f64;
    g_szrSelectors[0]->bTrue = (fValue >= g_szrSelectors[2]->f64 && fValue < g_szrSelectors[3]->f64);
  }break;

  case iI8ToI16:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i8;
  }break;
  case iI8ToI32:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i8;
  }break;
  case iI8ToI64:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i8;
  }break;
  case iI8ToF32:{
    g_szrSelectors[0]->f32 = F32(g_szrSelectors[1]->i8);
  }break;
  case iI8ToF64:{
    g_szrSelectors[0]->f64 = F64(g_szrSelectors[1]->i8);
  }break;
  case iI16ToI8:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i16;
  }break;
  case iI16ToI32:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i16;
  }break;
  case iI16ToI64:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i16;
  }break;
  case iI16ToF32:{
    g_szrSelectors[0]->f32 = F32(g_szrSelectors[1]->i16);
  }break;
  case iI16ToF64:{
    g_szrSelectors[0]->f64 = F64(g_szrSelectors[1]->i16);
  }break;
  case iI32ToI8:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i32;
  }break;
  case iI32ToI16:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i32;
  }break;
  case iI32ToI64:{
    g_szrSelectors[0]->i64 = g_szrSelectors[1]->i32;
  }break;
  case iI32ToF32:{
    g_szrSelectors[0]->f32 = F32(g_szrSelectors[1]->i32);
  }break;
  case iI32ToF64:{
    g_szrSelectors[0]->f64 = F64(g_szrSelectors[1]->i32);
  }break;
  case iI64ToI8:{
    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i64;
  }break;
  case iI64ToI16:{
    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i64;
  }break;
  case iI64ToI32:{
    g_szrSelectors[0]->i32 = g_szrSelectors[1]->i64;
  }break;
  case iI64ToF32:{
    g_szrSelectors[0]->f32 = F32(g_szrSelectors[1]->i64);
  }break;
  case iI64ToF64:{
    g_szrSelectors[0]->f64 = F64(g_szrSelectors[1]->i64);
  }break;
  case iF32ToI32:{
    g_szrSelectors[0]->i32 = I32(g_szrSelectors[1]->f32);
  }break;
  case iF64ToI64:{
    g_szrSelectors[0]->i64 = I64(g_szrSelectors[1]->f64);
  }break;
  case iOpen:{
    g_szrSelectors[0]->i32 = open((char*)g_szrSelectors[1]->ptr, g_szrSelectors[2]->i32, g_szrSelectors[3]->u32);
  }break;
  case iClose:{
    close(g_szrSelectors[0]->i32);
  }break;
  case iInput:{
    g_szrSelectors[0]->i64 = read(g_szrSelectors[1]->i32, g_szrSelectors[2]->ptr, g_szrSelectors[3]->u64);
    //    printf("%ld: %.*s\n", size, (int)size, (char*)g_szBuffer);
  }break;
  case iOutput:{
    g_szrSelectors[0]->i64 = write(g_szrSelectors[1]->i32, g_szrSelectors[2]->ptr, g_szrSelectors[3]->u64);
    //    printf("Wrote: %ld\n", size);
  }break;
  case iAllocate:{
    g_szrSelectors[0]->ptr = (Byte*)malloc(g_szrSelectors[1]->u64);
  }break;
  case iFree:{
    free(g_szrSelectors[0]->ptr);
  }break;
  case iByte_Skip:{
    Byte uValue = g_szrSelectors[2]->u8;
    Byte *p = g_szrSelectors[1]->ptr;
    while(*p == uValue)
      ++p;

    g_szrSelectors[0]->ptr = p;
  }break;
  case iByte_SkipUntil:{
    Byte uValue = g_szrSelectors[2]->u8;
    Byte *p = g_szrSelectors[1]->ptr;
    while(*p != uValue)
      ++p;

    g_szrSelectors[0]->ptr = p;
  }break;
  case iByte_SkipIn:{
    Byte uBegin = g_szrSelectors[2]->u8;
    Byte uEnd = g_szrSelectors[3]->u8;
    Byte *p = g_szrSelectors[1]->ptr;
    Byte uValue;
    while(1){
      uValue = *p;
      if(uValue < uBegin || uValue >= uEnd)
        break;
      ++p;
    }

    g_szrSelectors[0]->ptr = p;
  }break;
  case iMap_New:{
    //    g_szrSelectors[0]->ptr = (Byte*)new Map((MapComparer)g_szrSelectors[1]->ptr);
    g_szrSelectors[0]->ptr = (Byte*)new Map();
  }break;
  case iMap_Delete:{
    delete (Map*)g_szrSelectors[0]->ptr;
  }break;
  case iMap_Add:{
    Map *pMap = (Map*)g_szrSelectors[1]->ptr;
    g_szrSelectors[0]->bTrue = pMap->Add(g_szrSelectors[2]->ptr, g_szrSelectors[3]->u32, g_szrSelectors[4]->u64);
  }break;
  case iMap_Remove:{
    Map *pMap = (Map*)g_szrSelectors[1]->ptr;
    g_szrSelectors[0]->bTrue = pMap->Remove(g_szrSelectors[2]->ptr, g_szrSelectors[3]->u32);
  }break;
  case iMap_Find:{
    Map *pMap = (Map*)g_szrSelectors[1]->ptr;
    g_szrSelectors[0]->bTrue = pMap->Find(g_szrSelectors[2]->ptr, g_szrSelectors[3]->u32);
  }break;
  }

  *ppBegin = pI;
  return 0;
UNEXPECTED_END:
  fprintf(stderr, "Error: Unexpected instruction end!\n");
  return 1;
}

