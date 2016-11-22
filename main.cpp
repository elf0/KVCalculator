//License: Public Domain
//Author: elf
//EMail: elf@elf0.org

#include <stdio.h>
#include <map>
#include <string>

#include "KVReader.h"
#include "MMFile.h"
#include "String2U.h"

struct String{
  String(){}
  String(const C *p, std::size_t s): pBegin(p), uSize(s){}

  bool operator<(const String &right)const{
    std::size_t uMin = uSize > right.uSize? right.uSize : uSize;
    const C *pL = pBegin;
    const C *pEnd = pL + uMin;

    const C *pR = right.pBegin;
    while(pL != pEnd){
      if(*pL < *pR)
        return true;

      if(*pL++ > *pR++)
        return false;
    }

    return uSize < right.uSize;
  }

  const C *pBegin = nullptr;
  std::size_t uSize = 0;
};

//rational number
struct Value{
  Value(){}
  Value(I64 n): iNumerator(n){}
  Value(I64 n, I64 d): iNumerator(n), iDenominator(d){}

  bool operator==(const Value &right)const{
    return (iNumerator == right.iNumerator) && (iDenominator == right.iDenominator);
  }

  void operator+=(const Value &right){
    if(iDenominator == right.iDenominator)
      iNumerator += right.iNumerator;
    else{
      iNumerator = iNumerator * right.iDenominator + right.iNumerator * iDenominator;
      iDenominator *= right.iDenominator;
    }

    if(iDenominator != 1){
      if(iNumerator % iDenominator == 0){
        iNumerator /= iDenominator;
        iDenominator = 1;
      }
    }
  }

  void operator-=(const Value &right){
    if(iDenominator == right.iDenominator)
      iNumerator -= right.iNumerator;
    else{
      iNumerator = iNumerator * right.iDenominator - right.iNumerator * iDenominator;
      iDenominator *= right.iDenominator;
    }

    if(iDenominator != 1){
      if(iNumerator % iDenominator == 0){
        iNumerator /= iDenominator;
        iDenominator = 1;
      }
    }
  }

  void operator*=(const Value &right){
    if(iDenominator == right.iDenominator){
      if(iDenominator == 1)
        iNumerator *= right.iNumerator;
      else{
        if(iNumerator % iDenominator == 0)
          iNumerator = iNumerator / iDenominator * right.iNumerator;
        else if(right.iNumerator % iDenominator == 0){
          iNumerator *= right.iNumerator / iDenominator;
        }
        else{
          iNumerator *= right.iNumerator;
          iDenominator *= iDenominator;
        }
      }
    }
    else{
      if(iNumerator % right.iDenominator == 0)
        iNumerator = iNumerator / right.iDenominator * right.iNumerator;
      else if(right.iNumerator % iDenominator == 0){
        iNumerator *= right.iNumerator / iDenominator;
        iDenominator = right.iDenominator;
      }
      else{
        iNumerator *= right.iNumerator;
        iDenominator *= right.iDenominator;
      }
    }

    if(iDenominator != 1){
      if(iNumerator % iDenominator == 0){
        iNumerator /= iDenominator;
        iDenominator = 1;
      }
    }
  }

  void operator/=(const Value &right){
    if(iDenominator == right.iDenominator)
      iDenominator = right.iNumerator;
    else{
      if(iNumerator % right.iNumerator == 0)
        iNumerator = iNumerator / right.iNumerator * right.iDenominator;
      else if(right.iDenominator % iDenominator == 0){
        iNumerator *= right.iDenominator / iDenominator;
        iDenominator = right.iNumerator;
      }
      else{
        iNumerator *= right.iDenominator;
        iDenominator *= right.iNumerator;
      }
    }

    if(iDenominator != 1){
      if(iNumerator % iDenominator == 0){
        iNumerator /= iDenominator;
        iDenominator = 1;
      }
    }
  }

  I64 iNumerator = 0;
  I64 iDenominator = 1;
};

typedef std::map<String, Value> Variables;
static Variables g_variables;
#define MAX_ARGUMENTS 16
static String g_szArguments[MAX_ARGUMENTS];
static String *g_pArgumentsEnd = g_szArguments + MAX_ARGUMENTS;
static U64 g_uLine;

