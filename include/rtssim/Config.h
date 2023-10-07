#pragma once


// used in World.cpp
#define LOG_WORLD_CREATION






#ifdef LOG_WORLD_CREATION
#define _LOG_WORLD_CREATION(X) X;
#else
#define _LOG_WORLD_CREATION(X) ;
#endif