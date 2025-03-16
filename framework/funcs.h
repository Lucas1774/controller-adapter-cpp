#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "constants.h"
#include "joystick.h"
#include <SDL2/SDL.h>
#include <functional>
#include <windows.h>

class Functions {
  public:
    void setMaps(std::unordered_map<int, int> *buttonState,
                 const std::unordered_map<int, std::function<std::pair<int, int>()>> *input_to_mouse_move,
                 const std::unordered_map<int, std::function<std::pair<int, int>()>> *release_to_mouse_move,
                 const std::unordered_map<int, std::function<int()>> *input_to_mouse_click,
                 const std::unordered_map<int, std::function<int()>> *release_to_mouse_click,
                 const std::unordered_map<int, std::function<int()>> *input_to_button_toggle,
                 const std::unordered_map<int, std::function<int()>> *release_to_button_toggle,
                 const std::unordered_map<int, std::function<WORD()>> *input_to_key_tap,
                 const std::unordered_map<int, std::function<WORD()>> *release_to_key_tap,
                 const std::unordered_map<int, std::function<WORD()>> *input_to_key_hold,
                 const std::unordered_map<int, std::function<bool()>> *input_to_logic_before,
                 const std::unordered_map<int, std::function<bool()>> *input_to_logic_after,
                 const std::unordered_map<int, std::function<bool()>> *release_to_logic_before,
                 const std::unordered_map<int, std::function<bool()>> *release_to_logic_after);

    /// @brief Moves cursor to the specified position.
    /// @param x x coordinate.
    /// @param y y coordinate.
    void moveMouse(const int x, const int y, const double resScalingX, const double resScalingY) const;

    /// @brief moves the cursor relative to the current position.
    /// @param x x delta.
    /// @param y y delta.
    void moveMouseRelative(const int x, const int y, const double resScalingX, const double resScalingY) const;

    /// @brief sends a simple click.
    /// @param button mouse button to click.
    /// @param callback to be executed AFTER the click.
    void click(const int button, const std::function<void()> &callback = nullptr) const;

    /// @brief moves the cursor to the specified position in input_to_mouse_move or release_to_mouse_move map if the input state matches eventType.
    /// Can execute callbacks if defined in the corresponding logic before/after maps. Before callback is boolean and can stop the action.
    /// @param input controller button linked to the click and key in the map.
    /// @param eventType to match for the mouse to move to the mapped location. Can be any.
    void handleToMouseAbsoluteMove(const int &input, const int eventType, const double resScalingX, const double resScalingY) const;

    /// @brief  clicks the button if the input state matches eventType.
    /// Can execute callbacks if defined in the corresponding logic before/after maps. Before callback is boolean and can stop the action.
    /// @param input controller button linked to the click.
    /// @param eventType to match for the mouse button to be clicked. Can be any.
    /// @param button mouse button to click. If none is provided it will be obtained from the corresponding maps. 
    void handleToClick(const int &input, const int eventType, const int button = -1) const;

    /// @brief toggles the button if the input state matches eventType.
    /// Can execute callbacks if defined in the corresponding logic before/after maps. Before callback is boolean and can stop the action.
    /// @param input controller button linked to the mouse button toggle.
    /// @param eventType to match for the mouse button to be toggled. Can be any.
    /// @param button mouse button to toggle. If none is provided it will be obtained from the corresponding maps. 
    void handleToButtonToggle(const int &input, const int eventType, const int button = -1) const;

    /// @brief taps the key if the input state matches eventType.
    /// Can execute callbacks if defined in the corresponding logic before/after maps. Before callback is boolean and can stop the action.
    /// @param input controller button linked to the key tap.
    /// @param eventType to match for the key to be tapped. Can be any.
    /// @param key key to tap. If none is provided it will be obtained from the corresponding maps. 
    void handleToKeyTap(const int &input, const int eventType, const int key = -1) const;

    /// @brief holds the key while the input stays pressed.
    /// Can execute callbacks if defined in the corresponding logic before/after maps. Before callback is boolean and can stop the action.
    /// @param input controller button linked to the key hold.
    /// @param key key to hold. If none is provided it will be obtained from the corresponding maps. 
    void handleToKeyHold(const int &input, const int key = -1) const;

    /// @brief looks for activate button press to start the program. The activate button can be mapped in the config file.
    /// @param events event pool.
    /// @param buttonMapping to relate the event button id to the button id.
    /// @param running reference to boolean to control the program loop.
    void listenToRunEvent(const std::vector<SDL_Event> &events, const std::unordered_map<int, int> &buttonMapping, bool &running) const;

    /// @brief updates button state with the next transition logic:
    /// ```
    /// | Previous State | is_pressed == true | is_pressed == false |
    /// |----------------|--------------------|-------------------- |
    /// | JUST_RELEASED  | JUST_PRESSED       | RELEASED            |
    /// | RELEASED       | JUST_PRESSED       | RELEASED            |
    /// | JUST_PRESSED   | PRESSED            | JUST_RELEASED       |
    /// | PRESSED        | PRESSED            | JUST_RELEASED       |
    /// ```
    /// @param events event pool.
    /// @param buttonMapping to relate the event button ids to the button ids.
    void updateNonAnalogState(const std::vector<SDL_Event> &events, const std::unordered_map<int, int> &buttonMapping) const;

    /// @brief updates linked virtual buttons for actual joysticks and L2 and R2 for triggers.
    /// It should be combined with high deadzone values, at least for actual joysticks where both axis sit on the same physical element.
    /// @param joystick the joystick connection to update its buttons for.
    /// @param joystickMeta joystick data to update.
    /// @param type to help the program know which joystick is being passed in an efficient way.
    void updateJoystickAsDigital(SDL_Joystick *joystick, Joystick &joystickMeta, const ButtonGroups type) const;

