// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allocate memory for a drawing state and initialise it
state *newState() {
  state* new = NULL;
  new = (state*)malloc(sizeof(state));

  new -> x = 0;
  new -> y = 0;
  new -> tx = 0;
  new -> ty = 0;
  new -> tool = LINE;
  new -> start = 0;
  new -> data = 0;
  new -> end = false;

  return new;
}

// Reset values of state to default
void refreshState(state *s) {
  s -> x = 0;
  s -> y = 0;
  s -> tx = 0;
  s -> ty = 0;
  s -> tool = LINE;
  s -> data = 0;
  s -> end = false;
}

// Release all memory associated with the drawing state
void freeState(state *s) {
  free(s);
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
  if(b>=128+64){
    return DATA;
  }
  if (b>=128){
    return TOOL;
  }
  else if(b>=64){
    return DY;
  }
  return DX;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  int operand = 0;

  //remove opcode
  if(b>=128){
    b-=128;
  }
  if(b>=64){
    b-=64;
  }

  //remove two complement
  if(b>=32){
    operand = -32;
    b-=32;
  }

  return operand + b;
}

void obeyTool(display *d, state *s, int operand){
  switch(operand){
    case NONE: s -> tool = NONE; break;
    case LINE: s -> tool = LINE; break;
    case BLOCK: s -> tool = BLOCK; break;
    case COLOUR: colour(d,s -> data);break;
    case TARGETX: s -> tx = s -> data; break;
    case TARGETY: s -> ty = s -> data; break;
    case SHOW: show(d); break;
    case PAUSE: pause(d, s-> data); break;
    case NEXTFRAME: s->end = true;
  }
  s->data = 0;
}

void obeyDX(state *s, int operand){
  s -> tx = s-> tx + operand;
}

void obeyDY(display *d, state *s, int operand){
  s -> ty = s -> ty + operand;

  if(s-> tool == LINE){
    line(d, s->x, s->y, s->tx, s->ty);
  }
  else if(s-> tool == BLOCK){
    block(d,s->x,s->y,(s->tx-s->x),(s->ty-s->y));
  }

  s -> y = s -> ty;
  s -> x = s -> tx;
}

void obeyData(state *s, int operand){
  s -> data = s-> data << 6;
  s -> data += operand;
}

int getOperandUnsigned(byte b){
  if(b>=128){
    b-=128;
  }
  if(b>=64){
    b-=64;
  }
  return b;
}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op){
  int opcode = getOpcode(op);
  int operand = getOperand(op);
  int unsignedOperand = getOperandUnsigned(op);

  switch(opcode){
    case DATA: obeyData(s,unsignedOperand); break;
    case TOOL: obeyTool(d, s,operand); break;
    case DX: obeyDX(s, operand); break;
    case DY: obeyDY(d, s, operand);
  }
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.
bool processSketch(display *d, const char pressedKey, void *data) {

  //TO DO: OPEN, PROCESS/DRAW A SKETCH FILE BYTE BY BYTE, THEN CLOSE IT
  //NOTE: CHECK DATA HAS BEEN INITIALISED... if (data == NULL) return (pressedKey == 27);
  //NOTE: TO GET ACCESS TO THE DRAWING STATE USE... state *s = (state*) data;
  //NOTE: TO GET THE FILENAME... char *filename = getName(d);
  //NOTE: DO NOT FORGET TO CALL show(d); AND TO RESET THE DRAWING STATE APART FROM
  //      THE 'START' FIELD AFTER CLOSING THE FILE
  
  if(data == NULL) return (pressedKey == 27);

  state *s = (state*)data;

  //using lecture suggested IO reading for a binary file
  char *filename = getName(d);
  FILE *in = fopen(filename, "rb");
  unsigned char op = fseek(in, s->start, SEEK_SET);

  while(!feof(in)){
    if(s->end) break;
    obey(d,s,op);
    if(s->end) s-> start = ftell(in);
    op = fgetc(in);
  }
  if(feof(in)) s->start = 0;
  fclose(in); 

  refreshState(s);
  show(d);
  return (pressedKey == 27);
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif
