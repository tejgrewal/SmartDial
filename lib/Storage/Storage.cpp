#include "Storage.h"
static Preferences prefs; static const char* NS="qubeGames"; static const char* K_PONG="pongHigh"; static const char* K_SNAKE="snakeHigh";
int Storage::bestPong=0; int Storage::bestSnake=0;
void Storage::loadHighscores(){ prefs.begin(NS,false); bestPong=prefs.getInt(K_PONG,0); bestSnake=prefs.getInt(K_SNAKE,0); prefs.end(); }
void Storage::resetHighscores(){ prefs.begin(NS,false); prefs.putInt(K_PONG,0); prefs.putInt(K_SNAKE,0); prefs.end(); bestPong=bestSnake=0; }
void Storage::savePong(int s){ if(s>bestPong){ prefs.begin(NS,false); prefs.putInt(K_PONG,s); prefs.end(); bestPong=s; } }
void Storage::saveSnake(int s){ if(s>bestSnake){ prefs.begin(NS,false); prefs.putInt(K_SNAKE,s); prefs.end(); bestSnake=s; } }