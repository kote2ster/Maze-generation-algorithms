/** @file main.c  */
/** [TOC]
 * @mainpage Maze generation algorithms tester
 * @section intro_sec Introduction
 *  @author    Akos Kote
 *  @version   1.0
 *  @date      Last Modification: 2015.05.11.
 *
 * [GitHub Project](https://github.com/kote2ster/Maze-generation-algorithms "GitHub Project")
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libtcod.h"

#define NumOfAlgorithms 5 /**< Number of algorithms defined */
#define CON_X_SIZE 161 /**< Default Screen width in chars */
#define CON_Y_SIZE 81  /**< Default Screen height in chars  */
#define DEF_MAZE_X CON_X_SIZE/2+1  /**< Default Maze width in chars, should be uneven */
#define DEF_MAZE_Y CON_Y_SIZE/2+1  /**< Default Maze width in chars, should be uneven */
#define MAX_DEEPNESS 1000 /**< Maximum memory allocation size for backtrack */
#define MAX_MAZESIZE_FOR_SHORTESTPATH 65*35+1 /**< Maximum allowed maze size for shortest path finding algorithm */

#define FPS 60 /**< Libtcod FPS */
#define VERSION "v1.0" /**< Version number */

typedef enum {LEFT=0,UP=1,RIGHT=2,DOWN=3} DIRECTION; /** Direction enum */
enum {NOTHING=0,WALL=1,STARTINGPOS=2,ENDINGPOS=3,MARKEDPATH=4,DEADEND=5}; /** 0: nothing, 1: wall, 2: starting position, 3: ending position, 4: marked for path */

char* Algorithm[NumOfAlgorithms]= {"Simple","Depth-first search","Random Kruskal","Random Prim","Chamber division"}; /**< List of algorithm names */
int selectionIndx;   /**< Selected algorithm */
int algrthmIter,algrthmTime; /**< Required iteration steps and time for algorithm calculation */
int mazeX,mazeY;     /**< Maze X and Y size with border */
int startingPos[2],endingPos[2]; /**< Starting and ending of Maze positions, between them will be backtrack/path calculations */
char** Maze;         /**< the 2D maze, 0: nothing, 1: wall */
TCOD_key_t key;      /**< Keyboard state handle */
TCOD_mouse_t mouse;  /**< Mouse state handle */
TCOD_event_t event;  /**< Event handle */
uint32 Time;         /**< Timing handle */

/** Resets everything */
inline void Reset() {
    int i;
    if(Maze!=NULL) {
        for(i=0; i<mazeX; i++) free(Maze[i]);
        free(Maze);
    }
    startingPos[0]=startingPos[1]=endingPos[0]=endingPos[1]=0;
    algrthmIter=algrthmTime=0;
    TCOD_console_clear(NULL);
}
/** Input handle function
  * @param [in] text text before the inputted numbers
  * @param [in] defaultValue Default value
  * @param [in] suff suffix of the number
  * @return Inputted number
  */
