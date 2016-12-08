//License: Public Domain
//Author: elf
//EMail: elf@elf0.org

#include <cstdio>
#include <cstring>
#include <map>

#include "MMFile.h"
#include "vmi.h"

//CPU definition
typedef struct{
  union{
    Byte byte;
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

#define REGISTERS_COUNT 256
static Register g_szRegisters[REGISTERS_COUNT];

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
static E8 ExecuteInstruction(const Byte **ppBegin, const Byte **ppEnd);

inline
static E8 Execute(const Byte *pBegin, const Byte *pEnd, const Byte *pData, const Byte *pDataEnd){
  E8 e;
  const U8 *pI = pBegin;
  while(pI != pEnd){
    e = ExecuteInstruction(&pI, &pEnd);
    if(e)
      return e;
  }

  return 0;
}

inline
static E8 ExecuteInstruction(const Byte **ppBegin, const Byte **ppEnd){
  E8 e = 0;
  const Byte *pEnd = *ppEnd;
  const Byte *pI = *ppBegin;

  switch(*pI++){
  default:
    fprintf(stderr, "Error: Invalid instruction \"%d\"!\n", pI[-1]);
    return 1;
  case iExit:{
    if(pI == pEnd)
      goto UNEXPECTED_END;

    e = g_szRegisters[*pI++].e8;
    *ppEnd = pI;
    return e;
  }break;
  case iZero:{
    if(pI == pEnd)
      goto UNEXPECTED_END;

    g_szRegisters[*pI++].u64 = 0;
  }break;
  case iLoad8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uValue = *(U8*)pI++;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoad16:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uValue = *(U16*)pI;
    pI += 2;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoad32:{
    const Byte *pE = pI + 5;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uValue = *(U32*)pI;
    pI += 4;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoad64:{
    const Byte *pE = pI + 9;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uValue = *(U64*)pI;
    pI += 8;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoadM8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uValue = *(U8*)g_szRegisters[*pI++].ptr;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoadM16:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uValue = *(U16*)g_szRegisters[*pI++].ptr;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoadM32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uValue = *(U32*)g_szRegisters[*pI++].ptr;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;
  case iLoadM64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uValue = *(U64*)g_szRegisters[*pI++].ptr;
    g_szRegisters[*pI].u64 = uValue;
    pI = pE;
  }break;

  case iSaveM8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uValue = g_szRegisters[*pI++].u8;
    *(U8*)g_szRegisters[*pI].ptr = uValue;
    pI = pE;
  }break;
  case iSaveM16:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uValue = g_szRegisters[*pI++].u16;
    *(U16*)g_szRegisters[*pI].ptr = uValue;
    pI = pE;
  }break;
  case iSaveM32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uValue = g_szRegisters[*pI++].u32;
    *(U32*)g_szRegisters[*pI].ptr = uValue;
    pI = pE;
  }break;
  case iSaveM64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uValue = g_szRegisters[*pI++].u64;
    *(U64*)g_szRegisters[*pI].ptr = uValue;
    pI = pE;
  }break;

  case iAnd:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bLeft = g_szRegisters[*pI++].bTrue;
    B bRight = g_szRegisters[*pI++].bTrue;
    g_szRegisters[*pI].bTrue = bLeft && bRight;
    pI = pE;
  }break;
  case iOr:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bLeft = g_szRegisters[*pI++].bTrue;
    B bRight = g_szRegisters[*pI++].bTrue;
    g_szRegisters[*pI].bTrue = bLeft || bRight;
    pI = pE;
  }break;
  case iNot:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bValue = g_szRegisters[*pI++].bTrue;
    g_szRegisters[*pI].bTrue = !bValue;
    pI = pE;
  }break;
  case iU8_BitAnd:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uLeft = g_szRegisters[*pI++].u8;
    U8 uRight = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = uLeft & uRight;
    pI = pE;
  }break;
  case iU8_BitOr:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uLeft = g_szRegisters[*pI++].u8;
    U8 uRight = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = uLeft | uRight;
    pI = pE;
  }break;
  case iU8_BitNot:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uValue = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = ~uValue;
    pI = pE;
  }break;
  case iU8_Xor:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uLeft = g_szRegisters[*pI++].u8;
    U8 uRight = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = uLeft ^ uRight;
    pI = pE;
  }break;
  case iU8_ShiftLeft:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uLeft = g_szRegisters[*pI++].u8;
    U8 uRight = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = uLeft << uRight;
    pI = pE;
  }break;
  case iU8_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U8 uLeft = g_szRegisters[*pI++].u8;
    U8 uRight = g_szRegisters[*pI++].u8;
    g_szRegisters[*pI].u8 = uLeft >> uRight;
    pI = pE;
  }break;

  case iU16_BitAnd:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uLeft = g_szRegisters[*pI++].u16;
    U16 uRight = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = uLeft & uRight;
    pI = pE;
  }break;
  case iU16_BitOr:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uLeft = g_szRegisters[*pI++].u16;
    U16 uRight = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = uLeft | uRight;
    pI = pE;
  }break;
  case iU16_BitNot:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uValue = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = ~uValue;
    pI = pE;
  }break;
  case iU16_Xor:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uLeft = g_szRegisters[*pI++].u16;
    U16 uRight = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = uLeft ^ uRight;
    pI = pE;
  }break;
  case iU16_ShiftLeft:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uLeft = g_szRegisters[*pI++].u16;
    U16 uRight = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = uLeft << uRight;
    pI = pE;
  }break;
  case iU16_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U16 uLeft = g_szRegisters[*pI++].u16;
    U16 uRight = g_szRegisters[*pI++].u16;
    g_szRegisters[*pI].u16 = uLeft >> uRight;
    pI = pE;
  }break;

  case iU32_BitAnd:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uLeft = g_szRegisters[*pI++].u32;
    U32 uRight = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = uLeft & uRight;
    pI = pE;
  }break;
  case iU32_BitOr:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uLeft = g_szRegisters[*pI++].u32;
    U32 uRight = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = uLeft | uRight;
    pI = pE;
  }break;
  case iU32_BitNot:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uValue = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = ~uValue;
    pI = pE;
  }break;
  case iU32_Xor:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uLeft = g_szRegisters[*pI++].u32;
    U32 uRight = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = uLeft ^ uRight;
    pI = pE;
  }break;
  case iU32_ShiftLeft:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uLeft = g_szRegisters[*pI++].u32;
    U32 uRight = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = uLeft << uRight;
    pI = pE;
  }break;
  case iU32_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U32 uLeft = g_szRegisters[*pI++].u32;
    U32 uRight = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].u32 = uLeft >> uRight;
    pI = pE;
  }break;

  case iU64_BitAnd:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uLeft = g_szRegisters[*pI++].u64;
    U64 uRight = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = uLeft & uRight;
    pI = pE;
  }break;
  case iU64_BitOr:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uLeft = g_szRegisters[*pI++].u64;
    U64 uRight = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = uLeft | uRight;
    pI = pE;
  }break;
  case iU64_BitNot:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uValue = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = ~uValue;
    pI = pE;
  }break;
  case iU64_Xor:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uLeft = g_szRegisters[*pI++].u64;
    U64 uRight = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = uLeft ^ uRight;
    pI = pE;
  }break;
  case iU64_ShiftLeft:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uLeft = g_szRegisters[*pI++].u64;
    U64 uRight = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = uLeft << uRight;
    pI = pE;
  }break;
  case iU64_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uLeft = g_szRegisters[*pI++].u64;
    U64 uRight = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].u64 = uLeft >> uRight;
    pI = pE;
  }break;

    //  case iI8_Add:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 + g_szrSelectors[2]->i8;
    //  }break;
    //  case iI8_Sub:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 - g_szrSelectors[2]->i8;
    //  }break;
    //  case iI8_Mul:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 * g_szrSelectors[2]->i8;
    //  }break;
    //  case iI8_Div:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 / g_szrSelectors[2]->i8;
    //  }break;
    //  case iI8_Mod:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 % g_szrSelectors[2]->i8;
    //  }break;
    //  case iI8_ShiftRight:{
    //    g_szrSelectors[0]->i8 = g_szrSelectors[1]->i8 >> g_szrSelectors[2]->u8;
    //  }break;

    //  case iI16_Add:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 + g_szrSelectors[2]->i16;
    //  }break;
    //  case iI16_Sub:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 - g_szrSelectors[2]->i16;
    //  }break;
    //  case iI16_Mul:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 * g_szrSelectors[2]->i16;
    //  }break;
    //  case iI16_Div:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 / g_szrSelectors[2]->i16;
    //  }break;
    //  case iI16_Mod:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 % g_szrSelectors[2]->i16;
    //  }break;
    //  case iI16_ShiftRight:{
    //    g_szrSelectors[0]->i16 = g_szrSelectors[1]->i16 >> g_szrSelectors[2]->u16;
    //  }break;

  case iI32_Add:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft + iRight;
    pI = pE;
  }break;
  case iI32_Sub:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft - iRight;
    pI = pE;
  }break;
  case iI32_Mul:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft * iRight;
    pI = pE;
  }break;
  case iI32_Div:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft / iRight;
    pI = pE;
  }break;
  case iI32_Mod:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft % iRight;
    pI = pE;
  }break;
  case iI32_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iLeft = g_szRegisters[*pI++].i32;
    I32 iRight = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i32 = iLeft >> iRight;
    pI = pE;
  }break;
  case iI64_Add:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft + iRight;
    pI = pE;
  }break;
  case iI64_Sub:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft - iRight;
    pI = pE;
  }break;
  case iI64_Mul:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft * iRight;
    pI = pE;
  }break;
  case iI64_Div:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft / iRight;
    pI = pE;
  }break;
  case iI64_Mod:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft % iRight;
    pI = pE;
  }break;
  case iI64_ShiftRight:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = iLeft >> iRight;
    pI = pE;
  }break;
  case iI64_Equal:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft == iRight;
    pI = pE;
  }break;
  case iI64_NotEqual:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft != iRight;
    pI = pE;
  }break;
  case iI64_Less:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft < iRight;
    pI = pE;
  }break;
  case iI64_LessEqual:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft <= iRight;
    pI = pE;
  }break;
  case iI64_Greater:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft > iRight;
    pI = pE;
  }break;
  case iI64_GreaterEqual:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iLeft = g_szRegisters[*pI++].i64;
    I64 iRight = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].bTrue = iLeft >= iRight;
    pI = pE;
  }break;
  case iI64_If:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bCondition = g_szRegisters[*pI++].bTrue;
    I64 iTrue = g_szRegisters[*pI++].i64;
    I64 iFalse = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = bCondition? iTrue : iFalse;
    pI = pE;
  }break;
  case iI64_In:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    I64 iBegin = g_szRegisters[*pI++].i64;
    I64 iEnd = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i64 = (iValue >= iBegin && iValue < iEnd);
    pI = pE;
  }break;
  case iF32_Add:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fLeft = g_szRegisters[*pI++].f32;
    F32 fRight = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = fLeft + fRight;
    pI = pE;
  }break;
  case iF32_Sub:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fLeft = g_szRegisters[*pI++].f32;
    F32 fRight = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = fLeft - fRight;
    pI = pE;
  }break;
  case iF32_Mul:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fLeft = g_szRegisters[*pI++].f32;
    F32 fRight = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = fLeft * fRight;
    pI = pE;
  }break;
  case iF32_Div:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fLeft = g_szRegisters[*pI++].f32;
    F32 fRight = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = fLeft / fRight;
    pI = pE;
  }break;
  case iF32_If:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bCondition = g_szRegisters[*pI++].bTrue;
    F32 fTrue = g_szRegisters[*pI++].f32;
    F32 fFalse = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = bCondition? fTrue : fFalse;
    pI = pE;
  }break;
  case iF32_In:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fValue = g_szRegisters[*pI++].f32;
    F32 fBegin = g_szRegisters[*pI++].f32;
    F32 fEnd = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].f32 = (fValue >= fBegin && fValue < fEnd);
    pI = pE;
  }break;
  case iF64_Add:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fLeft = g_szRegisters[*pI++].f64;
    F64 fRight = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = fLeft + fRight;
    pI = pE;
  }break;
  case iF64_Sub:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fLeft = g_szRegisters[*pI++].f64;
    F64 fRight = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = fLeft - fRight;
    pI = pE;
  }break;
  case iF64_Mul:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fLeft = g_szRegisters[*pI++].f64;
    F64 fRight = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = fLeft * fRight;
    pI = pE;
  }break;
  case iF64_Div:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fLeft = g_szRegisters[*pI++].f64;
    F64 fRight = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = fLeft / fRight;
    pI = pE;
  }break;
  case iF64_If:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    B bCondition = g_szRegisters[*pI++].bTrue;
    F64 fTrue = g_szRegisters[*pI++].f64;
    F64 fFalse = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = bCondition? fTrue : fFalse;
    pI = pE;
  }break;
  case iF64_In:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fValue = g_szRegisters[*pI++].f64;
    F64 fBegin = g_szRegisters[*pI++].f64;
    F64 fEnd = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].f64 = (fValue >= fBegin && fValue < fEnd);
    pI = pE;
  }break;
  case iI8ToI16:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I8 iValue = g_szRegisters[*pI++].i8;
    g_szRegisters[*pI].i16 = iValue;
    pI = pE;
  }break;
  case iI8ToI32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I8 iValue = g_szRegisters[*pI++].i8;
    g_szRegisters[*pI].i32 = iValue;
    pI = pE;
  }break;
  case iI8ToI64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I8 iValue = g_szRegisters[*pI++].i8;
    g_szRegisters[*pI].i64 = iValue;
    pI = pE;
  }break;
  case iI8ToF32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I8 iValue = g_szRegisters[*pI++].i8;
    g_szRegisters[*pI].f32 = iValue;
    pI = pE;
  }break;
  case iI8ToF64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I8 iValue = g_szRegisters[*pI++].i8;
    g_szRegisters[*pI].f64 = iValue;
    pI = pE;
  }break;
  case iI16ToI8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I16 iValue = g_szRegisters[*pI++].i16;
    g_szRegisters[*pI].i8 = iValue;
    pI = pE;
  }break;
  case iI16ToI32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I16 iValue = g_szRegisters[*pI++].i16;
    g_szRegisters[*pI].i32 = iValue;
    pI = pE;
  }break;
  case iI16ToI64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I16 iValue = g_szRegisters[*pI++].i16;
    g_szRegisters[*pI].i64 = iValue;
    pI = pE;
  }break;
  case iI16ToF32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I16 iValue = g_szRegisters[*pI++].i16;
    g_szRegisters[*pI].f32 = iValue;
    pI = pE;
  }break;
  case iI16ToF64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I16 iValue = g_szRegisters[*pI++].i16;
    g_szRegisters[*pI].f64 = iValue;
    pI = pE;
  }break;
  case iI32ToI8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iValue = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i8 = iValue;
    pI = pE;
  }break;
  case iI32ToI16:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iValue = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i16 = iValue;
    pI = pE;
  }break;
  case iI32ToI64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iValue = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].i64 = iValue;
    pI = pE;
  }break;
  case iI32ToF32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iValue = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].f32 = iValue;
    pI = pE;
  }break;
  case iI32ToF64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iValue = g_szRegisters[*pI++].i32;
    g_szRegisters[*pI].f64 = iValue;
    pI = pE;
  }break;
  case iI64ToI8:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i8 = iValue;
    pI = pE;
  }break;
  case iI64ToI16:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i16 = iValue;
    pI = pE;
  }break;
  case iI64ToI32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].i32 = iValue;
    pI = pE;
  }break;
  case iI64ToF32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].f32 = iValue;
    pI = pE;
  }break;
  case iI64ToF64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I64 iValue = g_szRegisters[*pI++].i64;
    g_szRegisters[*pI].f64 = iValue;
    pI = pE;
  }break;
  case iF32ToI32:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F32 fValue = g_szRegisters[*pI++].f32;
    g_szRegisters[*pI].i32 = fValue;
    pI = pE;
  }break;
  case iF64ToI64:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    F64 fValue = g_szRegisters[*pI++].f64;
    g_szRegisters[*pI].i64 = fValue;
    pI = pE;
  }break;
  case iOpen:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    char *pFileName = (char*)g_szRegisters[*pI++].ptr;
    I32 iFlags = g_szRegisters[*pI++].i32;
    U32 uMod = g_szRegisters[*pI++].u32;

    g_szRegisters[*pI].i32 = open(pFileName, iFlags, uMod);
    pI = pE;
  }break;
  case iClose:{
    if(pI == pEnd)
      goto UNEXPECTED_END;
    close(g_szRegisters[*pI++].i32);
  }break;
  case iInput:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iFile = g_szRegisters[*pI++].i32;
    void *pBuffer = g_szRegisters[*pI++].ptr;
    U64 uSize = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].i64 = read(iFile, pBuffer, uSize);
    //    printf("Read: %lld\n", g_szRegisters[*pI].i64);
    pI = pE;
  }break;
  case iOutput:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    I32 iFile = g_szRegisters[*pI++].i32;
    void *pBuffer = g_szRegisters[*pI++].ptr;
    U64 uSize = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].i64 = write(iFile, pBuffer, uSize);
    //    printf("Wrote: %lld\n", g_szRegisters[*pI].i64);
    pI = pE;
  }break;
  case iAllocate:{
    const Byte *pE = pI + 2;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    U64 uSize = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].ptr = (Byte*)malloc(uSize);
    pI = pE;
  }break;
  case iFree:{
    if(pI == pEnd)
      goto UNEXPECTED_END;
    free(g_szRegisters[*pI++].ptr);
  }break;
  case iCopy:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    void *pDest = g_szRegisters[*pI++].ptr;
    void *pSrc = g_szRegisters[*pI++].ptr;
    memcpy(pDest, pSrc, g_szRegisters[*pI].u64);
    pI = pE;
  }break;
  case iMove:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    void *pDest = g_szRegisters[*pI++].ptr;
    void *pSrc = g_szRegisters[*pI++].ptr;
    memmove(pDest, pSrc, g_szRegisters[*pI].u64);
    pI = pE;
  }break;
  case iByte_Skip:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Byte *p = g_szRegisters[*pI++].ptr;
    Byte value = g_szRegisters[*pI++].byte;
    while(*p == value)
      ++p;

    g_szRegisters[*pI].ptr = p;
    pI = pE;
  }break;
  case iByte_SkipUntil:{
    const Byte *pE = pI + 3;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Byte *p = g_szRegisters[*pI++].ptr;
    Byte value = g_szRegisters[*pI++].byte;
    while(*p != value)
      ++p;

    g_szRegisters[*pI].ptr = p;
    pI = pE;
  }break;
  case iByte_SkipIn:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Byte *p = g_szRegisters[*pI++].ptr;
    Byte begin = g_szRegisters[*pI++].byte;
    Byte end = g_szRegisters[*pI++].byte;
    Byte value;
    while(1){
      value = *p;
      if(value < begin || value >= end)
        break;
      ++p;
    }

    g_szRegisters[*pI].ptr = p;
    pI = pE;
  }break;
  case iMap_New:{
    if(pI == pEnd)
      goto UNEXPECTED_END;
    //    g_szrSelectors[0]->ptr = (Byte*)new Map((MapComparer)g_szrSelectors[1]->ptr);
    g_szRegisters[*pI++].ptr = (Byte*)new Map();
  }break;
  case iMap_Delete:{
    if(pI == pEnd)
      goto UNEXPECTED_END;
    delete (Map*)g_szRegisters[*pI++].ptr;
  }break;
  case iMap_Add:{
    const Byte *pE = pI + 5;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Map *pMap = (Map*)g_szRegisters[*pI++].ptr;
    Byte *pKey = g_szRegisters[*pI++].ptr;
    U32 uKey = g_szRegisters[*pI++].u32;
    U64 uValue = g_szRegisters[*pI++].u64;
    g_szRegisters[*pI].bTrue = pMap->Add(pKey, uKey, uValue);
    pI = pE;
  }break;
  case iMap_Remove:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Map *pMap = (Map*)g_szRegisters[*pI++].ptr;
    Byte *pKey = g_szRegisters[*pI++].ptr;
    U32 uKey = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].bTrue = pMap->Remove(pKey, uKey);
    pI = pE;
  }break;
  case iMap_Find:{
    const Byte *pE = pI + 4;
    if(pE > pEnd)
      goto UNEXPECTED_END;

    Map *pMap = (Map*)g_szRegisters[*pI++].ptr;
    Byte *pKey = g_szRegisters[*pI++].ptr;
    U32 uKey = g_szRegisters[*pI++].u32;
    g_szRegisters[*pI].bTrue = pMap->Find(pKey, uKey);
    pI = pE;
  }break;
  }

  *ppBegin = pI;
  return e;
UNEXPECTED_END:
  fprintf(stderr, "Error: Unexpected instruction end!\n");
  return 1;
}

