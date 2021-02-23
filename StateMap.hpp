#ifndef PAQ8GEN_STATEMAP_HPP
#define PAQ8GEN_STATEMAP_HPP

#include "DivisionTable.hpp"
#include "AdaptiveMap.hpp"
#include "StateTable.hpp"
#include "UpdateBroadcaster.hpp"

/**
 * A @ref StateMap maps a context to a probability.
 */
class StateMap : public AdaptiveMap {
protected:
    const uint32_t numContextSets; /**< Number of context sets */
    const uint32_t numContextsPerSet; /**< Number of contexts in each context set */
    uint32_t numContexts; /**< Number of context indexes present in cxt array (0..s-1) */
    Array<uint32_t> cxt; /**< context index of last prediction per context set */
public:
    enum MAPTYPE {
        Generic, BitHistory, Run
    };

    /**
     * Creates a @ref StateMap with @ref n contexts using 4*n bytes memory.
     * @param s
     * @param n number of contexts
     * @param lim
     * @param mapType
     */
    StateMap(const Shared* sh, int s, int n, int lim, MAPTYPE mapType);

    void update() override;

    /**
     * Call @ref p1() when there is only 1 context set.
     * No need to call @ref subscribe().
     * @param cx
     * @return
     */
    auto p1(uint32_t cx) -> int;

    /**
     * Call @ref p2() for each context when there is more context sets and call @ref subscribe() once.
     * sm.p(y, cx, limit) converts state @ref cx (0..n-1) to a probability (0..4095)
     * that the next y=1, updating the previous prediction with y (0..1).
     * limit (1..1023, default 1023) is the maximum count for computing a
     * prediction.  Larger values are better for stationary sources.
     * @param s
     * @param cx
     * @return
     */
    auto p2(uint32_t s, uint32_t cx) -> int;
    void subscribe();

    /**
     * Call @ref skip() instead of @ref p2() when the context is unknown or uninteresting.
     * @remark no need to call @ref skip() when there is only 1 context or all contexts will be skipped.
     * @param s
     */
    void skip(uint32_t s);
#ifndef CHALLENGE
    void print() const;
#endif
};

#endif //PAQ8GEN_STATEMAP_HPP
