#pragma once
#include <string>

struct UIButton {
    float x, y;       
    float w, h;      
    std::string text;
    float r, g, b;    
    
    bool isHovered = false;
    bool isClicked = false; 
    bool isDown = false; 
    bool isDisabled = false;
};