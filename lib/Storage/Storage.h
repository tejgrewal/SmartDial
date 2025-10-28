#pragma once
#include <Preferences.h>
namespace Storage{
extern int bestPong, bestSnake;
void loadHighscores();
void resetHighscores();
void savePong(int s);
void saveSnake(int s);
}