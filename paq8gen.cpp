  /*
  PAQ8GEN file compressor/archiver
  see README for information
  see CHANGELOG for version history
*/


//////////////////////// Versioning ////////////////////////////////////////

#define PROGNAME     "paq8gen"
#define PROGVERSION  "4"  //update version here before publishing your changes
#define PROGYEAR     "2021"


#include "utils.hpp"

#include <stdexcept>  //std::exception

#include "Encoder.hpp"
#include "ProgramChecker.hpp"
#include "Shared.hpp"
#include "String.hpp"
#include "file/FileName.hpp"
#include "file/fileUtils2.hpp"
#include "filter/Filters.hpp"
#include "simd.hpp"

typedef enum { DoNone, DoCompress, DoExtract, DoCompare } WHATTODO;

static void printHelp() {
  printf("\n"
         "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
         "To compress:\n"
         "\n"
         "  " PROGNAME " -LEVEL[SWITCHES] INPUTSPEC [OUTPUTSPEC]\n"
         "\n"
         "    -LEVEL:\n"
         "      -0 = no compression, only transformations when applicable (uses 33 MB)\n"
         "      -1 -2 -3 = compress using less memory (88, 97, 116 MB)\n"
         "      -4 -5 -6 -7 -8 -9 = use more memory (153, 228, 378, 678, 1278, 2478 MB)\n"
         "      -10  -11  -12     = use even more memory (4877, 9676, 19274 MB)\n"
         "\n"
         "    INPUTSPEC:\n"
         "    The input may be a FILE or a PATH/FILE.\n"
         "    Only file content and the file size is kept in the archive. Filename,\n"
         "    path, date and any file attributes or permissions are not stored.\n"
         "\n"
         "    OUTPUTSPEC:\n"
         "    When omitted: the archive will be created in the current folder. The\n"
         "    archive filename will be constructed from the input file name by \n"
         "    appending ." PROGNAME PROGVERSION " extension to it.\n"
         "    When OUTPUTSPEC is a filename (with an optional path) it will be\n"
         "    used as the archive filename.\n"
         "    When OUTPUTSPEC is a folder the archive file will be generated from\n"
         "    the input filename and will be created in the specified folder.\n"
         "    If the archive file already exists it will be overwritten.\n"
         "\n"
         "    Example:\n"
         "      " PROGNAME " -8 sequence.fasta\n"
         "\n"
         "To extract (decompress contents):\n"
         "\n"
         "  " PROGNAME " -d [INPUTPATH/]ARCHIVEFILE [[OUTPUTPATH/]OUTPUTFILE]\n"
         "    If an output folder is not provided the output file will go to the input\n"
         "    folder. If an output filename is not provided output filename will be the\n"
         "    same as ARCHIVEFILE without the last extension (e.g. without ." PROGNAME PROGVERSION")\n"
         "    When OUTPUTPATH does not exist it will be created.\n"
         "\n"
         "To test:\n"
         "\n"
         "  " PROGNAME " -t [INPUTPATH/]ARCHIVEFILE [[OUTPUTPATH/]OUTPUTFILE]\n"
         "    Tests contents of the archive by decompressing it (to memory) and comparing\n"
         "    the result to the original file. If the file fails the test, the first\n"
         "    mismatched position will be printed to screen.\n"
         "\n"
         "Optional switches:\n"
         "\n"
         "    -v\n"
         "    Print more detailed (verbose) information to screen.\n"
         "\n"
         "    -log LOGFILE\n"
         "    Logs (appends) compression results in the specified tab separated LOGFILE.\n"
         "    Logging is only applicable for compression.\n"
         "\n"
         "    -simd [NONE|SSE2|SSSE3|AVX2|NEON]\n"
         "    Overrides detected SIMD instruction set for neural network operations\n"
         "\n"
         "Remark: the command line arguments may be used in any order except the input\n"
         "and output: always the input comes first then output (which may be omitted).\n"
         "\n"
         "    Example:\n"
         "      " PROGNAME " -8 sequence.fasta folder/ -v -log logfile.txt -simd sse2\n"
         "    is equivalent to:\n"
         "      " PROGNAME " -v -simd sse2 sequence.fasta -log logfile.txt folder/ -8\n");
}

