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

struct Expression{
  Expression(const C *p, std::size_t s): strExpression(p, s){}
  Expression(const String &e): strExpression(e){}
  B bEvaluated = 0;
  union{
    Value value;
    String strExpression;
  };
};

typedef std::map<String, Expression> StringMap;
static StringMap g_definitions;

#define MAX_ARGUMENTS 16
static U64 g_uLine;

static E8 onStatement(void *pContext, const C *pKey, const C *pKeyEnd, const C *pValue, const C *pValueEnd);

inline
static E8 EvaluateExpression(Expression &expression);

inline
static E8 Call(const String &strName, const String *szArguments, U8 uArguments, Value &value);

inline
static E8 EvaluateArgument(const C *pBegin, const C *pEnd, Value &value);

inline
static E8 Output(const C *pExpression, const C *pExpressionEnd);

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

  MMFile_Close(&file);
  return e;
}

static E8 onStatement(void *pContext, const C *pKey, const C *pKeyEnd, const C *pValue, const C *pValueEnd){
  ++g_uLine;
  //fprintf(stderr, "%llu: \"%.*s: %.*s\"\n", g_uLine, (int)(pKeyEnd - pKey), pKey, (int)(pValueEnd - pValue), pValue);
  if(*pKey != '$'){
    String strKey(pKey, pKeyEnd - pKey);
    auto pr = g_definitions.emplace(strKey, String(pValue, pValueEnd - pValue));
    if(pr.second)
      return 0;

    fprintf(stderr, "Error(%llu): \"%.*s\" redefined!\n", g_uLine, (int)strKey.uSize, strKey.pBegin);
    return 1;
  }


  //  ++pKey;
  //  String strKey(pKey, pKeyEnd - pKey);
  //  if(strKey.uSize == 0){
  return Output(pValue, pValueEnd);
  //  }
}

inline
static E8 ParseArguments(String *pArguments, const String **ppArgumentsEnd, const C *pBegin, const C *pEnd){
  const String *pArgumentsEnd = *ppArgumentsEnd;
  String *pArgument = pArguments;
  const C *p = pBegin;

  if(*p != 0x20){
    fprintf(stderr, "Error(%llu): Expect a space(0x20)!\n", g_uLine);
    return 1;
  }

  do{
    ++p;
    if(pArgument == pArgumentsEnd){
      fprintf(stderr, "Error(%llu): Too much arguments!\n", g_uLine);
      return 1;
    }

    pArgument->pBegin = p;
    while(*p != 0x20 && p != pEnd)
      ++p;
    pArgument->uSize = p - pArgument->pBegin;
    ++pArgument;
  }while(*p == 0x20);

  if(p != pEnd){
    fprintf(stderr, "Error(%llu): Invalid tail \"%.*s\"!\n", g_uLine, (int)(pEnd - p), (char*)p);
    return 1;
  }

  *ppArgumentsEnd = pArgument;
  return 0;
}

