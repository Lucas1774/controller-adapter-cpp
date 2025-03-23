#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"
#include "joystick.h"
#include <SDL2/SDL.h>
#include <functional>

namespace functions {

struct Mappings {
    std::unordered_map<Buttons, ButtonState> &buttonState;
    const std::unordered_map<Buttons, std::function<std::pair<int, int>()>> &inputToMouseMove = {};
    const std::unordered_map<Buttons, std::function<std::pair<int, int>()>> &releaseToMouseMove = {};
    const std::unordered_map<Buttons, std::function<int()>> &inputToMouseClick = {};
    const std::unordered_map<Buttons, std::function<int()>> &releaseToMouseClick = {};
    const std::unordered_map<Buttons, std::function<int()>> &inputToButtonToggle = {};
    const std::unordered_map<Buttons, std::function<int()>> &releaseToButtonToggle = {};
    const std::unordered_map<Buttons, std::function<int()>> &inputToKeyTap = {};
    const std::unordered_map<Buttons, std::function<int()>> &releaseToKeyTap = {};
    const std::unordered_map<Buttons, std::function<int()>> &inputToKeyHold = {};
    const std::unordered_map<ButtonGroups, std::function<Joystick &()>> &joystickToMouseRelative = {};
    const std::unordered_map<Buttons, std::function<bool()>> &inputToConditioningLogic = {};
    const std::unordered_map<Buttons, std::function<void()>> &inputToLogicBefore = {};
    const std::unordered_map<Buttons, std::function<void()>> &releaseToLogicBefore = {};
    const std::unordered_map<Buttons, std::function<void()>> &inputToLogicAfter = {};
    const std::unordered_map<Buttons, std::function<void()>> &releaseToLogicAfter = {};
};

struct GameParams {
    const std::unordered_map<Uint8, Buttons> &buttonMapping;
    std::unordered_map<Buttons, ButtonState> &buttonState;
    SDL_Joystick *joystick;
    Joystick &leftJoystick;
    Joystick &rightJoystick;
    Joystick *triggers;
    Mappings &mappings;
    const double resScalingX;
    const double resScalingY;
    bool running;
    const std::unordered_set<Buttons> &turboInputs;
};

/// @brief to be called by a game after setting up its game state and configuration.
/// @param gameParams loop params.
void run(const GameParams &gameParams);

namespace action {

/// @brief processes all input maps.
/// @param mappings mappings.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
/// @param turboInputs set of inputs to process on press, not just on just pressed.
void runMappings(const Mappings &mappings, double resScalingX = 1.0, double resScalingY = 1.0, const std::unordered_set<Buttons> &turboInputs = {});

/// @brief Moves cursor to the specified position.
/// @param x x coordinate.
/// @param y y coordinate.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
void moveMouse(const int x, const int y, const double resScalingX, const double resScalingY);

/// @brief moves the cursor relative to the current position.
/// @param x x delta.
/// @param y y delta.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
void moveMouseRelative(const int x, const int y, const double resScalingX, const double resScalingY);

/// @brief sends a simple click.
/// @param button mouse button to click.
void click(const int button);

} // namespace action

namespace state {

/// @brief updates button state with the next transition logic:
/// ```
/// | Previous State | is_pressed == true | is_pressed == false |
/// |----------------|--------------------|-------------------- |
/// | JUST_RELEASED  | JUST_PRESSED       | RELEASED            |
/// | RELEASED       | JUST_PRESSED       | RELEASED            |
/// | JUST_PRESSED   | PRESSED            | JUST_RELEASED       |
/// | PRESSED        | PRESSED            | JUST_RELEASED       |
/// ```
/// @param buttonState button state.
/// @param events event pool.
/// @param buttonMapping to relate the event button ids to the button ids.
void updateNonAnalogState(std::unordered_map<Buttons, ButtonState> &buttonState, const std::vector<SDL_Event> &events, const std::unordered_map<Uint8, Buttons> &buttonMapping);

/// @brief updates linked virtual buttons for actual joysticks and L2 and R2 for triggers.
/// It should be combined with high deadzone values, at least for actual joysticks where both axis sit on the same physical element.
/// @param buttonState button state.
/// @param joystick the joystick connection to update its buttons for.
/// @param joystickMeta joystick data to update.
/// @param type to help the program know which joystick is being passed in an efficient way.
void updateJoystick(std::unordered_map<Buttons, ButtonState> &buttonState, SDL_Joystick *joystick, Joystick &joystickMeta, const ButtonGroups type);

} // namespace state

namespace abstractStateUtils {

/// @brief Implements a debounce mechanism to improve performance or input rhythm.
/// Should be used as if (isBufferFree()) { // Update state to trigger action }.
/// @param buttonState button state.
/// @param secondInputDelayMillis duration for the buffer to allow an action from an input after a first action recorded in the buffer.
/// @param subsequentInputDelayMillis duration for the buffer to allow an action from an input after a non-first action recorded in the buffer.
/// @param input to debounce.
/// @param BufferState a buffer state that can be specific to the button, to a set of buttons or shared across all buttons.
/// @return true if the buffer is free, false otherwise.
bool isBufferFree(const std::unordered_map<Buttons, ButtonState> &buttonState, const int secondInputDelayMillis, const int subsequentInputDelayMillis, const Buttons &input, BufferState &BufferState);

/// @brief gives an index for an 8-axis target, effectively creating virtual, diagonal buttons.
/// @param buttonState button state.
/// @param eightAxis the element to obtain the index for. Can be left joystick, right joystick, or D-PAD.
/// @return the index of the pressed virtual button, where 0 is the bottom-left button and 7 is the bottom one.
int generateAxisTargetWithBitMask(std::unordered_map<Buttons, ButtonState> &buttonState, const ButtonGroups eightAxis);

/// @brief updates array index to control state.
/// Should be used in callback before or after input maps for inputs mapped to dynamic targets defined at the array the index references.
/// @tparam Elements size of the target array.
/// @param adjacencyMatrix adjacency matrix to determine the next target based on the current target and the input button.
/// @param index current index in the array. To be updated by the function.
/// @param button input button to determine the next target in the array.
/// @return true if a target update was made and thus a call should be made.
template <size_t Elements>
bool computeAdjacencyMatrixBasedTarget(const std::array<std::array<Buttons, Elements>, Elements> &adjacencyMatrix, int &index, const Buttons button);

/// @brief updates orthogonal matrix indexes to control state.
/// Should be used in callback before or after input maps for inputs mapped to dynamic targets defined at the matrix the indexes reference.
/// @param rowCount matrix width.
/// @param columnCount matrix height.
/// @param rowIndex current row index in the matrix. To be updated by the function.
/// @param columnIndex current column index in the matrix. To be updated by the function.
/// @param input input button to determine the next target in the grid.
/// @return true if a target update was made and thus a call should be made.
bool computeGridBasedTarget(const int rowCount, const int columnCount, int &rowIndex, int &columnIndex, const Buttons input);

} // namespace abstractStateUtils

}; // namespace functions

#include "funcs.tpp"

#endif // FUNCTIONS_H
