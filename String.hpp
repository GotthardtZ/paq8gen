#ifndef PAQ8GEN_STRING_HPP
#define PAQ8GEN_STRING_HPP

#include "Array.hpp"

/**
 * A specialized string class
 */
class String : public Array<char> {
private:
    void appendIntRecursive(uint64_t x);
protected:
#ifdef NDEBUG
#define chk_consistency() ((void) 0)
#else

    void chk_consistency() const {
      for( uint32_t i = 0; i < size() - 1; i++ ) {
        if((*this)[i] == 0 ) {
          quit("Internal error - string consistency check failed (1).");
}
}
      if(((*this)[size() - 1]) != 0 ) {
        quit("Internal error - string consistency check failed (2).");
}
    }

#endif
public:
    [[nodiscard]] auto c_str() const -> const char *;
    /**
     * Does not include the NUL terminator
     * @return length of the string
     */
    [[nodiscard]] auto strsize() const -> uint64_t;
    void operator=(const char *s);
    void operator+=(const char *s);
    void operator+=(char c);
    void operator+=(uint64_t x);
    auto endsWith(const char *ending) const -> bool;
    void stripEnd(uint64_t count);
    auto beginsWith(const char *beginning) const -> bool;
    void stripStart(uint64_t count);
    [[nodiscard]] auto findLast(char c) const -> int;
    explicit String(const char *s = "");
};

#endif //PAQ8GEN_STRING_HPP
