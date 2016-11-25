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
  String(const C *p, U8 s): pBegin(p), uSize(s){}

  bool operator<(const String &right)const{
    U8 uMin = uSize > right.uSize? right.uSize : uSize;
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
  U8 uSize = 0;
};

struct Value{
  enum class Type: U8{
    vtSource, vtBool, vtInteger, vtFloat, vFunction
  };

  Type type;
  union{
    B bValue;
    I64 iValue;
    F64 fValue;
  };
};

struct Expression{
  Expression(const C *p, U8 s): strExpression(p, s){}
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
  std::size_t uKey = pKeyEnd - pKey;
  std::size_t uValue = pValueEnd - pValue;
  //fprintf(stderr, "%llu: \"%.*s: %.*s\"\n", g_uLine, (int)uKey, pKey, (int)uValue, pValue);
  if(uKey > 255){
    fprintf(stderr, "Error(%llu): Key length must less than 256 bytes!\n", g_uLine);
    return 1;
  }

  if(uValue > 255){
    fprintf(stderr, "Error(%llu): Expression length must less than 256 bytes!\n", g_uLine);
    return 1;
  }

  if(*pKey != '$'){
    String strKey(pKey, uKey);
    auto pr = g_definitions.emplace(strKey, String(pValue, uValue));
    if(pr.second)
      return 0;

    fprintf(stderr, "Error(%llu): \"%.*s\" redefined!\n", g_uLine, (int)strKey.uSize, strKey.pBegin);
    return 1;
  }


  //  ++pKey;
  //  String strKey(pKey, uKey);
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

      if(vLeft.type != vRight.type){
        fprintf(stderr, "Error(%llu): Arguments must be same type!\n", g_uLine);
        return 1;
      }

      ++pArgument;

      if(vLeft.type == Value::Type::vtInteger)
        vLeft.iValue += vRight.iValue;
      else if(vLeft.type == Value::Type::vtFloat)
        vLeft.fValue += vRight.fValue;
      else{
        fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
        return 1;
      }
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

      if(vLeft.type != vRight.type){
        fprintf(stderr, "Error(%llu): Arguments must be same type!\n", g_uLine);
        return 1;
      }

      ++pArgument;
      if(vLeft.type == Value::Type::vtInteger)
        vLeft.iValue -= vRight.iValue;
      else if(vLeft.type == Value::Type::vtFloat)
        vLeft.fValue -= vRight.fValue;
      else{
        fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
        return 1;
      }
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

      if(vLeft.type != vRight.type){
        fprintf(stderr, "Error(%llu): Arguments must be same type!\n", g_uLine);
        return 1;
      }

      ++pArgument;
      if(vLeft.type == Value::Type::vtInteger)
        vLeft.iValue *= vRight.iValue;
      else if(vLeft.type == Value::Type::vtFloat)
        vLeft.fValue *= vRight.fValue;
      else{
        fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
        return 1;
      }
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

      if(vLeft.type != vRight.type){
        fprintf(stderr, "Error(%llu): Arguments must be same type!\n", g_uLine);
        return 1;
      }

      ++pArgument;
      if(vLeft.type == Value::Type::vtInteger)
        vLeft.iValue /= vRight.iValue;
      else if(vLeft.type == Value::Type::vtFloat)
        vLeft.fValue /= vRight.fValue;
      else{
        fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
        return 1;
      }
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

    while(pArgument != pArgumentEnd){
      e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
      if(e)
        return e;

      ++pArgument;
      value.bValue = (vLeft.type != vRight.type? 0: (vLeft.iValue == vRight.iValue));
      if(!value.bValue)
        break;
    }

    value.type = Value::Type::vtBool;
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

    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;

        if(vLeft.type != vRight.type)
          value.bValue = 0;
        else{
          if(vLeft.type == Value::Type::vtFloat)
            value.bValue = vLeft.fValue <= vRight.fValue;
          else if(vLeft.type == Value::Type::vtInteger)
            value.bValue = vLeft.iValue <= vRight.iValue;
          else{
            fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
            return 1;
          }
        }

