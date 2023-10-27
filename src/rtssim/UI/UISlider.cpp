#include "rtssim/UI/UISlider.h"

void UISlider::render(engone::UIModule* ui, engone::InputModule* input) {
    float value = calculateSlideValue();
    if(input->isKeyPressed(GLFW_MOUSE_BUTTON_LEFT)) {
        float mx = input->getMouseX();
        float my = input->getMouseY();
        if(mx >= x && mx < x + w && my >= y && my < y + h) {
            selected = true;
            clickedMX = input->getMouseX();
            clickedMY = input->getMouseY();
            clickedValue = value;
        }
    }
    if(input->isKeyReleased(GLFW_MOUSE_BUTTON_LEFT)) {
        selected = false;
    }
    const float padding = 2;
    float sliderWidth = w - 2*padding;
    if(selected) {
        if(sliderType == SLIDER_FLOAT) {
            if(floaty.sliderValue) {
                float newValue = (clickedValue * sliderWidth + (input->getMouseX() - clickedMX)) / sliderWidth;
                if(newValue < 0.f) newValue = 0.f;
                if(newValue > 1.f) newValue = 1.f;
                *floaty.sliderValue = newValue*(floaty.max - floaty.min) + floaty.min;
            }
        } else if(sliderType == SLIDER_I32) {
            if(inty.sliderValue) {
                float newValue = (clickedValue * sliderWidth + (input->getMouseX() - clickedMX)) / sliderWidth;
                if(newValue < 0.f) newValue = 0.f;
                if(newValue > 1.f) newValue = 1.f;
                *inty.sliderValue = newValue*(inty.max - inty.min) + inty.min;
            }
        }
    }
    
    value = calculateSlideValue();
    float sliderHeight=0, headerHeight=0;
    if(sliderName) {
        sliderHeight = h/2 - padding;
        headerHeight = h/2 - padding;
    } else {
        sliderHeight = h - 2*padding;
    }
    
    auto box = ui->makeBox();
    box->x = x;
    box->y = y;
    box->w = w;
    box->h = h;
    box->color = {0,0,0,0.2};
    
    box = ui->makeBox();
    box->x = x + padding;
    box->y = y + padding + headerHeight;
    box->w = sliderWidth * value;
    box->h = sliderHeight;
    box->color = {0.7f,0.7f,.7f,0.95};
    
    auto text = ui->makeText();
    text->color = {0,0,0,1};
    if(sliderType == SLIDER_FLOAT) {
        text->length = snprintf(textStorage, sizeof(textStorage), "%.2f",*floaty.sliderValue);
    } else if(sliderType == SLIDER_FLOAT) {
        text->length = snprintf(textStorage, sizeof(textStorage), "%d",*inty.sliderValue);
    }
    text->string = textStorage;
    text->h = sliderHeight;
    text->x = x + w/2 - ui->getWidthOfText(text)/2;
    text->y = y + headerHeight + padding;
    
    if(sliderName) {
        auto text = ui->makeText();
        text->color = {0,0,0,1};
        text->string = (char*)sliderName;
        text->length = strlen(sliderName);
        text->h = headerHeight;
        text->x = x + w/2 - ui->getWidthOfText(text)/2;
        text->y = y;
    }
}