static E8 onStatement(void *pContext, const C *pKey, const C *pKeyEnd, const C *pValue, const C *pValueEnd);

inline
static E8 ParseExpression(const C *pBegin, const C *pEnd, Value &value);

inline
static E8 Call(const String &strName, const String *szArguments, U8 uArguments, Value &value);

inline
static E8 ParseArgument(const String &strArgument, Value &value);

int main(int argc, char *argv[]){
  if(argc != 2)
    return 1;

  MMFile file;
  if(MMFile_OpenForRead(&file, (C*)argv[1]))
    return 2;

  g_uLine = 0;
  Byte *pBegin = file.pBegin;
  Byte *pEnd = pBegin + file.file.meta.st_size;
  E8 e = KVReader_Parse(0, pBegin, pEnd, onStatement);


  for(auto &pr: g_variables){
    const String &key = pr.first;
    const Value &value = pr.second;
    if(value.iDenominator == 1)
      fprintf(stderr, "%.*s: %lld\n", (int)(key.uSize), (char*)key.pBegin, value.iNumerator);
    else
      fprintf(stderr, "%.*s: %lld/%lld\n", (int)(key.uSize), (char*)key.pBegin, value.iNumerator, value.iDenominator);
  }

  MMFile_Close(&file);
  return e;
}

static E8 onStatement(void *pContext, const C *pKey, const C *pKeyEnd, const C *pValue, const C *pValueEnd){
  ++g_uLine;
  //fprintf(stderr, "%llu: \"%.*s: %.*s\"\n", g_uLine, (int)(pKeyEnd - pKey), pKey, (int)(pValueEnd - pValue), pValue);
  Value vNew;
  E8 e = ParseExpression(pValue, pValueEnd, vNew);
  if(e)
    return e;

  auto pr = g_variables.emplace(String(pKey, pKeyEnd - pKey), vNew);
  if(pr.second){
  }
  else{//Found
    Value &vOld = pr.first->second;
    if(vNew.iNumerator != vOld.iNumerator){
      //      fprintf(stderr, "%.*s(%llu) = %llu\n"
      //              , (int)(pKeyEnd - pKey), pKey
      //              , vOld.iNumerator
      //              , vNew.iNumerator);
      vOld = vNew;
    }
  }
  return 0;
}

inline
static E8 ParseExpression(const C *pBegin, const C *pEnd, Value &value){
  String *pArgument = g_szArguments;
  const C *p = pBegin;
  while(*p++ == 0x20){
    if(pArgument == g_pArgumentsEnd){
      fprintf(stderr, "Error(%llu): Too much arguments!\n", g_uLine);
      return 1;
    }

    pArgument->pBegin = p;
    while(*p != 0x20 && *p != '\n')
      ++p;
    pArgument->uSize = p - pArgument->pBegin;
    ++pArgument;
    if(*p == '\n')
      break;
  }

  U8 uArguments = pArgument - g_szArguments;
  //  fprintf(stderr, "%llu: %u arguments\n", g_uLine, uArguments);

  if(uArguments){
    pArgument = g_szArguments;
    const C *p = pArgument->pBegin;
    E8 e;
    if(uArguments == 1)
      e = ParseArgument(*pArgument, value);
    else
      e = Call(*pArgument, pArgument + 1, uArguments - 1, value);

    if(e)
      return e;
  }
  else{
    fprintf(stderr, "Error(%llu): Empty definition!\n", g_uLine);
    return 1;
  }
  return 0;
}