inline
static E8 EvaluateExpression(Expression &expression){
//  fprintf(stderr, "Evaluate(%llu): %.*s\n", g_uLine, (int)expression.strExpression.uSize, (char*)expression.strExpression.pBegin);
  const String &strExpression = expression.strExpression;
  String szArguments[MAX_ARGUMENTS];
  const String *pArgumentsEnd = szArguments + MAX_ARGUMENTS;
  E8 e = ParseArguments(szArguments, &pArgumentsEnd, strExpression.pBegin, strExpression.pBegin + strExpression.uSize);
  if(e)
    return e;

  U8 uArguments = pArgumentsEnd - szArguments;
  //  fprintf(stderr, "%llu: %u arguments\n", g_uLine, uArguments);

  if(uArguments){
    E8 e;
    if(uArguments == 1){
      const String &argument = szArguments[0];
      e = EvaluateArgument(argument.pBegin, argument.pBegin + argument.uSize, expression.value);
    }
    else
      e = Call(szArguments[0], &szArguments[1], uArguments - 1, expression.value);

    if(e)
      return e;

    expression.bEvaluated = 1;
  }
  else{
    fprintf(stderr, "Error(%llu): Empty expression!\n", g_uLine);
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

    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;
    ++pArgument;

    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
      vLeft += vRight;
    }

    value = vLeft;
  }break;
  case '-':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
      vLeft -= vRight;
    }

    value = vLeft;
  }break;
  case '*':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
      vLeft *= vRight;
    }

    value = vLeft;
  }break;
  case '/':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
      vLeft /= vRight;
    }

    value = vLeft;
  }break;
  case '=':{
    if(uArguments < 2){
      fprintf(stderr, "Error(%llu): Argument count must greater than 1, but %u!\n", g_uLine, uArguments);
      return 1;
    }

    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;

    bool bCompare;
    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
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
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
    if(e)
      return e;

    ++pArgument;

    bool bCompare;
    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;
        bCompare = vLeft.iNumerator <= vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }
    else{
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;

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
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
    if(e)
      return e;

    ++pArgument;

    bool bCompare;
    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;

        bCompare = vLeft.iNumerator >= vRight.iNumerator;

        if(!bCompare)
          break;

        vLeft = vRight;
      }
    }
    else{
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;

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
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vLeft);
    if(e)
      return e;

    ++pArgument;
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
    if(e)
      return e;

    ++pArgument;

    Value vFalse;
    e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vFalse);
    if(e)
      return e;

    ++pArgument;
    value = vLeft.iNumerator? vRight : vFalse;
  }break;
  }

  return 0;
}

inline
static E8 EvaluateArgument(const C *pBegin, const C *pEnd, Value &value){
  B bSigned = 0;
  if(*pBegin == '-'){
    ++pBegin;
    bSigned = 1;
  }

  if(C_IsDigit(*pBegin)){
    const C *p = pBegin;
    I64 iValue = *p++ - '0';
    if(String_ParseU64(&p, (U64*)&iValue) || iValue < 0){
      fprintf(stderr, "Error(%llu): Integer(%.*s) overflowed!\n", g_uLine, (int)(pEnd - pBegin), (char*)pBegin);
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
    auto iter = g_definitions.find(String(pBegin, uSize));
    if(iter == g_definitions.end()){
      fprintf(stderr, "Error(%llu): \"%.*s\" not defined!\n", g_uLine, (int)uSize, (char*)pBegin);
      return 1;
    }

    Expression &expression = iter->second;
    if(!expression.bEvaluated){
      E8 e = EvaluateExpression(expression);
      if(e)
        return e;
    }
    value = expression.value;
  }

  if(bSigned)
    value.iNumerator = -value.iNumerator;

  return 0;
}

inline
static E8 Output(const C *pExpression, const C *pExpressionEnd){

  const C *p = pExpression;
  if(*p != 0x20){
    fprintf(stderr, "Error(%llu): Expect a space(0x20)!\n", g_uLine);
    return 1;
  }

  Value value;
  const C *pArgument;
  std::size_t uArgument;

  do{
    ++p;

    pArgument = p;
    while(*p != 0x20 && *p != '\n')
      ++p;
    uArgument = p - pArgument;
    if(EvaluateArgument(pArgument, p, value))
      fprintf(stderr, "Error(%llu): Evaluate \"%.*s\" failed!\n", g_uLine, (int)uArgument, (char*)pArgument);
    else{
      if(value.iDenominator == 1)
        printf("%.*s: %lld\n", (int)uArgument, (char*)pArgument, value.iNumerator);
      else if(value.iNumerator == 0)
        printf("%.*s: 0\n", (int)uArgument, (char*)pArgument);
      else
        printf("%.*s: %lld/%lld=%f\n", (int)uArgument, (char*)pArgument, value.iNumerator, value.iDenominator, (F64)value.iNumerator/(F64)value.iDenominator);
    }
  }while(*p == 0x20);

  if(*p != '\n'){
    fprintf(stderr, "Error(%llu): Expect a line feed(0x0A)!\n", g_uLine);
    return 1;
  }

  return 0;
}
