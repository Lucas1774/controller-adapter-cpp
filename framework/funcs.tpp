#include "funcs.h"

namespace functions {

template <size_t Elements>
bool computeAdjacencyMatrixBasedTarget(
    const std::array<std::array<int, Elements>, Elements> &adjacencyMatrix, int &index, const int button) {
    for (int i = 0; i < Elements; ++i) {
        if (adjacencyMatrix[index][i] == button) {
            index = i;
            return true;
        }
    }
    return false;
}

} // namespace functions