int InputHandle(const char *text,int defaultValue,const char *suff) {
    int Input=defaultValue;
    TCOD_console_clear(NULL);
    do {
        TCOD_console_rect(NULL,0,0,CON_X_SIZE,2,1,TCOD_BKGND_DEFAULT);
        if(Input>100) {
            Input=0;
            TCOD_console_set_default_background(NULL,TCOD_red);
            TCOD_console_print(NULL,0,1,"TOO BIG - try again - should be <=100%s ",suff);
            TCOD_console_set_default_background(NULL,TCOD_black);
        }
        TCOD_console_print(NULL,0,0,"%s",text);
        TCOD_console_print(NULL,strlen(text),0,"%d%s",Input,suff);
        TCOD_console_flush();
        TCOD_sys_wait_for_event(TCOD_EVENT_KEY_PRESS,&key,NULL,1);
        if(key.c>='0'&&key.c<='9') Input=(key.c-'0')+Input*10;
        else if(key.vk==TCODK_DELETE||key.vk==TCODK_BACKSPACE) Input=0;
    } while(key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
    TCOD_sys_wait_for_event(TCOD_EVENT_KEY_RELEASE,NULL,NULL,1);
    return Input;
}
/** Halt program, Waiting for keypress */
void CheckForKeypress() {
    do {
        event=TCOD_sys_check_for_event(TCOD_EVENT_ANY,&key,&mouse);
        TCOD_sys_sleep_milli(1000/FPS);
    } while(( key.vk==TCODK_ENTER||key.vk==TCODK_KPENTER ||event!=TCOD_EVENT_KEY_PRESS  )&&
            ((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&!TCOD_console_is_window_closed());
}
/** User sets up maze size */
void SetupMazeSize() {
    int i,j;
    do {
        TCOD_console_clear(NULL);
        TCOD_console_print(NULL,0,0,"Adjust the size of the maze (will be uneven), with up/down/left/right arrows");
        TCOD_console_print(NULL,0,1,"Maze X: %3d Maze Y: %3d",mazeX,mazeY);
        TCOD_console_set_default_background(NULL,TCOD_green);
        TCOD_console_print_frame(NULL,0,2,mazeX,mazeY,1,TCOD_BKGND_SET,"MAZE");
        TCOD_console_set_default_background(NULL,TCOD_black);
        TCOD_console_flush();
        CheckForKeypress(); //Waiting for keypress
        if     (key.vk==TCODK_UP   &&mazeY-2>=5)            mazeY-=2;
        else if(key.vk==TCODK_DOWN &&mazeY+2<=CON_Y_SIZE-2) mazeY+=2;
        else if(key.vk==TCODK_LEFT &&mazeX-2>=5)            mazeX-=2;
        else if(key.vk==TCODK_RIGHT&&mazeX+2<=CON_X_SIZE)   mazeX+=2;
    } while(((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
    Maze = (char**)calloc(mazeX,sizeof(char*));
    for(i=0; i<mazeX; i++) Maze[i] = (char*)calloc(mazeY,sizeof(char));
    for(i=0; i<mazeX; i++) {
        for(j=0; j<mazeY; j++) {
            Maze[i][j] = NOTHING;
            if(i==0||j==0||i==mazeX-1||j==mazeY-1) Maze[i][j] = WALL;
        }
    }
}
/** Algorithm chooser menu */
void ChooseAlgorithmMenu() {
    int i;
    TCOD_console_print(NULL,0,0,"Welcome to the maze generation algorithms testing program.");
    TCOD_console_print(NULL,0,1,"Choose an algorithm:");
    for(i=0; i<NumOfAlgorithms; i++) {
        TCOD_console_print(NULL,0,3+i,"%s",Algorithm[i]);
    }
    TCOD_console_flush();
    selectionIndx=0;
    do {
        TCOD_console_set_default_background(NULL,TCOD_white);
        TCOD_console_set_default_foreground(NULL,TCOD_black);
        TCOD_console_print(NULL,0,3+selectionIndx,"%s",Algorithm[selectionIndx]);
        TCOD_console_rect (NULL,0,3+selectionIndx,30,1,0,TCOD_BKGND_SET);
        TCOD_console_flush();
        CheckForKeypress(); //Waiting for keypress
        TCOD_console_set_default_background(NULL,TCOD_black);
        TCOD_console_set_default_foreground(NULL,TCOD_white);
        TCOD_console_print(NULL,0,3+selectionIndx,"%s",Algorithm[selectionIndx]);
        TCOD_console_rect (NULL,0,3+selectionIndx,30,1,0,TCOD_BKGND_SET);
        if(key.vk==TCODK_DOWN&&selectionIndx+1<NumOfAlgorithms) selectionIndx++;
        if(key.vk==TCODK_UP  &&selectionIndx-1>=0)              selectionIndx--;
    } while(((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
}
/** maze generation simple algorithm, implemented from a python code\n
 *  randomizes x,y even indexes, and from there randomizes neighbours, which will be walls\n
 *  the density setting determines how many times new starting even x,y indexes are randomized\n
 *  the complexity setting determines max how many times will be randomized neighbours
 *  this algorithm is really inefficient, because there will be a lot of nothing to do iterations\n
 *  and only overlaps will guarantee that the maze is completed
 */
void SimpleGeneration() {
    int i,j,x,y,possibleNeighbNum=0;
    DIRECTION possibleNeighb[4];
    int complexity; /* divider for the maximum possible neighbour iteration number */
    int density;    /* divider for the maximum possible iteration number */
    complexity=InputHandle("Enter the complexity of the maze generation (0-100%)(Default=75%): ",75,"%");
    density=InputHandle("Enter the density of the maze generation (0-100%)(Default=75%): ",75,"%");
    Time=TCOD_sys_elapsed_milli();
    for(i=0; i<(int)(density/100.0*mazeX/2.*mazeY/2.); i++) {
        x=(rand()%(mazeX/2+1)) *2; //Only even indexes
        y=(rand()%(mazeY/2+1)) *2; //Only even indexes
        Maze[x][y] = WALL;
        for(j=0; j<(int)(complexity/100.0*5*(mazeX+mazeY)); j++) {
            possibleNeighbNum=0;
            if(x>=2     ) {
                possibleNeighb[possibleNeighbNum]=LEFT;
                possibleNeighbNum++;
            }
            if(y>=2     ) {
                possibleNeighb[possibleNeighbNum]=UP;
                possibleNeighbNum++;
            }
            if(x<mazeX-2) {
                possibleNeighb[possibleNeighbNum]=RIGHT;
                possibleNeighbNum++;
            }
            if(y<mazeY-2) {
                possibleNeighb[possibleNeighbNum]=DOWN;
                possibleNeighbNum++;
            }
            if(possibleNeighbNum!=0) {
                switch(possibleNeighb[rand()%possibleNeighbNum]) {
                case LEFT:
                    if(Maze[x-2][y]==NOTHING) {
                        x=x-2;
                        Maze[x][y]=WALL;
                        Maze[x+1][y]=WALL;
                    }
                    break;
                case UP:
                    if(Maze[x][y-2]==NOTHING) {
                        y=y-2;
                        Maze[x][y]=WALL;
                        Maze[x][y+1]=WALL;
                    }
                    break;
                case RIGHT:
                    if(Maze[x+2][y]==NOTHING) {
                        x=x+2;
                        Maze[x][y]=WALL;
                        Maze[x-1][y]=WALL;
                    }
                    break;
                case DOWN:
                    if(Maze[x][y+2]==NOTHING) {
                        y=y+2;
                        Maze[x][y]=WALL;
                        Maze[x][y-1]=WALL;
                    }
                    break;
                }
            }
            algrthmIter++; //Counting algorithm's iteration
        }
    }
    algrthmTime=TCOD_sys_elapsed_milli()-Time; //Time
}
/** Maze generation with Depth-first search, the algorithm can be biased with altering the randomizer */
void DFSGeneration() {
    int i,j,x,y,possibleNeighbNum=0;
    char finished=0;
    int bias=InputHandle("How many times higher is the chance for moving left-right (max 9): ",0,"x")%10+1;
    DIRECTION possibleNeighb[2+bias*2];
    for(i=0; i<mazeX; i++) //Fill maze with wall
        for(j=0; j<mazeY; j++) Maze[i][j]=WALL;
    x=y=1; //Start generating from (1;1) position
    Time=TCOD_sys_elapsed_milli();
    do {
        possibleNeighbNum=0;
        if(x>=3    &&Maze[x-2][y]==WALL) {
            for(i=0; i<bias; i++) {
                possibleNeighb[possibleNeighbNum]=LEFT;
                possibleNeighbNum++;
            }
        }
        if(y>=3    &&Maze[x][y-2]==WALL) {
            possibleNeighb[possibleNeighbNum]=UP;
            possibleNeighbNum++;
        }
        if(x<mazeX-3&&Maze[x+2][y]==WALL) {
            for(i=0; i<bias; i++) {
                possibleNeighb[possibleNeighbNum]=RIGHT;
                possibleNeighbNum++;
            }
        }
        if(y<mazeY-3&&Maze[x][y+2]==WALL) {
            possibleNeighb[possibleNeighbNum]=DOWN;
            possibleNeighbNum++;
        }
        if(possibleNeighbNum!=0) {
            switch(possibleNeighb[rand()%possibleNeighbNum]) { //Randomize a new neighbour
            case LEFT:
                x=x-2;
                Maze[x][y]=NOTHING;
                Maze[x+1][y]=NOTHING;
                break;
            case UP:
                y=y-2;
                Maze[x][y]=NOTHING;
                Maze[x][y+1]=NOTHING;
                break;
            case RIGHT:
                x=x+2;
                Maze[x][y]=NOTHING;
                Maze[x-1][y]=NOTHING;
                break;
            case DOWN:
                y=y+2;
                Maze[x][y]=NOTHING;
                Maze[x][y-1]=NOTHING;
                break;
            }
        } else { //No possible neighbour from (x;y) position
            Maze[x][y]=MARKEDPATH; //Mark path for dead end
            if        (Maze[x-1][y]==NOTHING) { //Trace back the end of the dead end
                Maze[x-1][y]=MARKEDPATH;
                x=x-2;
            } else if (Maze[x][y-1]==NOTHING) {
                Maze[x][y-1]=MARKEDPATH;
                y=y-2;
            } else if (Maze[x+1][y]==NOTHING) {
                Maze[x+1][y]=MARKEDPATH;
                x=x+2;
            } else if (Maze[x][y+1]==NOTHING) {
                Maze[x][y+1]=MARKEDPATH;
                y=y+2;
            }
        }
        algrthmIter++; //Counting algorithm's iteration
        if(Maze[x][y]==MARKEDPATH) finished=1; //Everything is marked
    } while(!finished);
    algrthmTime=TCOD_sys_elapsed_milli()-Time; //Time
    for(i=0; i<mazeX; i++) //Remove marks
        for(j=0; j<mazeY; j++)
            if(Maze[i][j]==MARKEDPATH) Maze[i][j]=NOTHING;
}
/** Randomized Kruskal's algorithm: In a checkboard pattern there are holes, between them we will choose walls to remove, after this
  * the holes which are now connected are grouped, algorithm ends when there is one big group of holes left */
void RndKruskalGeneration() {
    int i,j,k=0,groupVal;
    int nodes[mazeX][mazeY]; //Starting holes, this array indicates which group they are in
    int walls[(mazeX-3)/2*(mazeY-1)/2+(mazeX-1)/2*(mazeY-3)/2][2]; //Wall coordinates
    int remainingWalls=(mazeX-3)/2*(mazeY-1)/2+(mazeX-1)/2*(mazeY-3)/2;
    for(i=1; i<mazeX-1; i++) {
        for(j=1; j<mazeY-1; j++) {
            if((i%2==0&&j%2==1)||(i%2==1&&j%2==0)) { //Initializing wall coordinates
                walls[k][0]=i;
                walls[k][1]=j;
                k++;
            }
        }
    }
    k=0;
    for(i=0; i<mazeX; i++) {
        for(j=0; j<mazeY; j++) {
            Maze[i][j]=WALL; //Initializing maze
            nodes[i][j]=k; //Initializing node groups
            k++;
        }
    }
    Time=TCOD_sys_elapsed_milli();
    do {
        k=rand()%remainingWalls;
        if       (walls[k][0]%2==0&&walls[k][1]%2==1) { //Connect holes left/right
            if(nodes[walls[k][0]-1][walls[k][1]]!=nodes[walls[k][0]+1][walls[k][1]]) { //If not in same group
                groupVal=nodes[walls[k][0]+1][walls[k][1]];
                for(i=0; i<mazeX; i++)
                    for(j=0; j<mazeY; j++)
                        if(nodes[i][j]==groupVal) nodes[i][j]=nodes[walls[k][0]-1][walls[k][1]]; //Merge groups
                Maze[walls[k][0]-1][walls[k][1]]=Maze[walls[k][0]][walls[k][1]]=Maze[walls[k][0]+1][walls[k][1]]=NOTHING;
                algrthmIter+=mazeX*mazeY; //Counting algorithm's iteration
            }
            remainingWalls--;
            walls[k][0]=walls[remainingWalls][0];
            walls[k][1]=walls[remainingWalls][1];
        } else if(walls[k][0]%2==1&&walls[k][1]%2==0) { //Connect holes up/down
            if(nodes[walls[k][0]][walls[k][1]-1]!=nodes[walls[k][0]][walls[k][1]+1]) { //If not in same group
                groupVal=nodes[walls[k][0]][walls[k][1]+1];
                for(i=0; i<mazeX; i++)
                    for(j=0; j<mazeY; j++)
                        if(nodes[i][j]==groupVal) nodes[i][j]=nodes[walls[k][0]][walls[k][1]-1]; //Merge groups
                Maze[walls[k][0]][walls[k][1]-1]=Maze[walls[k][0]][walls[k][1]]=Maze[walls[k][0]][walls[k][1]+1]=NOTHING;
                algrthmIter+=mazeX*mazeY; //Counting algorithm's iteration
            }
            remainingWalls--;
            walls[k][0]=walls[remainingWalls][0];
            walls[k][1]=walls[remainingWalls][1];
        }
    } while(remainingWalls>0);
    algrthmTime=TCOD_sys_elapsed_milli()-Time; //Time
}
/** Randomized Prim's algorithm, similar to Kruskal's greedy algorithm, but more efficient, stores list of adjacent cells */
void RndPrimGeneration() {
    int i,j,x,y,stackIndx=0;
    int **CELLStack=(int**)calloc(MAX_DEEPNESS,sizeof(int*));
    for(i=0; i<MAX_DEEPNESS; i++) CELLStack[i]=(int*)calloc(2,sizeof(int)); //Cell coordinates
    for(i=0; i<mazeX; i++) //Fill maze with wall
        for(j=0; j<mazeY; j++) Maze[i][j]=WALL;
    Time=TCOD_sys_elapsed_milli();
    x=(rand()%(mazeX/2)) *2+1; //Only uneven indexes, starting position
    y=(rand()%(mazeY/2)) *2+1; //Only uneven indexes, starting position
    do {
        if(stackIndx!=0) {
            j=rand()%stackIndx; //Randomize a new starting point
            x=CELLStack[j][0];
            y=CELLStack[j][1];
            stackIndx--;
            CELLStack[j][0]=CELLStack[stackIndx][0];
            CELLStack[j][1]=CELLStack[stackIndx][1];
        }
        if(x>=3    &&Maze[x-2][y]==WALL&&stackIndx<MAX_DEEPNESS) {
            CELLStack[stackIndx][0]=x-2;
            CELLStack[stackIndx][1]=y;
            Maze[x][y]=Maze[x-1][y]=Maze[x-2][y]=NOTHING;
            stackIndx++;
        }
        if(y>=3    &&Maze[x][y-2]==WALL&&stackIndx<MAX_DEEPNESS) {
            CELLStack[stackIndx][0]=x;
            CELLStack[stackIndx][1]=y-2;
            Maze[x][y]=Maze[x][y-1]=Maze[x][y-2]=NOTHING;
            stackIndx++;
        }
        if(x<mazeX-3&&Maze[x+2][y]==WALL&&stackIndx<MAX_DEEPNESS) {
            CELLStack[stackIndx][0]=x+2;
            CELLStack[stackIndx][1]=y;
            Maze[x][y]=Maze[x+1][y]=Maze[x+2][y]=NOTHING;
            stackIndx++;
        }
        if(y<mazeY-3&&Maze[x][y+2]==WALL&&stackIndx<MAX_DEEPNESS) {
            CELLStack[stackIndx][0]=x;
            CELLStack[stackIndx][1]=y+2;
            Maze[x][y]=Maze[x][y+1]=Maze[x][y+2]=NOTHING;
            stackIndx++;
        }
        algrthmIter++; //Algorithm iterator
    } while(stackIndx>0&&stackIndx<MAX_DEEPNESS);
    algrthmTime=TCOD_sys_elapsed_milli()-Time; //Time
    for(i=0; i<MAX_DEEPNESS; i++) free(CELLStack[i]); //Cell coordinates
    free(CELLStack);
}
/** this function is using the chamber division method, each chamber is crossed with two walls=>
  * this creates 4 chambers and with 3 random doors each will be visitable */
void ChmbrDivGeneration() {
    int i,j,x,y,absx,absy,width,height,stackIndx=0;
    int **ChmbStack=(int**)calloc(MAX_DEEPNESS,sizeof(int*));
    for(i=0; i<MAX_DEEPNESS; i++) ChmbStack[i]=(int*)calloc(4,sizeof(int)); //Chamber position and width/height
    DIRECTION possibleNeighb[4]= {LEFT,UP,RIGHT,DOWN};
    Time=TCOD_sys_elapsed_milli();
    absx=absy=0; //Absolute starting chamber position in maze
    width =mazeX-2;
    height=mazeY-2;
    do {
        if(stackIndx!=0) {
            j=rand()%stackIndx; //Randomize a new starting point
            absx=ChmbStack[j][0];
            absy=ChmbStack[j][1];
            width =ChmbStack[j][2];
            height=ChmbStack[j][3];
            stackIndx--;
            ChmbStack[j][0]=ChmbStack[stackIndx][0];
            ChmbStack[j][1]=ChmbStack[stackIndx][1];
            ChmbStack[j][2]=ChmbStack[stackIndx][2];
            ChmbStack[j][3]=ChmbStack[stackIndx][3];
        }
        x=(rand()%(width /2)) *2+2+absx; //Only even indexes, starting wall dividing position
        y=(rand()%(height/2)) *2+2+absy; //Only even indexes, starting wall dividing position
        for(i=absx; i<=absx+width ; i++) Maze[i][y]=WALL; //Horizontal split
        for(j=absy; j<=absy+height; j++) Maze[x][j]=WALL; //Vertical split
        algrthmIter+=mazeX+mazeY; //Algorithm iterator
        i=rand()%4;
        j=possibleNeighb[i]; //Randomize 3 splitting wall direction for doors (omit 1)
        possibleNeighb[i]=possibleNeighb[3];
        possibleNeighb[3]=j;
        for(i=0; i<3; i++) {
            switch(possibleNeighb[i]) {
            case LEFT:
                j=(rand()%((x-absx)/2))*2+1+absx;
                Maze[j][y]=NOTHING;
                break;
            case UP:
                j=(rand()%((y-absy)/2))*2+1+absy;
                Maze[x][j]=NOTHING;
                break;
            case RIGHT:
                j=(rand()%((absx+width+1-x)/2))*2+1+x;
                Maze[j][y]=NOTHING;
                break;
            case DOWN:
                j=(rand()%((absy+height+1-y)/2))*2+1+y;
                Maze[x][j]=NOTHING;
                break;
            }
        }
        if(x-absx>2&&y-absy>2) {
            ChmbStack[stackIndx][0]=absx;
            ChmbStack[stackIndx][1]=absy;
            ChmbStack[stackIndx][2]=x-absx-1;
            ChmbStack[stackIndx][3]=y-absy-1;
            stackIndx++;
        }
        if(x-absx>2&&absy+height-y>2) {
            ChmbStack[stackIndx][0]=absx;
            ChmbStack[stackIndx][1]=y;
            ChmbStack[stackIndx][2]=x-absx-1;
            ChmbStack[stackIndx][3]=absy+height-y;
            stackIndx++;
        }
        if(absx+width-x>2&&y-absy>2) {
            ChmbStack[stackIndx][0]=x;
            ChmbStack[stackIndx][1]=absy;
            ChmbStack[stackIndx][2]=absx+width-x;
            ChmbStack[stackIndx][3]=y-absy-1;
            stackIndx++;
        }
        if(absx+width-x>2&&absy+height-y>2) {
            ChmbStack[stackIndx][0]=x;
            ChmbStack[stackIndx][1]=y;
            ChmbStack[stackIndx][2]=absx+width-x;
            ChmbStack[stackIndx][3]=absy+height-y;
            stackIndx++;
        }
//        PrintMaze();
    } while(stackIndx>0&&stackIndx<MAX_DEEPNESS);
    algrthmTime=TCOD_sys_elapsed_milli()-Time; //Time
    for(i=0; i<MAX_DEEPNESS; i++) free(ChmbStack[i]); //Chamber position and width/height
    free(ChmbStack);
}
/** main printing function */
void PrintMaze() {
    int i,j;
    TCOD_console_clear(NULL);
    for(i=0; i<mazeX; i++) {
        for(j=0; j<mazeY; j++) {
            if     (Maze[i][j]==WALL)        TCOD_console_put_char(NULL,i,2+j,TCOD_CHAR_BLOCK3,TCOD_BKGND_DEFAULT);
            else if(Maze[i][j]==STARTINGPOS) TCOD_console_put_char(NULL,i,2+j,TCOD_CHAR_CROSS,TCOD_BKGND_DEFAULT);
            else if(Maze[i][j]==ENDINGPOS)   TCOD_console_put_char(NULL,i,2+j,TCOD_CHAR_CROSS,TCOD_BKGND_DEFAULT);
            else if(Maze[i][j]==MARKEDPATH)  TCOD_console_put_char(NULL,i,2+j,'$',TCOD_BKGND_DEFAULT);
            else if(Maze[i][j]==DEADEND)     TCOD_console_put_char(NULL,i,2+j,'#',TCOD_BKGND_DEFAULT);
            else                             TCOD_console_put_char(NULL,i,2+j,' ',TCOD_BKGND_DEFAULT);
        }
    }
    TCOD_console_flush();
}
/** Prints maze with algorithm informations */
void PrintMazeWithInfo() {
    PrintMaze();
    TCOD_console_print(NULL,0,0,"Used algorithm: %s\nMaze width: %3d Maze height: %3d",Algorithm[selectionIndx],mazeX,mazeY);
    TCOD_console_print_ex(NULL,CON_X_SIZE-1,0,TCOD_BKGND_DEFAULT,TCOD_RIGHT,"Algorithm iteration: %8d",algrthmIter);
    TCOD_console_print_ex(NULL,CON_X_SIZE-1,1,TCOD_BKGND_DEFAULT,TCOD_RIGHT,"Algorithm time (ms): %8d",algrthmTime);
    TCOD_console_flush();
}
/** In this function the user chooses exits */
void ChooseExits() {
    int prevMouseX=0,prevMouseY=0;
    PrintMaze();
    TCOD_console_print(NULL,0,0,"Choose exit(s) with your mouse (left click).\nPress ENTER when you're finished");
    TCOD_console_flush();
    do {
        event=TCOD_sys_check_for_event(TCOD_EVENT_ANY,&key,&mouse);
        TCOD_sys_sleep_milli(1000/FPS);
        if(event==TCOD_EVENT_MOUSE_MOVE) {
            //Possible exits only on uneven positions
            if(0<mouse.cx&&mouse.cx<mazeX-1&&(mouse.cy==2||mouse.cy==2+mazeY-1)&&mouse.cx%2==1) { //first, last line
                TCOD_console_set_char_foreground(NULL,prevMouseX,prevMouseY,TCOD_white);
                TCOD_console_set_char_foreground(NULL,mouse.cx,mouse.cy,TCOD_green);
                prevMouseX=mouse.cx;
                prevMouseY=mouse.cy;
                TCOD_console_flush();
            }
            if(2<mouse.cy&&mouse.cy<2+mazeY-1&&(mouse.cx==0||mouse.cx==mazeX-1)&&mouse.cy%2==1) { //first, last column
                TCOD_console_set_char_foreground(NULL,prevMouseX,prevMouseY,TCOD_white);
                TCOD_console_set_char_foreground(NULL,mouse.cx,mouse.cy,TCOD_green);
                prevMouseX=mouse.cx;
                prevMouseY=mouse.cy;
                TCOD_console_flush();
            }
        }
        if(mouse.lbutton&&prevMouseY>0) {
            Maze[prevMouseX][prevMouseY-2]=NOTHING; //Remove wall where exit
            PrintMaze();
            TCOD_console_print(NULL,0,0,"Choose exit(s) with your mouse (left click).\nPress ENTER when you're finished");
            TCOD_console_flush();
        }
    } while(((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
}
/** In this function user chooses a starting ang finishing position, between them will be backtrack/path calculations */
void ChooseStartFinish() {
    int prevMouseX=0,prevMouseY=0,firstPos=1;
    PrintMaze();
    TCOD_console_print(NULL,0,0,"Choose starting position with your mouse (left click).");
    TCOD_console_flush();
    do {
        event=TCOD_sys_check_for_event(TCOD_EVENT_ANY,&key,&mouse);
        TCOD_sys_sleep_milli(1000/FPS);
        if(event==TCOD_EVENT_MOUSE_MOVE) {
            //Starting/Ending positions are not on walls
            if(0<=mouse.cx&&mouse.cx<mazeX&&2<=mouse.cy&&mouse.cy<2+mazeY&&Maze[mouse.cx][mouse.cy-2]==NOTHING) {
                if(prevMouseY>0) TCOD_console_set_char_background(NULL,prevMouseX,prevMouseY,TCOD_black,TCOD_BKGND_SET);
                if(firstPos) TCOD_console_set_char_background(NULL,mouse.cx,mouse.cy,TCOD_green,TCOD_BKGND_SET);
                else TCOD_console_set_char_background(NULL,mouse.cx,mouse.cy,TCOD_green,TCOD_BKGND_SET);
                prevMouseX=mouse.cx;
                prevMouseY=mouse.cy;
                TCOD_console_flush();
            }
        }
        if(event==TCOD_EVENT_MOUSE_RELEASE&&mouse.lbutton_pressed&&prevMouseY>0) {
            if(firstPos) {
                firstPos=0;
                Maze[prevMouseX][prevMouseY-2]=STARTINGPOS; //Store starting position
                startingPos[0] = prevMouseX;
                startingPos[1] = prevMouseY-2;
                PrintMaze();
                TCOD_console_print(NULL,0,0,"Choose ending position with your mouse (left click).");
                TCOD_console_flush();
            } else {
                firstPos=-1; //End functon
                Maze[prevMouseX][prevMouseY-2]=ENDINGPOS; //Store ending position
                endingPos[0] = prevMouseX;
                endingPos[1] = prevMouseY-2;
            }
        }
    } while(firstPos!=-1&&((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
}
/** this function calculates the path between two selected positions with backtrack algorithm without recursion */
void RouteBetweenPositions() {
    int lengthOfpath=0,minlengthOfpath=0,i;
    int currentPos[2]= {startingPos[0],startingPos[1]};
    char *BTStack=(char*)calloc(MAX_DEEPNESS,sizeof(char)); //Represents tried moves, with enum DIRECTION: left,up,right,down
    char *shortBTStack=NULL; //Represents shortest moves to exit position
    DIRECTION minDir=LEFT; //Stores the previous DIRECTION, next direction try must be "bigger", DIRECTION enum is in order: LEFT=0,UP=1,RIGHT=2,DOWN=3
    //Selecting path generation mode
    char pathMode=0; //0: fastest, 1: shortest path generation
    if(mazeX*mazeY<MAX_MAZESIZE_FOR_SHORTESTPATH) { //Allow only shortest path search selection, when maze is not too big
        TCOD_console_clear(NULL);
        TCOD_console_print(NULL,0,0,"Choose the path search mode between two selected positions");
        TCOD_console_print(NULL,0,2,"Fastest search for path");
        TCOD_console_print(NULL,0,3,"Shortest path search (slow)");
        TCOD_console_flush();
        do {
            TCOD_console_set_default_background(NULL,TCOD_white);
            TCOD_console_set_default_foreground(NULL,TCOD_black);
            if     (pathMode==0) TCOD_console_print(NULL,0,2,"Fastest search for path");
            else if(pathMode==1) TCOD_console_print(NULL,0,3,"Shortest path search (slow)");
            TCOD_console_rect (NULL,0,2+pathMode,30,1,0,TCOD_BKGND_SET);
            TCOD_console_flush();
            CheckForKeypress(); //Waiting for keypress
            TCOD_console_set_default_background(NULL,TCOD_black);
            TCOD_console_set_default_foreground(NULL,TCOD_white);
            if     (pathMode==0) TCOD_console_print(NULL,0,2,"Fastest search for path");
            else if(pathMode==1) TCOD_console_print(NULL,0,3,"Shortest path search (slow)");
            TCOD_console_rect (NULL,0,2+pathMode,30,1,0,TCOD_BKGND_SET);
            if(key.vk==TCODK_DOWN&&pathMode+1<2)  pathMode++;
            if(key.vk==TCODK_UP  &&pathMode-1>=0) pathMode--;
        } while(((key.vk!=TCODK_ENTER&&key.vk!=TCODK_KPENTER)||event!=TCOD_EVENT_KEY_RELEASE)&&key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
        if(TCOD_console_is_window_closed()||key.vk==TCODK_ESCAPE) exit(0);
    }
    //Backtrack
    do {
        if     (currentPos[0]>0&&LEFT>=minDir&&(Maze[currentPos[0]-1][currentPos[1]]==NOTHING||Maze[currentPos[0]-1][currentPos[1]]==ENDINGPOS)) {
            currentPos[0]-=1;
            if(Maze[currentPos[0]][currentPos[1]]!=ENDINGPOS) Maze[currentPos[0]][currentPos[1]]=MARKEDPATH;
            BTStack[lengthOfpath]=LEFT;
            lengthOfpath++;
            minDir=LEFT;
        } else if(currentPos[1]>0&&UP>=minDir&&(Maze[currentPos[0]][currentPos[1]-1]==NOTHING||Maze[currentPos[0]][currentPos[1]-1]==ENDINGPOS)) {
            currentPos[1]-=1;
            if(Maze[currentPos[0]][currentPos[1]]!=ENDINGPOS) Maze[currentPos[0]][currentPos[1]]=MARKEDPATH;
            BTStack[lengthOfpath]=UP;
            lengthOfpath++;
            minDir=LEFT;
        } else if(currentPos[0]<mazeX-1&&RIGHT>=minDir&&(Maze[currentPos[0]+1][currentPos[1]]==NOTHING||Maze[currentPos[0]+1][currentPos[1]]==ENDINGPOS)) {
            currentPos[0]+=1;
            if(Maze[currentPos[0]][currentPos[1]]!=ENDINGPOS) Maze[currentPos[0]][currentPos[1]]=MARKEDPATH;
            BTStack[lengthOfpath]=RIGHT;
            lengthOfpath++;
            minDir=LEFT;
        } else if(currentPos[1]<mazeY-1&&DOWN>=minDir&&(Maze[currentPos[0]][currentPos[1]+1]==NOTHING||Maze[currentPos[0]][currentPos[1]+1]==ENDINGPOS)) {
            currentPos[1]+=1;
            if(Maze[currentPos[0]][currentPos[1]]!=ENDINGPOS) Maze[currentPos[0]][currentPos[1]]=MARKEDPATH;
            BTStack[lengthOfpath]=DOWN;
            lengthOfpath++;
            minDir=LEFT;
        } else if(lengthOfpath>0) { //Didn't found possible move, Move back
            i=0;
            if(currentPos[0]>0&&(Maze[currentPos[0]-1][currentPos[1]]==WALL||Maze[currentPos[0]-1][currentPos[1]]==DEADEND)) i++;
            else if(currentPos[0]==0) i++;
            if(currentPos[1]>0&&(Maze[currentPos[0]][currentPos[1]-1]==WALL||Maze[currentPos[0]][currentPos[1]-1]==DEADEND)) i++;
            else if(currentPos[1]==0) i++;
            if(currentPos[0]<mazeX-1&&(Maze[currentPos[0]+1][currentPos[1]]==WALL||Maze[currentPos[0]+1][currentPos[1]]==DEADEND)) i++;
            else if(currentPos[0]==mazeX-1) i++;
            if(currentPos[1]<mazeY-1&&(Maze[currentPos[0]][currentPos[1]+1]==WALL||Maze[currentPos[0]][currentPos[1]+1]==DEADEND)) i++;
            else if(currentPos[1]==mazeY-1) i++;
            if(i==3) Maze[currentPos[0]][currentPos[1]]=DEADEND; //It's a dead end
            /*---------------------------*/
            lengthOfpath--;
            minDir=BTStack[lengthOfpath];
            //Previous coordinates
            if(Maze[currentPos[0]][currentPos[1]]==MARKEDPATH) Maze[currentPos[0]][currentPos[1]]=NOTHING; //Remove marked path
            if     (minDir==LEFT ) currentPos[0]+=1;
            else if(minDir==UP   ) currentPos[1]+=1;
            else if(minDir==RIGHT) currentPos[0]-=1;
            else if(minDir==DOWN ) currentPos[1]-=1;
            minDir++; //Try again from the next direction
        } else if(lengthOfpath==0) break; //No possible solution!!
        if(Maze[currentPos[0]][currentPos[1]]==ENDINGPOS&&pathMode==0) break; //Check if finished,if pathMode=1 do not exit

        else if(pathMode==1&&Maze[currentPos[0]][currentPos[1]]==ENDINGPOS) {
            if(minlengthOfpath==0||minlengthOfpath>lengthOfpath) { //Initializing minlengthOfpath and If found shorter path
                if(shortBTStack!=NULL) free(shortBTStack);
                minlengthOfpath=lengthOfpath;
                shortBTStack=(char*)calloc(lengthOfpath,sizeof(char));
                for(i=0; i<lengthOfpath; i++) {
                    shortBTStack[i]=BTStack[i];
                }
            }
            lengthOfpath--; //Move back
            minDir=BTStack[lengthOfpath];
            //Previous coordinates
            if(Maze[currentPos[0]][currentPos[1]]==MARKEDPATH) Maze[currentPos[0]][currentPos[1]]=NOTHING; //Remove marked path
            if     (minDir==LEFT ) currentPos[0]+=1;
            else if(minDir==UP   ) currentPos[1]+=1;
            else if(minDir==RIGHT) currentPos[0]-=1;
            else if(minDir==DOWN ) currentPos[1]-=1;
            minDir++; //Try again from the next direction
        }
//        PrintMaze();
    } while(lengthOfpath<MAX_DEEPNESS);
    free(BTStack);

    if(pathMode==1&&minlengthOfpath!=0) { //Build up maze path
        currentPos[0]=startingPos[0];
        currentPos[1]=startingPos[1];
        for(i=0; i<minlengthOfpath-1; i++) {
            if     (shortBTStack[i]==LEFT ) currentPos[0]-=1;
            else if(shortBTStack[i]==UP   ) currentPos[1]-=1;
            else if(shortBTStack[i]==RIGHT) currentPos[0]+=1;
            else if(shortBTStack[i]==DOWN ) currentPos[1]+=1;
            Maze[currentPos[0]][currentPos[1]]=MARKEDPATH;
        }
        if(shortBTStack!=NULL) free(shortBTStack);
    }
}
/** main function loop */
int main(int argc,char* argv[]) {
    Maze=NULL;
    mazeX=DEF_MAZE_X;
    mazeY=DEF_MAZE_Y;
    srand(time(NULL));
    TCOD_console_init_root(CON_X_SIZE,CON_Y_SIZE,"Maze generation algorithms tester",0,TCOD_RENDERER_SDL);
    TCOD_sys_set_fps(FPS);
    do {
        Reset();
        ChooseAlgorithmMenu();
        SetupMazeSize();
        if     (strcmp(Algorithm[selectionIndx],"Simple")==0            ) SimpleGeneration();
        else if(strcmp(Algorithm[selectionIndx],"Depth-first search")==0) DFSGeneration();
        else if(strcmp(Algorithm[selectionIndx],"Random Kruskal")==0    ) RndKruskalGeneration();
        else if(strcmp(Algorithm[selectionIndx],"Random Prim")==0       ) RndPrimGeneration();
        else if(strcmp(Algorithm[selectionIndx],"Chamber division")==0  ) ChmbrDivGeneration();
        ChooseExits();
        ChooseStartFinish();
        RouteBetweenPositions();
        PrintMazeWithInfo();
        CheckForKeypress();
    } while(key.vk!=TCODK_ESCAPE&&!TCOD_console_is_window_closed());
    return 0;
}
