#pragma once

#include <charconv>
#include <iostream>
#include <map>
#include <vector>
#include <variant>

namespace BT::BEncoding
{

using BInt = int;
using BStr = std::string;

struct BElem;
using BList = std::vector<BElem>;
using BDict = std::map<BStr, BElem>;

enum class Type
{
  None = 0,
  BInt,
  BStr,
  BList,
  BDict,
};

std::ostream& operator<<(std::ostream& stream, Type type)
{
  switch(type)
  {
    case Type::BInt:  stream << "BInt";  break;
    case Type::BStr:  stream << "BStr";  break;
    case Type::BList: stream << "BList"; break;
    case Type::BDict: stream << "BDict"; break;
    case Type::None:  stream << "None";  break;
  }

  return stream;
}

using BElemVariant = std::variant<std::monostate, BInt, BStr, BList, BDict>;
struct BElem : public BElemVariant
{
  using BElemVariant::variant;

  BElem& operator[](const BStr& key)
  {
    if(this->GetType() != Type::BDict)
      throw std::runtime_error{"using operator[](BStr) on non BDict"};

    return std::get<BDict>(*this)[key];
  }

  BElem& operator[](const BInt& index)
  {
    if(this->GetType() != Type::BList)
      throw std::runtime_error{"using operator[](BInt) on non BList"};

    return std::get<BList>(*this)[index];
  }

  template<typename T>
  T& As()
  {
    return std::get<T>(*this);
  }

  Type GetType() const
  {
    return std::holds_alternative<BInt>(*this)  ? Type::BInt  :
           std::holds_alternative<BStr>(*this)  ? Type::BStr  :
           std::holds_alternative<BList>(*this) ? Type::BList : 
           std::holds_alternative<BDict>(*this) ? Type::BDict : Type::None;
  }
};

Type PeekNext(std::istream& stream)
{
  int next = stream.peek();
  return next == 'i' ? Type::BInt  :
         next == 'l' ? Type::BList :
         next == 'd' ? Type::BDict :
         next >= '0' && next <= '9' ? Type::BStr : Type::None;
}

BElem Parse(std::istream& stream);

BInt ParseBInt(std::istream& stream)
{
  //check for start identifier char 'i'
  if(stream.peek() != 'i')
    throw std::runtime_error{"invalid bint: no start identifier"};

  //consume start identifier
  stream.get();

  //check sign
  bool negative = (stream.peek() == '-');

  //consume sign if there
  if(negative)
    stream.get();

  std::vector<char> numberChars;

  while( std::isdigit(stream.peek()) )
    numberChars.push_back(stream.get());

  //check if any digits were parsed
  if(numberChars.size() == 0)
    throw std::runtime_error{"invalid bint: no digits"};

  //check for end identifier char 'e'
  if(stream.peek() != 'e')
    throw std::runtime_error{"invalid bint: invalid end identifier"};

  //consume end idenfitier
  stream.get();

  //convert chars to int
  int num;
  auto [ptr, err] = std::from_chars(numberChars.data(), numberChars.data()+numberChars.size(), num);

  if(err != std::errc())
    throw std::runtime_error{"invalid bint: std::from_chars failed"};

  //negate number if negative
  if(negative)
    num = -num;

  return num;
}

BStr ParseBStr(std::istream& stream)
{
  //read len string
  std::vector<char> lenChars;
  while( std::isdigit(stream.peek()) )
    lenChars.push_back(stream.get());

  if(lenChars.size() == 0)
    throw std::runtime_error{"invalid bstr: invalid str len before ':'"};

  //check for ':'
  if(stream.peek() != ':')
    throw std::runtime_error{"invalid bstr: no ':' found"};

  //consume ':'
  stream.get();

  //convert len str to int
  int len;
  auto [ptr, err] = std::from_chars(lenChars.data(), lenChars.data()+lenChars.size(), len);

  if(err != std::errc())
    throw std::runtime_error{"invalid bstr: std::from_chars failed"};

  //read string off stream
  std::string str(len, '\0');
  stream.read(&str[0], len);

  return str;
}

BList ParseBList(std::istream& stream)
{
  //check for start identifier char 'l'
  if(stream.peek() != 'l')
    throw std::runtime_error{"invalid blist: no start identifier"};

  //consume start idenfitier
  stream.get();

  std::vector<BElem> list;

  //parse list elements until end idenfitier 'e' is encountered
  while(stream.peek() != 'e')
    list.emplace_back( BEncoding::Parse(stream) );

  //consume end identifier
  stream.get();

  return list;
}

BDict ParseBDict(std::istream& stream)
{
  //check for start identifier char 'd'
  if(stream.peek() != 'd')
    throw std::runtime_error{"invalid bdict: no start identifier"};

  //consume start idenfitier
  stream.get();

  BDict dict;

  //ensure next element is a string
  if(PeekNext(stream) != Type::BStr)
    throw std::runtime_error{"invalid bdict: key is not string"};

  //parse key value pairs until end idenfitier 'e' is encountered
  while(stream.peek() != 'e')
  {
    BStr  key = ParseBStr(stream);
    BElem val = BEncoding::Parse(stream);
    dict[key] = val;
  }
  
  //consume end identifier
  stream.get();

  return dict;
}

BElem Parse(std::istream& stream)
{
  switch( PeekNext(stream) )
  {
    case Type::BInt:  return ParseBInt(stream);
    case Type::BStr:  return ParseBStr(stream);
    case Type::BList: return ParseBList(stream);
    case Type::BDict: return ParseBDict(stream);
  }

  return std::monostate{};
}

std::istream& operator>>(std::istream& stream, BElem& elem)
{
  elem = Parse(stream);

  if(elem.GetType() == Type::None)
    stream.setstate(std::ios::failbit);

  return stream;
}

void Serialize(std::ostream&, const BElem&);

void SerializeBInt(std::ostream& stream, const BInt& bint)
{
  stream << 'i' << bint << 'e';
}

void SerializeBStr(std::ostream& stream, const BStr& bstr)
{
  stream << bstr.length() << ':' << bstr;
}

void SerializeBList(std::ostream& stream, const BList& blist)
{
  stream << 'l';
  for(const BElem& elem : blist)
    Serialize(stream, elem);
  stream << 'e';
}

void SerializeBDict(std::ostream& stream, const BDict& bdict)
{
  stream << 'd';
  for(const auto& kv : bdict)
  {
    SerializeBStr(stream, kv.first);
    Serialize(stream, kv.second);
  }
  stream << 'e';
}

void Serialize(std::ostream& stream, const BElem& elem)
{
  switch( elem.GetType() )
  {
    case Type::BInt:  SerializeBInt(stream,  std::get<BInt>(elem));  break;
    case Type::BStr:  SerializeBStr(stream,  std::get<BStr>(elem));  break;
    case Type::BList: SerializeBList(stream, std::get<BList>(elem)); break;
    case Type::BDict: SerializeBDict(stream, std::get<BDict>(elem)); break;
  }
}

std::ostream& operator<<(std::ostream& stream, const BElem& elem)
{
  Serialize(stream, elem);
  return stream;
}


} //namespace: BT::BEncoding
