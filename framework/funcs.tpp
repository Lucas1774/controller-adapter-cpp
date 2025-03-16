#include "funcs.h"

template <typename Target, size_t Elements>
bool Functions::computeAdjacencyMatrixBasedTarget(
    const std::array<std::array<int, Elements>, Elements> &adjacencyMatrix,
    const std::array<Target, Elements> &targets,
    Target &newTargets, int &index, const int button) const {
    for (int i = 0; i < Elements; ++i) {
        if (adjacencyMatrix[index][i] == button) {
            index = i;
            newTargets = targets[i];
            return true;
        }
    }
    return false;
}

template <typename Target, size_t Rows, size_t Cols>
bool Functions::computeGridBasedTarget(
    const std::array<std::array<Target, Cols>, Rows> &targets,
    Target &newTargets, int &rowIndex, int &columnIndex, const int button) const {
    switch (button) {
    case PAD_UP:
        if (1 == Rows) {
            return false;
        }
        rowIndex = (rowIndex + Rows - 1) % Rows;
        break;
    case PAD_DOWN:
        if (1 == Rows) {
            return false;
        }
        rowIndex = (rowIndex + 1) % Rows;
        break;
    case PAD_LEFT:
        if (1 == Cols) {
            return false;
        }
        columnIndex = (columnIndex + Cols - 1) % Cols;
        break;
    case PAD_RIGHT:
        if (1 == Cols) {
            return false;
        }
        columnIndex = (columnIndex + 1) % Cols;
        break;
    default: // go to the next target. Hacky but handy. The correct way would be to define a normal adjacency map.
        columnIndex = (columnIndex + 1) % Cols;
        break;
    }
    newTargets = targets[rowIndex][columnIndex];
    return true;
}