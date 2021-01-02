#ifndef PAQ8GEN_DIVISIONTABLE_HPP
#define PAQ8GEN_DIVISIONTABLE_HPP

/**
 * This class provides a static (common) 1024-element lookup table for integer division
 * Initialization will run multiple times, but the table is created only once
 * @todo Split into declaration and definition
 */
class DivisionTable {
public:
    static auto getDT() -> int * {
      static int dt[1024]; // i -> 16K/(i+i+3)
      for( int i = 0; i < 1024; ++i ) {
        dt[i] = 16384 / (i + i + 3);
      }
      return dt;
    }
};

#endif //PAQ8GEN_DIVISIONTABLE_HPP
