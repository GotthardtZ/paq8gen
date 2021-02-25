#ifndef PAQ8GEN_FILTERS_HPP
#define PAQ8GEN_FILTERS_HPP

#include "../Array.hpp"
#include "../Encoder.hpp"
#include "../file/File.hpp"
#include "../file/FileDisk.hpp"
#include "../file/FileTmp.hpp"
#include "../utils.hpp"
#include "Filter.hpp"
#include "eol.hpp"
#include "TextParserStateInfo.hpp"
#include <cctype>
#include <cstdint>
#include <cstring>

/////////////////////////// Filters /////////////////////////////////
//@todo: Update this documentation
//
// Before compression, data is encoded in blocks with the following format:
//
//   <type> <size> <encoded-data>
//
// type is 1 byte (type BlockType): DEFAULT=0, JPEG, EXE
// size is 4 bytes in big-endian format.
// Encoded-data decodes to <size> bytes.  The encoded size might be
// different.  Encoded data is designed to be more compressible.
//
//   void encode(File *in, File *out, int n);
//
// Reads n bytes of in (open in "rb" mode) and encodes one or
// more blocks to temporary file out (open in "wb+" mode).
// The file pointer of in is advanced n bytes.  The file pointer of
// out is positioned after the last byte written.
//
//   en.setFile(File *out);
//   int decode(Encoder& en);
//
// Decodes and returns one byte.  Input is from en.decompressByte(), which
// reads from out if in COMPRESS mode.  During compression, n calls
// to decode() must exactly match n bytes of in, or else it is compressed
// as type 0 without encoding.
//
//   BlockType detect(File *in, int n, BlockType type);
//
// Reads n bytes of in, and detects when the type changes to
// something else.  If it does, then the file pointer is repositioned
// to the start of the change and the new type is returned.  If the type
// does not change, then it repositions the file pointer n bytes ahead
// and returns the old type.
//
// For each type X there are the following 2 functions:
//
//   void encode_X(File *in, File *out, int n, ...);
//
// encodes n bytes from in to out.
//
//   int decode_X(Encoder& en);
//
// decodes one byte from en and returns it.  decode() and decode_X()
// maintain state information using static variables.





// Detect blocks
static auto detect(File *in, uint64_t blockSize, BlockType type, int &info) -> BlockType {
  TextParserStateInfo *textParser = &TextParserStateInfo::getInstance();
  // TODO: Large file support
  const uint64_t n = blockSize;

  // last 16 bytes
  uint32_t buf3 = 0;
  uint32_t buf2 = 0;
  uint32_t buf1 = 0;
  uint32_t buf0 = 0;
  
  static uint64_t start = 0;
  static uint64_t prv_start = 0;

  prv_start = start;    // for DEC Alpha detection
  start = in->curPos(); // start of the current block

  textParser->reset(0);
  for (uint64_t i = 0; i < n; ++i) {
    int c = in->getchar();
    if (c == EOF) {
      return static_cast<BlockType>(-1);
    }
    buf3 = buf3 << 8 | buf2 >> 24;
    buf2 = buf2 << 8 | buf1 >> 24;
    buf1 = buf1 << 8 | buf0 >> 24;
    buf0 = buf0 << 8 | c;


    // Detect text
    // This is different from the above detection routines: it's a negative detection (it detects a failure)
    uint32_t t = TextParserStateInfo::utf8StateTable[c];
    textParser->UTF8State = TextParserStateInfo::utf8StateTable[256 + textParser->UTF8State + t];
    if( textParser->UTF8State == TextParserStateInfo::utf8Accept ) { // proper end of a valid utf8 sequence
      if( c == NEW_LINE ) {
        if(((buf0 >> 8) & 0xff) != CARRIAGE_RETURN ) {
          textParser->setEolType(2); // mixed or LF-only
        } else if( textParser->eolType() == 0 ) {
          textParser->setEolType(1); // CRLF-only
        }
      }
      textParser->invalidCount = textParser->invalidCount * (TEXT_ADAPT_RATE - 1) / TEXT_ADAPT_RATE;
      if( textParser->invalidCount == 0 ) {
        textParser->setEnd(i); // a possible end of block position
      }
    } else if( textParser->UTF8State == TextParserStateInfo::utf8Reject ) { // illegal state
      textParser->invalidCount = textParser->invalidCount * (TEXT_ADAPT_RATE - 1) / TEXT_ADAPT_RATE + TEXT_ADAPT_RATE;
      textParser->UTF8State = TextParserStateInfo::utf8Accept; // reset state
      if( textParser->validLength() < TEXT_MIN_SIZE ) {
        textParser->reset(i + 1); // it's not text (or not long enough) - start over
      } else if( textParser->invalidCount >= TEXT_MAX_MISSES * TEXT_ADAPT_RATE ) {
        if( textParser->validLength() < TEXT_MIN_SIZE ) {
          textParser->reset(i + 1); // it's not text (or not long enough) - start over
        } else { // Commit text block validation
          textParser->next(i + 1);
        }
      }
    }
  }
  return type;
}