        if(!value.bValue)
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

        if(vLeft.type != vRight.type)
          value.bValue = 0;
        else{
          if(vLeft.type == Value::Type::vtFloat)
            value.bValue = vLeft.fValue < vRight.fValue;
          else if(vLeft.type == Value::Type::vtInteger)
            value.bValue = vLeft.iValue < vRight.iValue;
          else{
            fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
            return 1;
          }
        }

        if(!value.bValue)
          break;
        vLeft = vRight;
      }
    }

    value.type = Value::Type::vtBool;
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

    if(p[1] == '='){
      while(pArgument != pArgumentEnd){
        e = EvaluateArgument(pArgument->pBegin, pArgument->pBegin + pArgument->uSize, vRight);
        if(e)
          return e;

        ++pArgument;

        if(vLeft.type != vRight.type)
          value.bValue = 0;
        else{
          if(vLeft.type == Value::Type::vtFloat)
            value.bValue = vLeft.fValue >= vRight.fValue;
          else if(vLeft.type == Value::Type::vtInteger)
            value.bValue = vLeft.iValue >= vRight.iValue;
          else{
            fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
            return 1;
          }
        }

        if(!value.bValue)
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

        if(vLeft.type != vRight.type)
          value.bValue = 0;
        else{
          if(vLeft.type == Value::Type::vtFloat)
            value.bValue = vLeft.fValue > vRight.fValue;
          else if(vLeft.type == Value::Type::vtInteger)
            value.bValue = vLeft.iValue > vRight.iValue;
          else{
            fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
            return 1;
          }
        }

        if(!value.bValue)
          break;

        vLeft = vRight;
      }
    }

    value.type = Value::Type::vtBool;
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
    value = vLeft.bValue? vRight : vFalse;
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

    if(*p != '.'){
      value.type = Value::Type::vtInteger;
      value.iValue = iValue;
    }
    else{
      value.type = Value::Type::vtFloat;
      value.fValue = F64(iValue);
      value.fValue += strtod((char*)p, (char**)&p);
    }

    if(p != pEnd){
      fprintf(stderr, "Error(%llu): Invalid tail \"%.*s\"!\n", g_uLine, (int)(pEnd - p), (char*)p);
      return 1;
    }

  }
  else{
    U8 uSize = pEnd - pBegin;
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

  if(bSigned){
    if(value.type == Value::Type::vtFloat)
      value.fValue = -value.fValue;
    else if(value.type == Value::Type::vtInteger)
      value.iValue = -value.iValue;
    else{
      fprintf(stderr, "Error(%llu): Not supported opeartion!\n", g_uLine);
      return 1;
    }
  }

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
  U8 uArgument;

  do{
    ++p;

    pArgument = p;
    while(*p != 0x20 && *p != '\n')
      ++p;
    uArgument = p - pArgument;
    if(EvaluateArgument(pArgument, p, value))
      fprintf(stderr, "Error(%llu): Evaluate \"%.*s\" failed!\n", g_uLine, (int)uArgument, (char*)pArgument);
    else{
      if(value.type == Value::Type::vtInteger)
        printf("%.*s: %lld\n", (int)uArgument, (char*)pArgument, value.iValue);
      else if(value.type == Value::Type::vtBool)
        printf("%.*s: %s\n", (int)uArgument, (char*)pArgument, value.bValue? "true" : "false");
      else if(value.type == Value::Type::vtFloat)
        printf("%.*s: %f\n", (int)uArgument, (char*)pArgument, value.fValue);
    }
  }while(*p == 0x20);

  if(*p != '\n'){
    fprintf(stderr, "Error(%llu): Expect a line feed(0x0A)!\n", g_uLine);
    return 1;
  }

  return 0;
}
