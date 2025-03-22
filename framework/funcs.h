#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"
#include "joystick.h"
#include <SDL2/SDL.h>
#include <functional>

namespace functions {

struct Mappings {
    std::unordered_map<Buttons, ButtonState> &buttonState;
    const std::unordered_map<Buttons, std::function<std::pair<int, int>()>> &input_to_mouse_move = {};
    const std::unordered_map<Buttons, std::function<std::pair<int, int>()>> &release_to_mouse_move = {};
    const std::unordered_map<Buttons, std::function<int()>> &input_to_mouse_click = {};
    const std::unordered_map<Buttons, std::function<int()>> &release_to_mouse_click = {};
    const std::unordered_map<Buttons, std::function<int()>> &input_to_button_toggle = {};
    const std::unordered_map<Buttons, std::function<int()>> &release_to_button_toggle = {};
    const std::unordered_map<Buttons, std::function<int()>> &input_to_key_tap = {};
    const std::unordered_map<Buttons, std::function<int()>> &release_to_key_tap = {};
    const std::unordered_map<Buttons, std::function<int()>> &input_to_key_hold = {};
    const std::unordered_map<ButtonGroups, std::function<Joystick &()>> &joystick_to_mouse_relative = {};
    const std::unordered_map<Buttons, std::function<bool()>> &input_to_conditioning_logic = {};
    const std::unordered_map<Buttons, std::function<void()>> &input_to_logic_before = {};
    const std::unordered_map<Buttons, std::function<void()>> &release_to_logic_before = {};
    const std::unordered_map<Buttons, std::function<void()>> &input_to_logic_after = {};
    const std::unordered_map<Buttons, std::function<void()>> &release_to_logic_after = {};
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

/// @brief to be called by a game after setting up its state.
/// @param gameParams loop params.
void run(GameParams &gameParams);

namespace action {

/// @brief runs the handler you would expect for each input in each map.
/// @param mappings mappings.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
/// @param turboInputs set of inputs for which to run the handler on press, not just on just pressed.
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

/// @brief moves the cursor to the specified position in input_to_mouse_move or release_to_mouse_move map if the input state matches eventType.
/// Can execute a callback if defined in the corresponding logic before map. This callback is boolean and can stop the action.
/// @param mappings mappings.
/// @param input controller button linked to the click and key in the map.
/// @param eventType to match for the mouse to move to the mapped location. Can be any.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
void handleToMouseAbsoluteMove(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const double resScalingX, const double resScalingY);

/// @brief  clicks the button if the input state matches eventType.
/// Can execute a callback if defined in the corresponding logic before map. This callback is boolean and can stop the action.
/// @param mappings mappings.
/// @param input controller button linked to the click.
/// @param eventType to match for the mouse button to be clicked. Can be any.
/// @param button mouse button to click. If none is provided it will be obtained from the corresponding maps.
void handleToClick(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int button = -1);

/// @brief toggles the button if the input state matches eventType.
/// Can execute a callback if defined in the corresponding logic before map. This callback is boolean and can stop the action.
/// @param mappings mappings.
/// @param input controller button linked to the mouse button toggle.
/// @param eventType to match for the mouse button to be toggled. Can be any.
/// @param button mouse button to toggle. If none is provided it will be obtained from the corresponding maps.
void handleToButtonToggle(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int button = -1);

/// @brief taps the key if the input state matches eventType.
/// Can execute a callback if defined in the corresponding logic before map. This callback is boolean and can stop the action.
/// @param mappings mappings.
/// @param input controller button linked to the key tap.
/// @param eventType to match for the key to be tapped. Can be any.
/// @param key key to tap. If none is provided it will be obtained from the corresponding maps.
void handleToKeyTap(const Mappings &mappings, const Buttons &input, const ButtonState eventType, const int key = -1);

/// @brief holds the key while the input stays pressed.
/// Can execute a callback if defined in the corresponding logic before map. This callback is boolean and can stop the action.
/// @param mappings mappings.
/// @param input controller button linked to the key hold.
/// @param key key to hold. If none is provided it will be obtained from the corresponding maps.
void handleToKeyHold(const Mappings &mappings, const Buttons &input, const int key = -1);

/// @brief moves the mouse based on joystick sensitivity and joystick input if joystick is active.
/// @param mappings mappings.
/// @param input controller button representing half a joystick axis.
/// @param joystick joystick data to read.
/// @param resScalingX horizontal resolution factor over 1080p.
/// @param resScalingY vertical resolution factor over 1080p.
void handleJoystickToMouseRelative(const Mappings &mappings, const ButtonGroups &input, const Joystick &joystick, const double resScalingX, const double resScalingY);

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
/// @param second_input_delay_mills duration for the buffer to allow an action from an input after a first action recorded in the buffer.
/// @param subsequent_inputs_delay_millis duration for the buffer to allow an action from an input after a non-first action recorded in the buffer.
/// @param input to debounce.
/// @param BufferState a buffer state that can be specific to the button, to a set of buttons or shared across all buttons.
/// @return true if the buffer is free, false otherwise.
bool isBufferFree(const std::unordered_map<Buttons, ButtonState> &buttonState, const int second_input_delay_mills, const int subsequent_inputs_delay_millis, const Buttons &input, BufferState &BufferState);

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
