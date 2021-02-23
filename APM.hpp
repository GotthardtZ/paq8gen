#ifndef PAQ8GEN_APM_HPP
#define PAQ8GEN_APM_HPP

#include "AdaptiveMap.hpp"
#include "Squash.hpp"
#include "Stretch.hpp"
#include "UpdateBroadcaster.hpp"

/**
 * An APM maps a probability and a context to a new probability.
 */
class APM : public AdaptiveMap {
private:
    const int N; /**< Number of contexts */
    const int steps;
    int cxt; /**< context index of last prediction */
#ifdef CHALLENGE
    Stretch * stretch = &Stretch::getInstance();
#endif
public:
    /**
     * Creates with @ref n contexts using 4*s*n bytes memory.
     * @param n the number of contexts
     * @param s the number of steps
     */
    APM(const Shared* sh, int n, int s);

    /**
     * a.update() updates probability map. y=(0..1) is the last bit
     */
    void update() override;

    /**
     * Returns a new probability (0..4095) like with @ref StateMap.
     * cx=(0..n-1) is the context.
     * pr=(0..4095) is considered part of the context.
     * The output is computed by interpolating @ref pr into @ref steps ranges nonlinearly
     * with smaller ranges near the ends. The initial output is pr.
     * lim=(0..1023): set a lower limit (like 255) for faster adaptation.
     * @param pr the initial probability. Considered part of the context.
     * @param cx the context
     * @param lim limit; lower limit means faster adaptation
     * @return the adjusted probability
     */
    int p(int pr, int cx, int lim);
};

#endif //PAQ8GEN_APM_HPP
