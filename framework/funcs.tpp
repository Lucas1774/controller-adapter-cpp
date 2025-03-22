#include "funcs.h"

namespace functions::abstractStateUtils {

template <size_t Elements>
bool computeAdjacencyMatrixBasedTarget(
    const std::array<std::array<Buttons, Elements>, Elements> &adjacencyMatrix, int &index, const Buttons button) {
    for (int i = 0; i < Elements; ++i) {
        if (adjacencyMatrix[index][i] == button) {
            index = i;
            return true;
        }
    }
    return false;
}

} // namespace functions::abstractStateUtils