//////////////////// Compress, Decompress ////////////////////////////

static void directEncodeBlock(BlockType type, File *in, uint64_t len, Encoder &en, int info = -1) {
  // TODO: Large file support
  en.encodeBlockType(type);
  en.encodeBlockSize(len);
  if( info != -1 ) {
    en.encodeInfo(info);
  }
  fprintf(stderr, "Compressing... ");
  for( uint64_t j = 0; j < len; ++j ) {
#ifndef CHALLENGE
    if((j & 0xfff) == 0 ) {
      en.printStatus(j, len);
    }
#endif
    en.compressByte(in->getchar());
  }
  fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

static void compressRecursive(File *in, uint64_t blockSize, Encoder &en, String &blstr, int recursionLevel, float p1, float p2);

static auto
decodeFunc(BlockType type, Encoder &en, File *tmp, uint64_t len, int info, File *out, FMode mode, uint64_t &diffFound) -> uint64_t {
  if( type == TEXT_EOL ) {
    auto d = new EolFilter();
    d->setEncoder(en);
    return d->decode(tmp, out, mode, len, diffFound);
  } 
  else {
    assert(false);
  }
  return 0;
}

static auto encodeFunc(BlockType type, File *in, File *tmp, uint64_t len, int info, int &hdrsize) -> uint64_t {
if( type == TEXT_EOL ) {
    auto e = new EolFilter();
    e->encode(in, tmp, len, info, hdrsize);
  } else {
    assert(false);
  }
  return 0;
}

static void
transformEncodeBlock(BlockType type, File *in, uint64_t len, Encoder &en, int info, String &blstr, int recursionLevel, float p1, float p2, uint64_t begin) {
  if( hasTransform(type)) {
    FileTmp tmp;
    int headerSize = 0;
    uint64_t diffFound = encodeFunc(type, in, &tmp, len, info, headerSize);
    const uint64_t tmpSize = tmp.curPos();
    tmp.setpos(tmpSize); //switch to read mode
    if( diffFound == 0 ) {
      tmp.setpos(0);
      en.setFile(&tmp);
      in->setpos(begin);
      decodeFunc(type, en, &tmp, tmpSize, info, in, FCOMPARE, diffFound);
    }
    // Test fails, compress without transform
    if( diffFound > 0 || tmp.getchar() != EOF) {
      printf("Transform fails at %" PRIu64 ", skipping...\n", diffFound - 1);
      in->setpos(begin);
      directEncodeBlock(DEFAULT, in, len, en);
    } else {
      tmp.setpos(0);
      if( hasRecursion(type)) {
        // TODO: Large file support
        en.encodeBlockType(type);
        en.encodeBlockSize(tmpSize);
        compressRecursive(&tmp, tmpSize, en, blstr, recursionLevel + 1, p1, p2);
      } else {
        directEncodeBlock(type, &tmp, tmpSize, en, hasInfo(type) ? info : -1);
      }
    }
    tmp.close();
  } else {
    directEncodeBlock(type, in, len, en, hasInfo(type) ? info : -1);
  }
}

static void compressRecursive(File *in, const uint64_t blockSize, Encoder &en, String &blstr, int recursionLevel, float p1, float p2) {
  static const char *typeNames[4] = {"default", "filecontainer", "text", "text - eol"};
  BlockType type = DEFAULT;
  int blNum = 0;
  int info = 0; // image width or audio type
  uint64_t begin = in->curPos();
  uint64_t blockEnd = begin + blockSize;
  if( recursionLevel == 5 ) {
    directEncodeBlock(DEFAULT, in, blockSize, en);
    return;
  }
  float pscale = blockSize > 0 ? (p2 - p1) / blockSize : 0;

  // Transform and test in blocks
  uint64_t nextBlockStart = 0;
  uint64_t textStart = 0;
  uint64_t textEnd = 0;
  BlockType nextBlockType;
  BlockType nextBlockTypeBak = DEFAULT; //initialized only to suppress a compiler warning, will be overwritten
  uint64_t bytesToGo = blockSize;
  TextParserStateInfo *textParser = &TextParserStateInfo::getInstance();
  while( bytesToGo > 0 ) {
    if( type == TEXT || type == TEXT_EOL ) { // it was a split block in the previous iteration: TEXT -> DEFAULT -> ...
      nextBlockType = nextBlockTypeBak;
      nextBlockStart = textEnd + 1;
    } else {
      nextBlockType = detect(in, bytesToGo, type, info);
      nextBlockStart = in->curPos();
      in->setpos(begin);
    }

    // override (any) next block detection by a preceding text block
    textStart = begin + textParser->_start[0];
    textEnd = begin + textParser->_end[0];
    if( textEnd > nextBlockStart - 1 ) {
      textEnd = nextBlockStart - 1;
    }
    if( type == DEFAULT && textStart < textEnd && textEnd != UINT64_MAX ) { // only DEFAULT blocks may be overridden
      if( textStart == begin && textEnd == nextBlockStart - 1 ) { // whole first block is text
        type = (textParser->_EOLType[0] == 1) ? TEXT_EOL : TEXT; // DEFAULT -> TEXT
      } else if( textEnd - textStart + 1 >= TEXT_MIN_SIZE ) { // we have one (or more) large enough text portion that splits DEFAULT
        if( textStart != begin ) { // text is not the first block
          nextBlockStart = textStart; // first block is still DEFAULT
          nextBlockTypeBak = nextBlockType;
          nextBlockType = (textParser->_EOLType[0] == 1) ? TEXT_EOL : TEXT; //next block is text
          textParser->removeFirst();
        } else {
          type = (textParser->_EOLType[0] == 1) ? TEXT_EOL : TEXT; // first block is text
          nextBlockType = DEFAULT; // next block is DEFAULT
          nextBlockStart = textEnd + 1;
        }
      }
      // no text block is found, still DEFAULT
    }

    if( nextBlockStart > blockEnd ) { // if a detection reports a larger size than the actual block size, fall back
      nextBlockStart = begin + 1;
      type = nextBlockType = DEFAULT;
    }

    uint64_t len = nextBlockStart - begin;
    if( len > 0 ) {
      en.setStatusRange(p1, p2 = p1 + pscale * len);

      //Compose block enumeration string
      String blstrSub;
      blstrSub += blstr.c_str();
      if( blstrSub.strsize() != 0 ) {
        blstrSub += "-";
      }
      blstrSub += uint64_t(blNum);
      blNum++;
      printf(" %-11s | %-16s |%10" PRIu64 " bytes [%" PRIu64 " - %" PRIu64 "]", blstrSub.c_str(),
             typeNames[type], len, begin, nextBlockStart - 1);
      printf("\n");
      transformEncodeBlock(type, in, len, en, info, blstrSub, recursionLevel, p1, p2, begin);
      p1 = p2;
      bytesToGo -= len;
    }
    type = nextBlockType;
    begin = nextBlockStart;
  }
}

// Compress a file. Split fileSize bytes into blocks by type.
// For each block, output
// <type> <size> and call encode_X to convert to type X.
// Test transform and compress.
static void compressfile(const Shared* const shared, const char *filename, uint64_t fileSize, Encoder &en, bool verbose) {
  assert(en.getMode() == COMPRESS);
  assert(filename && filename[0]);

  en.encodeBlockType(FILECONTAINER);
  uint64_t start = en.size();
  en.encodeBlockSize(fileSize);

  FileDisk in;
  in.open(filename, true);
  printf("Block segmentation:\n");
  String blstr;
  compressRecursive(&in, fileSize, en, blstr, 0, 0.0F, 1.0F);
  in.close();


}

static auto decompressRecursive(File *out, uint64_t blockSize, Encoder &en, FMode mode, int recursionLevel) -> uint64_t {
  BlockType type;
  uint64_t len = 0;
  uint64_t i = 0;
  uint64_t diffFound = 0;
  int info = 0;
  while( i < blockSize ) {
    type = en.decodeBlockType();
    len = en.decodeBlockSize();
    if( hasInfo(type)) {
      info = en.decodeInfo();
    }
    if( hasRecursion(type)) {
      FileTmp tmp;
      decompressRecursive(&tmp, len, en, FDECOMPRESS, recursionLevel + 1);
      if( mode != FDISCARD ) {
        tmp.setpos(0);
        if( hasTransform(type)) {
          len = decodeFunc(type, en, &tmp, len, info, out, mode, diffFound);
        }
      }
      tmp.close();
    } else if( hasTransform(type)) {
      len = decodeFunc(type, en, nullptr, len, info, out, mode, diffFound);
    } else {
      for( uint64_t j = 0; j < len; ++j ) {
#ifndef CHALLENGE
        if((j & 0xfff) == 0u ) {
          en.printStatus();
        }
#endif
        if( mode == FDECOMPRESS ) {
          out->putChar(en.decompressByte());
        } else if( mode == FCOMPARE ) {
          if( en.decompressByte() != out->getchar() && (diffFound == 0u)) {
            mode = FDISCARD;
            diffFound = i + j + 1;
          }
        } else {
          en.decompressByte();
        }
      }
    }
    i += len;
  }
  return diffFound;
}

// Decompress or compare a file
static void decompressFile(const Shared *const shared, const char *filename, FMode fMode, Encoder &en) {
  assert(en.getMode() == DECOMPRESS);
  assert(filename && filename[0]);

  BlockType blocktype = en.decodeBlockType();
  if( blocktype != FILECONTAINER ) {
    quit("Bad archive.");
  }
  uint64_t fileSize = en.decodeBlockSize();

  FileDisk f;
  if( fMode == FCOMPARE ) {
    f.open(filename, true);
    printf("Comparing");
  } else { //mode==FDECOMPRESS;
    f.create(filename);
    printf("Extracting");
  }
  printf(" %s %" PRIu64 " bytes -> ", filename, fileSize);

  // Decompress/Compare
  uint64_t r = decompressRecursive(&f, fileSize, en, fMode, 0);
#ifndef CHALLENGE
  if( fMode == FCOMPARE && (r == 0u) && f.getchar() != EOF) {
    printf("file is longer\n");
  } else if( fMode == FCOMPARE && (r != 0u)) {
    printf("differ at %" PRIu64 "\n", r - 1);
  } else if( fMode == FCOMPARE ) {
    printf("identical\n");
  } else {
    printf("done   \n");
  }
#endif
  f.close();
}

#endif //PAQ8GEN_FILTERS_HPP
