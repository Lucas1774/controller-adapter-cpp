#include "funcs.h"

template <size_t Elements>
bool Functions::computeAdjacencyMatrixBasedMouseTarget(
    const std::array<std::array<int, Elements>, Elements> &adjacencyMatrix,
    const std::array<std::pair<int, int>, Elements> &coordinates,
    std::pair<int, int> &newCoordinates, int &index, const int button,
    const double resScalingX, const double resScalingY) const {
    for (int i = 0; i < Elements; ++i) {
        if (adjacencyMatrix[index][i] == button) {
            index = i;
            newCoordinates = {
                coordinates[i].first * resScalingX,
                coordinates[i].second * resScalingY};
            return true;
        }
    }
    return false;
}

template <size_t Rows, size_t Cols>
bool Functions::computeGridBasedMouseTarget(
    const std::array<std::array<std::pair<int, int>, Cols>, Rows> &coordinates,
    std::pair<int, int> &newCoordinates, int &rowIndex, int &columnIndex, const int button,
    const double resScalingX, const double resScalingY) const {
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
    default:
        break;
    }
    newCoordinates = {
        coordinates[rowIndex][columnIndex].first * resScalingX,
        coordinates[rowIndex][columnIndex].second * resScalingY};
    return true;
}