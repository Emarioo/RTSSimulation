#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#undef STB_IMAGE_IMPLEMENTATION

#include "rtssim/Game.h"

int main() {
    using namespace engone;
    engone::log::out.enableReport(false);
    
    
    // [|-2,-1|, |0,1| , |2,3| ,|4,5|, |6,7| ,8,9]
    
    // #define grid(x) ((x)<0 ? 0x8000'0000|((-(x+1) / 16) : (x) / 16 )
    
    // #define pri(x) printf("%d -> %d\n",x,(int)(grid(x)));
    
    // // printf("%f -> %d\n",-15.999f,(int)(grid(-15.999f)));
    // pri(0)
    // pri(-1)
    // pri(-15)
    // pri(-16)
    // pri(-17)
    
    StartGame();
    return 0;
}