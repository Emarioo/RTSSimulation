#pragma once 

#include "Engone/UIModule.h"
#include "Engone/InputModule.h"

struct UISlider {
    enum SliderType : u32 {
        SLIDER_FLOAT = 1,
        SLIDER_I32,
    };
    
    float x=0.f,y=0.f,w=0.f,h=0.f;
    const char* sliderName = nullptr; // displayed?
    SliderType sliderType = SLIDER_FLOAT;
    int clickedMX = 0, clickedMY = 0;
    float clickedValue = 0.5f;
    bool selected=false;
    char textStorage[256]; // temporary
    union {
        struct {
            int min, max;
            int* sliderValue;
        } inty;
        struct {
            float min, max;
            float* sliderValue;
        } floaty;
    };
    void render(engone::UIModule* ui, engone::InputModule* input);
    float calculateSlideValue() {
        if(sliderType == SLIDER_FLOAT) {
            if(floaty.sliderValue)
                return (*floaty.sliderValue - floaty.min) / (floaty.max - floaty.min);
        } else if(sliderType == SLIDER_I32) {
            if(inty.sliderValue)
                return (float)(*inty.sliderValue - inty.min) / (float)(inty.max - inty.min);
        }
        return 0.5f;
    }
    void setSlidingPointer(float* sliderPtr) {
        sliderType = SLIDER_FLOAT;
        floaty.sliderValue = sliderPtr;   
    }
};