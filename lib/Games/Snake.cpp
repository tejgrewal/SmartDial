#include "Snake.h"
#include "AppConfig.h"
#include "Haptics.h"
#include "Theme.h"
#include "Storage.h"


struct Cell { int x; int y; };
static const int GRID=20, CELL=10; static const int ORGX = CX - (GRID*CELL)/2; static const int ORGY = CY - (GRID*CELL)/2 + 10;
static Cell snake[GRID*GRID]; static int snakeLen=0; enum Dir: uint8_t { UP, RIGHTD, DOWN, LEFTD }; static Dir dir=RIGHTD; static int foodX=10, foodY=10; static uint32_t stepMs=180,lastStep=0; static bool gameOver=false; static int score=0;
static ModulinoKnob* k=nullptr; static int lastKnob=0, accum=0; static const int TURN_THRESH=2; static bool turnUsed=false;


static uint32_t lcg=1234567; static uint32_t frand(){ lcg=1664525UL*lcg+1013904223UL; return lcg; }
static void placeFood(){ while(true){ foodX=frand()%GRID; foodY=frand()%GRID; bool ok=true; for(int i=0;i<snakeLen;i++){ if(snake[i].x==foodX && snake[i].y==foodY){ ok=false; break; } } if(ok) break; } }
void Snake::init(TFT_eSprite &, ModulinoKnob &knob){ k=&knob; lastKnob=knob.get(); }
void Snake::reset(){ snakeLen=3; snake[0]={GRID/2-1,GRID/2}; snake[1]={GRID/2,GRID/2}; snake[2]={GRID/2+1,GRID/2}; dir=RIGHTD; score=0; stepMs=180; lastStep=millis(); gameOver=false; placeFood(); accum=0; turnUsed=false; }
static void turnLeft(){ Dir nd=(dir==UP)?LEFTD:(dir==LEFTD)?DOWN:(dir==DOWN)?RIGHTD:UP; if(!((dir==UP&&nd==DOWN)||(dir==DOWN&&nd==UP)||(dir==LEFTD&&nd==RIGHTD)||(dir==RIGHTD&&nd==LEFTD))) dir=nd; }
static void turnRight(){ Dir nd=(dir==UP)?RIGHTD:(dir==RIGHTD)?DOWN:(dir==DOWN)?LEFTD:UP; if(!((dir==UP&&nd==DOWN)||(dir==DOWN&&nd==UP)||(dir==LEFTD&&nd==RIGHTD)||(dir==RIGHTD&&nd==LEFTD))) dir=nd; }


void Snake::tick(){ if(gameOver) return; uint32_t now=millis(); if(now-lastStep<stepMs) return; lastStep=now; turnUsed=false;
int pos=k->get(), d=pos-lastKnob; lastKnob=pos; if(d!=0 && !turnUsed){ accum += (d>0)?1:-1; if(accum>=TURN_THRESH){ turnRight(); turnUsed=true; accum=0; Haptics::tap(); } else if(accum<=-TURN_THRESH){ turnLeft(); turnUsed=true; accum=0; Haptics::tap(); } }
Cell head=snake[snakeLen-1]; if(dir==UP) head.y--; else if(dir==DOWN) head.y++; else if(dir==LEFTD) head.x--; else head.x++; if(head.x<0) head.x=GRID-1; else if(head.x>=GRID) head.x=0; if(head.y<0) head.y=GRID-1; else if(head.y>=GRID) head.y=0;
for(int i=0;i<snakeLen;i++){ if(snake[i].x==head.x && snake[i].y==head.y){ gameOver=true; Storage::saveSnake(score); return; } }
if(head.x==foodX && head.y==foodY){ snake[snakeLen++]=head; score+=10; if(stepMs>80) stepMs-=5; placeFood(); } else { for(int i=0;i<snakeLen-1;i++) snake[i]=snake[i+1]; snake[snakeLen-1]=head; }
}


void Snake::draw(TFT_eSprite &spr){
spr.fillSprite(Theme::BG);
int fx=ORGX+foodX*CELL + CELL/2; int fy=ORGY+foodY*CELL + CELL/2; spr.fillCircle(fx,fy,CELL/2-1,TFT_RED);
for(int i=0;i<snakeLen;i++){ int x=ORGX+snake[i].x*CELL; int y=ORGY+snake[i].y*CELL; spr.fillRoundRect(x+1,y+1,CELL-2,CELL-2,3,TFT_GREEN); }
spr.setTextDatum(BC_DATUM); spr.setTextColor(TFT_WHITE,Theme::BG); char b[24]; snprintf(b,sizeof(b),"SCORE %d",score); spr.drawString(b, CX, SCREEN_HEIGHT-6);
if(gameOver){ spr.setTextDatum(MC_DATUM); spr.setTextColor(TFT_YELLOW,Theme::BG); spr.drawString("GAME OVER",CX,CY-10); spr.setTextColor(TFT_WHITE,Theme::BG); char s[24]; snprintf(s,sizeof(s),"SCORE %d",score); spr.drawString(s, CX, CY+14); spr.setTextColor(TFT_CYAN,Theme::BG); spr.drawString("Press to return",CX,CY+36); }
spr.pushSprite(0,0);
}


void Snake::onPress(void (*goMenu)()){ static bool showMenu=false; static int sel=0; if(gameOver){ showMenu=true; }
if(showMenu){ if(sel==0){ reset(); showMenu=false; } else { showMenu=false; Haptics::back(); goMenu(); } }
else { sel=0; showMenu=true; Haptics::tap(); }
}