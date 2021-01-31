#ifndef PAQ8GEN_SSE_HPP
#define PAQ8GEN_SSE_HPP

#include "APM.hpp"
#include "APM1.hpp"
#include "APMPost.hpp"
#include "Hash.hpp"

/**
 * Filter the context model with APMs
 */
class SSE {
private:
    Shared * const shared;
    APM APMs[4];
    APM1 APM1s[3];
    APMPost apmPost;

public:
    explicit SSE(Shared* const sh);
    auto p(int pr0) -> int;
};

#endif //PAQ8GEN_SSE_HPP