    /// @brief updates joystick or trigger data, without updating buttons related to the joystick.
    /// @param joystick the joystick connection to update its data for.
    /// @param joystickMeta joystick data to update.
    /// @param type to help the program know which joystick is being passed in an efficient way.
    void updateJoystickAsAnalog(SDL_Joystick *joystick, Joystick &joystickMeta, const ButtonGroups type) const;

    /// @brief Implements a debounce mechanism to improve performance or input rhythm.
    /// Should be used as if (isBufferFree()) { // Update state to trigger action }.
    /// @param second_input_delay_mills duration for the buffer to allow an action from an input after a first action recorded in the buffer.
    /// @param subsequent_inputs_delay_millis duration for the buffer to allow an action from an input after a non-first action recorded in the buffer.
    /// @param button to debounce.
    /// @param BufferState a buffer state that can be specific to the button, to a set of buttons or shared across all buttons.
    /// @return true if the buffer is free, false otherwise.
    bool isBufferFree(const int second_input_delay_mills, const int subsequent_inputs_delay_millis, const int &button, BufferState &BufferState) const;

    /// @brief gives an index for an 8-axis target, effectively creating virtual, diagonal buttons.
    /// @param eightAxis the element to obtain the index for. Can be left joystick, right joystick, or D-PAD.
    /// @return the index of the pressed virtual button, where 0 is the bottom-left button and 7 is the bottom one.
    int generateAxisTargetWithBitMask(const ButtonGroups eightAxis) const;

    /// @brief returns coordinates for a mouse target based on an adjacency matrix to update dynamic state in callback before input action.
    /// Should be used in callback before input maps for inputs mapped to dynamic target mouse movements.
    /// @tparam Elements size of the coordinate array.
    /// @param adjacencyMatrix adjacency matrix to determine the next target based on the current target and the input button.
    /// @param coordinates coordinate array.
    /// @param newCoordinates to be updated by the function.
    /// @param index current index in the coordinate array. To be updated by the function.
    /// @param button input button to determine the next target.
    /// @return true if a mouse target update was made and thus a mouse move call should be made.
    template <size_t Elements>
    bool computeAdjacencyMatrixBasedMouseTarget(
        const std::array<std::array<int, Elements>, Elements> &adjacencyMatrix,
        const std::array<std::pair<int, int>, Elements> &coordinates,
        std::pair<int, int> &newCoordinates, int &index, const int button) const;

    /// @brief returns coordinates for a mouse target based on a 2D coordinate grid to update dynamic state in callback before input action.
    /// @tparam Rows rows of the coordinate grid.
    /// @tparam Cols columns of the coordinate grid.
    /// @param coordinates coordinate grid.
    /// @param newCoordinates to be updated by the function.
    /// @param rowIndex current row index in the grid. To be updated by the function.
    /// @param columnIndex current column index in the grid. To be updated by the function.
    /// @param button input button to determine the next target
    /// @return true if a mouse target update was made and thus a mouse move call should be made.
    template <size_t Rows, size_t Cols>
    bool computeGridBasedMouseTarget(
        const std::array<std::array<std::pair<int, int>, Cols>, Rows> &coordinates,
        std::pair<int, int> &newCoordinates, int &rowIndex, int &columnIndex, const int button) const;

  private:
    const std::unordered_map<int, DWORD> BUTTON_ID_TO_PRESS_EVENT = {
        {SDL_BUTTON_LEFT, MOUSEEVENTF_LEFTDOWN},
        {SDL_BUTTON_RIGHT, MOUSEEVENTF_RIGHTDOWN},
        {SDL_BUTTON_MIDDLE, MOUSEEVENTF_MIDDLEDOWN}};
    const std::unordered_map<int, DWORD> BUTTON_ID_TO_RELEASE_EVENT = {
        {SDL_BUTTON_LEFT, MOUSEEVENTF_LEFTUP},
        {SDL_BUTTON_RIGHT, MOUSEEVENTF_RIGHTUP},
        {SDL_BUTTON_MIDDLE, MOUSEEVENTF_MIDDLEUP}};
    void sendInput(const int key, const DWORD flags) const;
    bool actionCallback(const int &input, const bool on_press, const bool before) const;
    void pressButton(const int button_to_click, const std::function<void()> &callback = nullptr) const;
    void releaseButton(const int button_to_release, const std::function<void()> &callback = nullptr) const;
    void pressThenRelease(const int key_to_tap, const std::function<void()> &callback = nullptr) const;
    void handleState(int &state, const bool is_pressed) const;
    std::unordered_map<int, int> *buttonState;
    const std::unordered_map<int, std::function<std::pair<int, int>()>> *input_to_mouse_move;
    const std::unordered_map<int, std::function<std::pair<int, int>()>> *release_to_mouse_move;
    const std::unordered_map<int, std::function<int()>> *input_to_mouse_click;
    const std::unordered_map<int, std::function<int()>> *release_to_mouse_click;
    const std::unordered_map<int, std::function<int()>> *input_to_button_toggle;
    const std::unordered_map<int, std::function<int()>> *release_to_button_toggle;
    const std::unordered_map<int, std::function<WORD()>> *input_to_key_tap;
    const std::unordered_map<int, std::function<WORD()>> *release_to_key_tap;
    const std::unordered_map<int, std::function<WORD()>> *input_to_key_hold;
    const std::unordered_map<int, std::function<bool()>> *input_to_logic_before;
    const std::unordered_map<int, std::function<bool()>> *input_to_logic_after;
    const std::unordered_map<int, std::function<bool()>> *release_to_logic_before;
    const std::unordered_map<int, std::function<bool()>> *release_to_logic_after;
};

#include "funcs.tpp"

#endif // FUNCTIONS_H
