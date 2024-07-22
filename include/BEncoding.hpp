#pragma once

#include <charconv>
#include <istream>
#include <vector>

namespace BT::BEncoding
{

enum Type
{
  BInt,
  BStr,
  BList,
  BDict,
  INVALID,
};

Type PeekNext(std::istream& stream)
{
  char next = stream.peek();
  return next == 'i' ? BInt  :
         next == 'l' ? BList :
         next == 'd' ? BDict :
         std::isdigit(next) && next != '0' ? BStr :
         INVALID;
}

int ParseBInt(std::istream& stream)
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

std::string ParseBStr(std::istream& stream)
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
  std::string str;
  while(len--)
    str += stream.get();

  return str;
}

} //namespace: BT::BEncoding