inline
static E8 Call(const String &strName, const String *szArguments, U8 uArguments, Value &value){
  E8 e;
  const String *pArgument = szArguments;
  const String *pArgumentEnd = pArgument + uArguments;
  Value vLeft, vRight;
  const C *p = strName.pBegin;
  switch(*p){
  default:
    fprintf(stderr, "Error(%llu): \"%.*s\" not defined!\n", g_uLine, (int)strName.uSize, (char*)strName.pBegin);
    return 1;
  case '+':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    while(pArgument != pArgumentEnd){
      e = ParseArgument(*pArgument++, vRight);
      if(e)
        return e;

      vLeft += vRight;
    }

    value = vLeft;
  }break;
  case '-':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    while(pArgument != pArgumentEnd){
      e = ParseArgument(*pArgument++, vRight);
      if(e)
        return e;
      vLeft -= vRight;
    }

    value = vLeft;
  }break;
  case '*':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    while(pArgument != pArgumentEnd){
      e = ParseArgument(*pArgument++, vRight);
      if(e)
        return e;
      vLeft *= vRight;
    }

    value = vLeft;
  }break;
  case '/':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    while(pArgument != pArgumentEnd){
      e = ParseArgument(*pArgument++, vRight);
      if(e)
        return e;
      vLeft /= vRight;
    }

    value = vLeft;
  }break;
  case '=':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    bool bCompare;
    while(pArgument != pArgumentEnd){
      e = ParseArgument(*pArgument++, vRight);
      if(e)
        return e;
      bCompare = vLeft == vRight;
      if(!bCompare)
        break;
    }

    value.iNumerator = bCompare;
    value.iDenominator = 1;
  }break;
  case '<':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }
    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    e = ParseArgument(*pArgument++, vRight);
    if(e)
      return e;

    bool bCompare;
    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = ParseArgument(*pArgument++, vRight);
        if(e)
          return e;

        bCompare = vLeft.iNumerator <= vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }
    else{
      while(pArgument != pArgumentEnd){
        e = ParseArgument(*pArgument++, vRight);
        if(e)
          return e;

        bCompare = vLeft.iNumerator < vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }

    value.iNumerator = bCompare;
    value.iDenominator = 1;
  }break;
  case '>':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }
    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    e = ParseArgument(*pArgument++, vRight);
    if(e)
      return e;

    bool bCompare;
    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = ParseArgument(*pArgument++, vRight);
        if(e)
          return e;

        bCompare = vLeft.iNumerator >= vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }
    else{
      while(pArgument != pArgumentEnd){
        e = ParseArgument(*pArgument++, vRight);
        if(e)
          return e;

        bCompare = vLeft.iNumerator > vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }

    value.iNumerator = bCompare;
    value.iDenominator = 1;
  }break;
  case '?':{
    if(uArguments != 3){
      fprintf(stderr, "Error(%llu): Argument count must be 3, but %u!\n", g_uLine, uArguments);
      return 1;
    }
    e = ParseArgument(*pArgument++, vLeft);
    if(e)
      return e;

    e = ParseArgument(*pArgument++, vRight);
    if(e)
      return e;

    Value vFalse;
    e = ParseArgument(*pArgument++, vFalse);
    if(e)
      return e;

    value = vLeft.iNumerator? vRight : vFalse;
  }break;
  }

  return 0;
}

inline
static E8 ParseArgument(const String &strArgument, Value &value){
  const C *pBegin = strArgument.pBegin;
  const C *pEnd = pBegin + strArgument.uSize;
  B bSigned = 0;
  if(*pBegin == '-'){
    ++pBegin;
    bSigned = 1;
  }

  if(C_IsDigit(*pBegin)){
    const C *p = pBegin;
    I64 iValue = *p++ - '0';
    if(String_ParseU64(&p, (U64*)&iValue) || iValue < 0){
      fprintf(stderr, "Error(%llu): Integer(%.*s) overflowed!\n", g_uLine, (int)strArgument.uSize, (char*)strArgument.pBegin);
      return 1;
    }

    if(p != pEnd){
      fprintf(stderr, "Error(%llu): Invalid tail \"%.*s\"!\n", g_uLine, (int)(pEnd - p), (char*)p);
      return 1;
    }

    value.iNumerator = iValue;
    value.iDenominator = 1;
  }
  else{
    std::size_t uSize = pEnd - pBegin;
    auto iter = g_variables.find(String(pBegin, uSize));
    if(iter == g_variables.end()){
      fprintf(stderr, "Error(%llu): \"%.*s\" not defined!\n", g_uLine, (int)uSize, (char*)pBegin);
      return 1;
    }

    value = iter->second;
  }

  if(bSigned)
    value.iNumerator = -value.iNumerator;

  return 0;
}
