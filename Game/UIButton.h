#pragma once
#include <string>

struct UIButton {
    float x, y;       // Position
    float w, h;       // Size
    std::string text;
    float r, g, b;    // Base Color
    
    // State Flags (The System will update these)
    bool isHovered = false;
    bool isClicked = false; 
    bool isDown = false; // True while held down
    bool isDisabled = false; // True when button should be grayed out
};