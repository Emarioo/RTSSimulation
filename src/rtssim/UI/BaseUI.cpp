#include "rtssim/UI/BaseUI.h"

#include "rtssim/Game.h"

void RenderBaseUI(GameState* gameState) {
    using namespace engone;
    engone::UIModule* ui = &gameState->uiModule;
    
    int padding = 5;
    
    int layout_y = padding;
    int layout_x = padding;
    
    // #### RENDER RESOURCES
    for(int i=0;i<gameState->teamResources.resources.size();i++){
        auto& item = gameState->teamResources.resources[i];
        
        int temp_str_max = 256;
        char* temp_str = (char*)gameState->stringAllocator_render.allocate(temp_str_max);
        
        auto text = ui->makeText();
        int temp_str_len = snprintf(temp_str,temp_str_max,"%s: %d", ToString(item.type), item.amount);
        text->string = temp_str;
        text->length = temp_str_len;
        text->color = {1,1,1,1};
        text->h = 18;
        text->y = layout_y;
        layout_y += text->h;
        text->x = gameState->winWidth - ui->getWidthOfText(text) - padding;
    }
    // auto text = ui->makeText();
    
    // text = ui->makeText();
    // static char stone_str[256];
    // int stone_str_len = snprintf(stone_str,sizeof(stone_str),"Stone: %d", gameState->teamResources.stone);
    // text->string = stone_str;
    // text->length = stone_str_len;
    // text->color = {1,1,1,1};
    // text->h = 18;
    // text->y = layout_y;
    // layout_y += text->h;
    // text->x = gameState->winWidth - ui->getWidthOfText(text) - padding;
    
    // text = ui->makeText();
    // static char iron_str[256];
    // int iron_str_len = snprintf(iron_str,sizeof(iron_str),"Iron: %d", gameState->teamResources.iron);
    // text->string = iron_str;
    // text->length = iron_str_len;
    // text->color = {1,1,1,1};
    // text->h = 18;
    // text->y = layout_y;
    // layout_y += text->h;
    // text->x = gameState->winWidth - ui->getWidthOfText(text) - padding;
    
    // TODO: Use keybindings instead of hardcoded keys
    layout_x = gameState->winWidth - padding;
    layout_y = gameState->winHeight - padding;
    auto text = ui->makeText();
    static const char* training_hall_str = "Press 'T' for Training Hall";
    text->string = (char*)training_hall_str;
    text->length = strlen(training_hall_str);
    text->color = {1,1,1,1};
    text->h = 15;
    layout_y -= text->h;
    text->y = layout_y;
    text->x = layout_x - ui->getWidthOfText(text);
    
    text = ui->makeText();
    static const char* worker_str = "Press 'Y' for Worker";
    text->string = (char*)worker_str;
    text->length = strlen(worker_str);
    text->color = {1,1,1,1};
    text->h = 15;
    layout_y -= text->h;
    text->y = layout_y;
    text->x = layout_x - ui->getWidthOfText(text);
    
    text = ui->makeText();
    int temp_str_max = 256;
    char* temp_str = (char*)gameState->stringAllocator_render.allocate(temp_str_max);
        
    snprintf(temp_str, temp_str_max, "Press 'L' for non-fixed FPS");
    text->string = (char*)temp_str;
    text->length = strlen(temp_str);
    text->color = {1,1,1,1};
    text->h = 15;
    layout_y -= text->h;
    text->y = layout_y;
    text->x = layout_x - ui->getWidthOfText(text);
    
    // #### RENDER MESSAGES
    int msg_h = 15;
    int height_of_all_messages = gameState->messages.size() * msg_h;
    layout_x = padding;
    layout_y = gameState->winHeight - padding;
    for(int i=0;i<gameState->messages.size();i++){
        auto& msg = gameState->messages[i];
        
        msg.lifetime -= gameState->current_frameTime;
        if(msg.lifetime < 0) {
            gameState->messages.removeAt(i);
            i--;
            continue;
        }
        
        auto ui_text = ui->makeText();
        ui_text->string = (char*)msg.log.c_str();
        ui_text->length = msg.log.length();
        ui_text->color = {msg.color.r,msg.color.g,msg.color.b,1};
        if(msg.lifetime < 0.5)
            ui_text->color.a = msg.lifetime*2.f;
        ui_text->h = msg_h;
        ui_text->y = layout_y - msg_h;
        layout_y -= text->h;
        ui_text->x = layout_x;
    }
}