static void printSimdInfo(int simdIset, int detectedSimdIset) {
  printf("\nHighest SIMD vectorization support on this system: ");
  if( detectedSimdIset < 0 || detectedSimdIset > 11 ) {
    quit("Oops, sorry. Unexpected result.");
  }
  static const char *vectorizationString[12] = {"none", "MMX", "SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "AVX", "AVX2", "AVX512", "NEON"};
  printf("%s.\n", vectorizationString[detectedSimdIset]);

  printf("Using ");
  if (simdIset == 11) {
    printf("NEON");
  } else if( simdIset >= 9 ) {
    printf("AVX2");
  } else if (simdIset >= 5) {
    printf("SSE2 & SSSE3");
  } else if( simdIset >= 3 ) {
    printf("SSE2");
  } else {
    printf("non-vectorized");
  }
  printf(" neural network and hashtable functions.\n");
}

static void printCommand(const WHATTODO &whattodo) {
  printf(" To do          = ");
  if( whattodo == DoNone ) {
    printf("-");
  }
  if( whattodo == DoCompress ) {
    printf("Compress");
  }
  if( whattodo == DoExtract ) {
    printf("Extract");
  }
  if( whattodo == DoCompare ) {
    printf("Compare");
  }
  printf("\n");
}

static void printOptions(Shared *shared) {
  printf(" Level          = %d\n", shared->level);
}

auto processCommandLine(int argc, char **argv) -> int {
  ProgramChecker *programChecker = ProgramChecker::getInstance();
  Shared shared;
  try {

    if( !shared.toScreen ) { //we need a minimal feedback when redirected
      fprintf(stderr, PROGNAME " archiver v" PROGVERSION " (c) " PROGYEAR ", Matt Mahoney et al.\n");
    }
    printf(PROGNAME " archiver v" PROGVERSION " (c) " PROGYEAR ", Matt Mahoney et al.\n");

    // Print help message
    if( argc < 2 ) {
      printHelp();
      quit();
    }

    // Parse command line arguments
    WHATTODO whattodo = DoNone;
    bool verbose = false;
    int c = 0;
    int simdIset = -1; //simd instruction set to use

    FileName input;
    FileName output;
    FileName inputPath;
    FileName outputPath;
    FileName archiveName;
    FileName logfile;

    for( int i = 1; i < argc; i++ ) {
      int argLen = static_cast<int>(strlen(argv[i]));
      if( argv[i][0] == '-' ) {
        if( argLen == 1 ) {
          quit("Empty command.");
        }
        if( argv[i][1] >= '0' && argv[i][1] <= '9' ) { // first  digit of level
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          int level = argv[i][1] - '0';
          int j = 2;
          if (argLen >= 3 && argv[i][2] >= '0' && argv[i][2] <= '9') { // second digit of level
            level = level*10 + argv[i][2] - '0';
            j++;
          }
          if (level > 12) {
            quit("Compression level must be between 0 and 12.");
          }
          shared.init(level);
          whattodo = DoCompress;
          //process optional compression switches
          for( ; j < argLen; j++ ) {
            switch( argv[i][j] & 0xDFU ) {
              default: {
                printf("Invalid compression switch: %c", argv[1][j]);
                quit();
              }
            }
          }
        } else if( strcasecmp(argv[i], "-d") == 0 ) {
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          whattodo = DoExtract;
        } else if( strcasecmp(argv[i], "-t") == 0 ) {
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          whattodo = DoCompare;
        } else if( strcasecmp(argv[i], "-v") == 0 ) {
          verbose = true;
        } else if( strcasecmp(argv[i], "-log") == 0 ) {
          if( logfile.strsize() != 0 ) {
            quit("Only one logfile may be specified.");
          }
          if( ++i == argc ) {
            quit("The -log switch requires a filename.");
          }
          logfile += argv[i];
        }
        else if( strcasecmp(argv[i], "-simd") == 0 ) {
          if( ++i == argc ) {
            quit("The -simd switch requires an instruction set name (NONE,SSE2,SSSE3, AVX2, NEON).");
          }
          if( strcasecmp(argv[i], "NONE") == 0 ) {
            simdIset = 0;
          } else if( strcasecmp(argv[i], "SSE2") == 0 ) {
            simdIset = 3;
          } else if( strcasecmp(argv[i], "SSSE3") == 0 ) {
            simdIset = 5;
         } else if( strcasecmp(argv[i], "AVX2") == 0 ) {
            simdIset = 9;
         } else if (strcasecmp(argv[i], "NEON") == 0) {
            simdIset = 11;
          } else {
            quit("Invalid -simd option. Use -simd NONE, -simd SSE2, -simd SSSE3, -simd AVX2 or -simd NEON.");
          }
        } else {
          printf("Invalid command: %s", argv[i]);
          quit();
        }
      } else { //this parameter does not begin with a dash ("-") -> it must be a folder/filename
        if( input.strsize() == 0 ) {
          input += argv[i];
          input.replaceSlashes();
        } else if( output.strsize() == 0 ) {
          output += argv[i];
          output.replaceSlashes();
        } else {
          quit("More than two file names specified. Only an input and an output is needed.");
        }
      }
    }

    // Determine CPU's (and OS) support for SIMD vectorization instruction set
    int detectedSimdIset = simdDetect();
    if( simdIset == -1 ) {
      simdIset = detectedSimdIset;
    }
    if( simdIset > detectedSimdIset ) {
      printf("\nOverriding system highest vectorization support. Expect a crash.");
    }

    // Print anything only if the user wants/needs to know
    if( verbose || simdIset != detectedSimdIset ) {
      printSimdInfo(simdIset, detectedSimdIset);
    }

    // Set highest or user selected vectorization mode
    if (simdIset == 11) {
      shared.chosenSimd = SIMD_NEON;
    } else if (simdIset >= 9) {
      shared.chosenSimd = SIMD_AVX2;
    } else if (simdIset >= 5) {
      shared.chosenSimd = SIMD_SSSE3;
    } else if( simdIset >= 3 ) {
      shared.chosenSimd = SIMD_SSE2;
    } else {
      shared.chosenSimd = SIMD_NONE;
    }

    if( verbose ) {
      printf("\n");
      printf(" Command line   =");
      for( int i = 0; i < argc; i++ ) {
        printf(" %s", argv[i]);
      }
      printf("\n");
    }

    // Successfully parsed command line arguments
    // Let's check their validity
    if( whattodo == DoNone ) {
      quit("A command switch is required: -0..-12 to compress, -d to decompress, -t to test, -l to list.");
    }
    if( input.strsize() == 0 ) {
      printf("\nAn %s is required %s.\n", whattodo == DoCompress ? "input file or filelist" : "archive filename",
             whattodo == DoCompress ? "for compressing" : whattodo == DoExtract ? "for decompressing" : whattodo == DoCompare
                                                                                                        ? "for testing" : "");
      quit();
    }

    int pathType = 0;

    //Logfile supplied?
    if( logfile.strsize() != 0 ) {
      if( whattodo != DoCompress ) {
        quit("A log file may only be specified for compression.");
      }
      pathType = examinePath(logfile.c_str());
      if( pathType == 2 || pathType == 4 ) {
        quit("Specified log file should be a file, not a directory.");
      }
      if( pathType == 0 ) {
        printf("\nThere is a problem with the log file: %s", logfile.c_str());
        quit();
      }
    }

    // Separate paths from input filename/directory name
    pathType = examinePath(input.c_str());
    if( pathType == 2 || pathType == 4 ) {
      printf("\nSpecified input is a directory but should be a file: %s", input.c_str());
      quit();
    }
    if( pathType == 3 ) {
      printf("\nSpecified input file does not exist: %s", input.c_str());
      quit();
    }
    if( pathType == 0 ) {
      printf("\nThere is a problem with the specified input file: %s", input.c_str());
      quit();
    }
    if( input.lastSlashPos() >= 0 ) {
      inputPath += input.c_str();
      inputPath.keepPath();
      input.keepFilename();
    }

    // Separate paths from output filename/directory name
    if( output.strsize() > 0 ) {
      pathType = examinePath(output.c_str());
      if( pathType == 1 || pathType == 3 ) { //is an existing file, or looks like a file
        if( output.lastSlashPos() >= 0 ) {
          outputPath += output.c_str();
          outputPath.keepPath();
          output.keepFilename();
        }
      } else if( pathType == 2 || pathType == 4 ) {//is an existing directory, or looks like a directory
        outputPath += output.c_str();
        if( !outputPath.endsWith("/") && !outputPath.endsWith("\\")) {
          outputPath += GOODSLASH;
        }
        //output file is not specified
        output.resize(0);
        output.pushBack(0);
      } else {
        printf("\nThere is a problem with the specified output: %s", output.c_str());
        quit();
      }
    }

    //determine archive name
    if( whattodo == DoCompress ) {
      archiveName += outputPath.c_str();
      if( output.strsize() == 0 ) { // If no archive name is provided, construct it from input (append PROGNAME extension to input filename)
        archiveName += input.c_str();
        archiveName += "." PROGNAME PROGVERSION;
      } else {
        archiveName += output.c_str();
      }
    } else { // extract/compare/list: archivename is simply the input
      archiveName += inputPath.c_str();
      archiveName += input.c_str();
    }
    if( verbose ) {
      printf(" Archive        = %s\n", archiveName.c_str());
      printf(" Input folder   = %s\n", inputPath.strsize() == 0 ? "." : inputPath.c_str());
      printf(" Output folder  = %s\n", outputPath.strsize() == 0 ? "." : outputPath.c_str());
    }

    Mode mode = whattodo == DoCompress ? COMPRESS : DECOMPRESS;

    { //single file mode or extract/compare/list
      FileName fn(inputPath.c_str());
      fn += input.c_str();
      getFileSize(fn.c_str()); // Does file exist? Is it readable? (we don't actually need the file size now)
    }

    FileDisk archive;  // compressed file

    if( mode == DECOMPRESS ) {
      archive.open(archiveName.c_str(), true);
      // Verify archive header, get level and options
      int len = static_cast<int>(strlen(PROGNAME));
      for( int i = 0; i < len; i++ ) {
        if( archive.getchar() != PROGNAME[i] ) {
          printf("%s: not a valid %s file.", archiveName.c_str(), PROGNAME);
          quit();
        }
      }
      
      c = archive.getchar();
      uint8_t level = static_cast<uint8_t>(c);
      if (level > 12) {
        quit("Unexpected compression level setting in archive");
      }
      shared.init(level);
    }

    if( verbose ) {
      printCommand(whattodo);
      printOptions(&shared);
    }
    printf("\n");

    int numberOfFiles = 1; //default for single file mode

    // Write archive header to archive file
    if( mode == COMPRESS ) {
      printf("Creating archive %s ...\n", archiveName.c_str());
      archive.create(archiveName.c_str());
      archive.append(PROGNAME);
      archive.putChar(shared.level);
    }

    // In single file mode with no output filename specified we must construct it from the supplied archive filename
    { //single file mode
      if((whattodo == DoExtract || whattodo == DoCompare) && output.strsize() == 0 ) {
        output += input.c_str();
        const char *fileExtension = "." PROGNAME PROGVERSION;
        if( output.endsWith(fileExtension)) {
          output.stripEnd(static_cast<int>(strlen(fileExtension)));
        } else {
          printf("Can't construct output filename from archive filename.\nArchive file extension must be: '%s'", fileExtension);
          quit();
        }
      }
    }

    Encoder en(&shared, mode, &archive);
    uint64_t contentSize = 0;
    uint64_t totalSize = 0;

    // Compress list of files
    if( mode == COMPRESS ) {
      uint64_t start = en.size(); //header size (=8)
      if( verbose ) {
        printf("Writing header : %" PRIu64 " bytes\n", start);
      }
      totalSize += start;
    }

    // Compress or decompress files
    if( mode == COMPRESS ) {
      if( !shared.toScreen ) { //we need a minimal feedback when redirected
        fprintf(stderr, "Output is redirected - only minimal feedback is on screen\n");
      }
      { //single file mode
        FileName fn;
        fn += inputPath.c_str();
        fn += input.c_str();
        const char *fName = fn.c_str();
        uint64_t fSize = getFileSize(fName);
        if( !shared.toScreen ) { //we need a minimal feedback when redirected
          fprintf(stderr, "\nFilename: %s (%" PRIu64 " bytes)\n", fName, fSize);
        }
        printf("\nFilename: %s (%" PRIu64 " bytes)\n", fName, fSize);
        compressfile(&shared, fName, fSize, en, verbose);
        totalSize += fSize + 4; //4: file size information
        contentSize += fSize;
      }

      auto preFlush = en.size();
      en.flush();
      totalSize += en.size() - preFlush; //we consider padding bytes as auxiliary bytes
      printf("-----------------------\n");
      printf("Total input size     : %" PRIu64 "\n", contentSize);
      if( verbose ) {
        printf("Total metadata bytes : %" PRIu64 "\n", totalSize - contentSize);
      }
      printf("Total archive size   : %" PRIu64 "\n", en.size());
      printf("\n");
      // Log compression results
      if( logfile.strsize() != 0 ) {
        String results;
        pathType = examinePath(logfile.c_str());
        //Write header if needed
        if( pathType == 3 /*does not exist*/ ||
            (pathType == 1 && getFileSize(logfile.c_str()) == 0)/*exists but does not contain a header*/) {
          results += "PROG_NAME\tPROG_VERSION\tCOMMAND_LINE\tLEVEL\tINPUT_FILENAME\tORIGINAL_SIZE_BYTES\tCOMPRESSED_SIZE_BYTES\tRUNTIME_MS\n";
        }
        //Write results to logfile
        results += PROGNAME "\t" PROGVERSION "\t";
        for( int i = 1; i < argc; i++ ) {
          if( i != 0 ) {
            results += ' ';
          }
          results += argv[i];
        }
        results += "\t";
        results += uint64_t(shared.level);
        results += "\t";
        results += input.c_str();
        results += "\t";
        results += contentSize;
        results += "\t";
        results += en.size();
        results += "\t";
        results += uint64_t(programChecker->getRuntime() * 1000.0);
        results += "\t";
        results += "\n";
        appendToFile(logfile.c_str(), results.c_str());
        printf("Results logged to file '%s'\n", logfile.c_str());
        printf("\n");
      }
    } else { //decompress
      if( whattodo == DoExtract || whattodo == DoCompare ) {
        FMode fMode = whattodo == DoExtract ? FDECOMPRESS : FCOMPARE;
        { //single file mode
          FileName fn;
          fn += outputPath.c_str();
          fn += output.c_str();
          const char *fName = fn.c_str();
          decompressFile(&shared, fName, fMode, en);
        }
      }
    }

    archive.close();
    programChecker->print();
  }
    // we catch only the intentional exceptions from quit() to exit gracefully
    // any other exception should result in a crash and must be investigated
  catch( IntentionalException const & ) {
  }

  return 0;
}

#ifdef WINDOWS
#include "shellapi.h"
#pragma comment(lib,"shell32.lib")
#endif

auto main(int argc, char **argv) -> int {
#ifdef WINDOWS
  // On Windows, argv is encoded in the effective codepage, therefore unsuitable for acquiring command line arguments (file names
  // in our case) not representable in that particular codepage.
  // -> We will recreate argv as UTF8 (like in Linux)
  uint32_t oldcp = GetConsoleOutputCP();
  SetConsoleOutputCP(CP_UTF8);
  wchar_t **szArglist;
  int argc_utf8;
  char** argv_utf8;
  if( (szArglist = CommandLineToArgvW(GetCommandLineW(), &argc_utf8)) == NULL) {
    printf("CommandLineToArgvW failed\n");
    return 0;
  } else {
    if(argc!=argc_utf8)quit("Error preparing command line arguments.");
    argv_utf8=new char*[argc_utf8+1];
    for(int i=0; i<argc_utf8; i++) {
      wchar_t *s=szArglist[i];
      int buffersize = WideCharToMultiByte(CP_UTF8,0,s,-1,NULL,0,NULL,NULL);
      argv_utf8[i] = new char[buffersize];
      WideCharToMultiByte(CP_UTF8,0,s,-1,argv_utf8[i],buffersize,NULL,NULL);
      //printf("%d: %s\n", i, argv_utf8[i]); //debug: see if conversion is successful
    }
    argv_utf8[argc_utf8]=nullptr;
    int retval=processCommandLine(argc_utf8, argv_utf8);
    for(int i=0; i<argc_utf8; i++)
      delete[] argv_utf8[i];
    delete[] argv_utf8;
    SetConsoleOutputCP(oldcp);
    return retval;
  }
#else
  return processCommandLine(argc, argv);
#endif
}
