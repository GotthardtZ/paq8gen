#include "file/File.hpp"
#include "file/fileUtils2.hpp"

// The following transformations are used for the "SARS-CoV-2 Coronavirus Data Compression Benchmark" (https://coronavirus.innar.com/)
// 1) interleave/deinterleave sequence names and sequences
// 2) alphabet-transformation (for the sequences only)

static constexpr uint8_t mapping[16] = { 71,67,65,84,78,89,75,82,87,77,10,83,72,68,86,66 }; // "GCATNYKRWM{\n}SHDVB"
static constexpr uint8_t mapping_reverse[25] = { 2,15,1,13,16,17,0,12,18,19,6,20,9,4,21,22,23,7,11,3,24,14,8,25,5 };

/*
alphabet-transformation is based on the following frequencies:

65	A	388931944
66	B	59
67	C	239142742
68	D	144
69	E	0
70	F	0
71	G	255577497
72	H	237
73	I	0
74	J	0
75	K	11622
76	L	0
77	M	3505
78	N	15186607
79	O	0
80	P	0
81	Q	0
82	R	7865
83	S	2381
84	T	418420417
85	U	0
86	V	89
87	W	5764
88	X	0
89	Y	18009
90	Z	0

*/

// deinterleave names and sequences + transform sequence-alphabet to 16-value alphabet (result: better compressible file)
void do_transform(uint64_t size, uint8_t* buffer/*original file content*/, uint8_t* output /*transformed file content - to be compressed*/) {
  int j = 0;
  for (int k = 0; k < 2; k++) {
    for (int i = 0; i < size; i++) {
      if (buffer[i] == '>') //name
        for (; i < size; i++) {
          uint8_t c = buffer[i];
          if (k == 0) {
            output[j] = c;
            j++;
          }
          if (c == '\n')break;
        }
      else //sequence
        for (; i < size; i++) {
          uint8_t c = buffer[i];
          if (k != 0) {
            //note: 0-15 is actually used; code for \n (10) newline is preserved for line-end detection to work
            if (c != '\n')
              c = mapping_reverse[c - 65];
            output[j] = c;
            j++;
          }
          if (c == '\n')break;
        }
    }
  }

}


// interleave names and sequences + transform 16-value alphabet to original sequence-alphabet (result: original file)
void do_transform_reverse(uint64_t size, uint8_t* buffer /*decompressed file content*/, uint8_t* output /* restored original file content*/) {
  int i = 0; //start of names
  int j = 583867; //start of sequences
  int k = 0;
  while(j<size) {
    //name
    for (;;) {
      uint8_t c = buffer[i];
      i++;
      output[k] = c;
      k++;
      if (c == '\n')
        break;
    }
    //sequence
    for (;;) {
      //note: code for \n (10) newline is preserved for line-end detection to work
      uint8_t c = buffer[j];
      j++;
      output[k] = mapping[c];
      k++;
      if (c == '\n')
        break;
    }

  }
}

void transform_sars_cov2_challenge() {
  const char* input_filename = "coronavirus.unwrapped.fasta";
  const char* output_filename = "c-full";

  FileDisk f;
  f.open(input_filename, true);
  size_t size = getFileSize(input_filename);
  Array<uint8_t> buffer(size);
  Array<uint8_t> output(size);
  f.blockRead(&buffer[0], size);
  f.close();

  do_transform(size, &buffer[0], &output[0]);

  f.create(output_filename);
  f.blockWrite(&output[0], size);
  f.close();
}


void transform_sars_cov2_challenge_reverse() {
  const char* input_filename = "c-full";
  const char* output_filename = "c-full-restored";

  FileDisk f;
  f.open(input_filename, true);
  size_t size = getFileSize(input_filename);
  Array<uint8_t> buffer(size);
  Array<uint8_t> output(size);
  f.blockRead(&buffer[0], size);
  f.close();

  do_transform_reverse(size, &buffer[0], &output[0]);

  f.create(output_filename);
  f.blockWrite(&output[0], size);
  f.close();
}
