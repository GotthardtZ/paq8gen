#ifndef PAQ8GEN_ADAPTIVEMAP_HPP
#define PAQ8GEN_ADAPTIVEMAP_HPP

#include "IPredictor.hpp"
#include <cstdint>
#include "Shared.hpp"
#include "DivisionTable.hpp"

/**
 * This is the base class for StateMap and APM.
 * Purpose: common members are here
 */
class AdaptiveMap : protected IPredictor {
protected:
    const Shared * const shared;
    Array<uint32_t> t; /**< cxt -> prediction in high 22 bits, count in low 10 bits */
    int limit;
    int *dt; /**< Pointer to division table */
    AdaptiveMap(const Shared* const sh, int n, int lim);
    ~AdaptiveMap() override = default;
    void update(uint32_t *p);
public:
    void setLimit(int lim);
};

#endif //PAQ8GEN_ADAPTIVEMAP_HPP
