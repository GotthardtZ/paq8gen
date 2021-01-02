#include "TextParserStateInfo.hpp"

auto TextParserStateInfo::getInstance() -> TextParserStateInfo& {
  static TextParserStateInfo instance;
  return instance;
}

void TextParserStateInfo::reset(uint64_t startPos) {
  _start.resize(1);
  _end.resize(1);
  _start[0] = startPos;
  _end[0] = startPos - 1;
  _EOLType.resize(1);
  _EOLType[0] = 0;
  invalidCount = 0;
  UTF8State = 0;
}

auto TextParserStateInfo::start() -> uint64_t {
  return _start[_start.size() - 1];
}

auto TextParserStateInfo::end() -> uint64_t {
  return _end[_end.size() - 1];
}

void TextParserStateInfo::setEnd(uint64_t end) {
  _end[_end.size() - 1] = end;
}

auto TextParserStateInfo::eolType() -> uint32_t {
  return _EOLType[_EOLType.size() - 1];
}

void TextParserStateInfo::setEolType(uint32_t eolType) {
  _EOLType[_EOLType.size() - 1] = eolType;
}

auto TextParserStateInfo::validLength() -> uint64_t {
  return end() - start() + 1;
}

void TextParserStateInfo::next(uint64_t startPos) {
  _start.pushBack(startPos);
  _end.pushBack(startPos - 1);
  _EOLType.pushBack(0);
  invalidCount = 0;
  UTF8State = 0;
}

void TextParserStateInfo::removeFirst() {
  if( _start.size() == 1 ) {
    reset(0);
  } else {
    for( int i = 0; i < static_cast<int>(_start.size()) - 1; i++ ) {
      _start[i] = _start[i + 1];
      _end[i] = _end[i + 1];
      _EOLType[i] = _EOLType[i + 1];
    }
    _start.popBack();
    _end.popBack();
    _EOLType.popBack();
  }
}