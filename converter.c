#include "displayfull.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

//---------- 3d array --------------

struct intArray3d{
    size_t sizeX;
    size_t sizeY;
    size_t sizeZ;
    int *data;
};
typedef struct intArray3d lineData;

lineData *createLineData(int x, int y, int z){
    lineData *new = (lineData*)malloc(sizeof(lineData));
    int *xs = malloc(x*y*z*sizeof(int));
    new -> sizeX = x;
    new -> sizeY = y;
    new -> sizeZ = z;
    new -> data = xs;
    return new;
}

void freeLineData(lineData *ld){
    free(ld->data);
    free(ld);
}

void setItem(lineData *ld, int x, int y, int z, int item){
    int X = ld -> sizeX;
    int Y = ld -> sizeY;
    ld -> data[x*X+y*Y+z] = item;
}

int getItem(lineData *ld, int x, int y, int z){
    int X = ld -> sizeX;
    int Y = ld -> sizeY;
    return ld -> data[x*X+y*Y+z];
}

void incItem(lineData *ld, int x, int y, int z){
    int X = ld -> sizeX;
    int Y = ld -> sizeY;
    ld -> data[x*X+y*Y+z] = ld -> data[x*X+y*Y+z] +1;
}


//---------- Error Handling -------------

void displayFileError(){
    printf("Enter in the format ./converter [filename]\n");
}

//------------ Write to SK File ---------------

// Change the current sketch colour to a grey value from 0-255
// Based on the ratio of the pixel int val and the maxval
void writeColour(FILE *out,int maxval,int colour){
    // + 0.5 for floor to round correctly
    double maxvalD = maxval, colourD = colour;
    double valueNeeded = (colourD/maxvalD)*255 + 0.5;
    unsigned int hexNeeded = floor(valueNeeded);
    unsigned int targetColour = 255 + (hexNeeded <<8) + (hexNeeded <<16) + (hexNeeded <<24);
    unsigned int instructionToPut =0;

    if(targetColour != 0){

        //put 11 and then the first two bits of rgba
        instructionToPut = 0xc0 + ((0xC0000000 & targetColour) >> 30);
        fputc(instructionToPut,out);

        //put 11 and then next 6 bits of target
        instructionToPut = 0xc0 + ((0x3F000000 & targetColour)>>24);
        fputc(instructionToPut,out);

        //put 11 and then next 6 bits of target
        instructionToPut = 0xc0 + ((0xfc0000 & targetColour)>>18);
        fputc(instructionToPut,out);

        //put 11 and then next 6 bits of target
        instructionToPut = 0xc0 + ((0x3f000 & targetColour)>>12);
        fputc(instructionToPut,out);

        //put 11 and then next 6 bits of target
        instructionToPut = 0xc0 + ((0xfc0 & targetColour)>>6);
        fputc(instructionToPut,out);

        // put 11 and then the last 6 bits of target
        int instructionToPut = 0xc0 + (0x3f & targetColour);
        fputc(instructionToPut,out);
    }
    //put tool - colour
    fputc(0x83, out);
}

// Create a block from (0,0) to (x1,y1) in current colour
void writeBlockBG(FILE *out, int x1, int y1){
    fputc(0x80,out);
    if(x1>31){
        //put data
        fputc(0xc0 + ((0xfc0 & x1)>>6),out);
        fputc(0xc0 + 0x3f & x1, out);
        // move target x
        fputc(0x84,out);
    }
    else{
        // move x if small enough
        fputc(x1,out);
    }
    if(y1>31){
        // put data
        fputc(0xc0 + ((0xfc0 & y1)>>6),out);
        fputc(0xc0 + 0x3f & y1, out);
        //move target y
        fputc(0x85,out);
        //set tool and make a block
        fputc(0x82,out);
        fputc(0x40,out);
    }
    else{
        // set too and make a block if small enough
        fputc(0x82,out);
        fputc(0x40 + y1,out);
    }
}

// Set the background given a colour, H and Width
void writeBackground(FILE *out, int maxval, int H, int W, int maxColour){
    writeColour(out,maxval, maxColour);
    writeBlockBG(out,H,W);
}

// Move the position in the sketch file to (x1,y1)
void writeMoveXY(FILE *out, int x1, int y1){
    // Tool: none
    fputc(0x80,out);

    // X
    // data
    if(x1 >= pow(2,5))fputc(0xc0 + ((0xfc0 & x1)>>6),out);
    fputc(0xc0 + (0x3f & x1), out);
    // move target
    fputc(0x84,out);

    //Y
    //data
    if(y1 >= pow(2,5))fputc(0xc0 + ((0xfc0 & y1)>>6),out);
    fputc(0xc0 + (0x3f & y1), out);
    // move target
    fputc(0x85,out);

    //Move x y
    fputc(0x40,out);
}

// Move the s -> tx to target int
void writeMoveTargetX(FILE *out, int target){

    // X
    // data
    if(target >= pow(2,5))fputc(0xc0 + ((0xfc0 & target)>>6),out);
    fputc(0xc0 + (0x3f & target), out);
    // move target
    fputc(0x84,out);
}

// Write a line based on line data
void writeLine(FILE *out, int x0,int y0,int length){
    writeMoveXY(out,x0,y0);
    writeMoveTargetX(out,x0+length-1);
    // set tool to line
    fputc(0x81,out);
    //draw line
    fputc(0x40,out);

}

//---------------- Conversion --------------

// Is the end of the file name .pgm
bool isPGM(int size, char name[]){
    if(name[size-1]!= 'm')return false;
    if(name[size-2]!= 'g') return false;
    if(name[size-3]!= 'p') return false;
    if(name[size-4]!= '.') return false;
    return true;
}

// Is the end of the file name .sk
bool isSK(int size, char name[]){
    if(name[size-1]!= 'k')return false;
    if(name[size-2]!= 's') return false;
    if(name[size-3]!= '.') return false;
    return true;
}

// Is the file name of acceptable format
bool acceptableFileName(char name[]){
    int size =0;
    while(name[size]!=0){
        size++;
    }
    return (isPGM(size,name) || isSK(size,name));
}

// Is the file a meant to be converted to .sk file
// I.e is it a PGM
bool toSketch(char name[]){
    int size =0;
    while(name[size]!=0){
        size++;
    }
    return isPGM(size,name);
}

// Change the ending of a file name from .sk to .pgm
void toSKFile(int size, char nameIn[], char nameOut[]){
    nameOut[size-3] = 's';
    nameOut[size-2] = 'k';
    nameOut[size-1] = 0;
    for(int i=0; i<size-3;i++){
        nameOut[i] = nameIn[i];
    }
}

// Change the ending of a file name from .pgm to .sk
void toPGMFile(int size, char nameIn[], char nameOut[]){
    nameOut[size-4] = 'p';
    nameOut[size-3] = 'g';
    nameOut[size-2] = 'm';
    nameOut[size-1] = 0;
    for(int i=0; i<size-4;i++){
        nameOut[i] = nameIn[i];
    }
}

// Determine file type and change the name for the ouput file appropriately
void changeFileType(char nameIn[], char nameOut[]){
    int size =0;
    while(nameIn[size]!=0){
        size++;
    }
    
    if(isSK(size,nameIn)) toPGMFile(size,nameIn, nameOut);
    else toSKFile(size,nameIn, nameOut);
}

// Extract the height of the image from the header data
int getWidthHeader(char header[]){
    int nOfWhiteSpace = 0, i=0, index =0,length,height = 0;
    char number[10];

    // Ignore the magic number
    while(nOfWhiteSpace <1){
        if(header[i] == ' ') nOfWhiteSpace++;
        i++;
    }
    index = i;

    // Get the width stored between the first and second whitespaces
    while(nOfWhiteSpace <2){
        number[i-index] = header[i];
        if(header[i] == ' ') nOfWhiteSpace++;
        i++;
    }

    length = i - index - 1; // length of the number

    //convert the number string into an int
    for(int j=0; j< length;j++){
        height += (number[j] -48) * pow(10,length-j-1);
    }

    return height;

}

// Extract the width of the image from the header data
int getHeightHeader(char header[]){

    int nOfWhiteSpace = 0, i=0, index =0,length,width = 0;
    char number[10];

    // ignore the magic number and width
    while(nOfWhiteSpace <2){
        if(header[i] == ' ') nOfWhiteSpace++;
        i++;
    }
    index = i;

    // get the width data stored between the 2nd and 3rd white spaces
    while(nOfWhiteSpace <3){
        number[i-index] = header[i];
        if(header[i] == ' ') nOfWhiteSpace++;
        i++;
    }

    length = i - index - 1; // length of the number

    // convert number from string to int
    for(int j=0; j< length;j++){
        width += (number[j] -48) * pow(10,length-j-1);
    }

    return width;

}

// extract the max grey value from the header data
int getMaxvalHeader(char header[]){

    int nOfWhiteSpace = 0, i=0, index =0,length,greyVal = 0;
    char number[10];

    // ignore the magic number, height and wdith
    while(nOfWhiteSpace <3){
        if(header[i] == ' ') nOfWhiteSpace++;
        i++;
    }
    index = i;

    // get the width data stored between the 3rd and 4th white spaces
    while(nOfWhiteSpace <4){
        number[i-index] = header[i];
        // 10 is EOL for ASCII
        if(header[i] == ' ' || header[i] == 10) nOfWhiteSpace++;
        i++;
    }

    length = i - index - 1; // length of the number

    // convert number from string to int
    for(int j=0; j< length;j++){
        greyVal += (number[j] -48) * pow(10,length-j-1);
    }

    return greyVal;

}

// reinterpret cast all items in an array from signed to unsigned
void unsignedSignedCopy(int length, unsigned char destination[length], char source[length]){
    for(int i=0;i<length;i++){
        destination[i] = *(unsigned char*)(&source[i]);
    }
}

// get the image data from a given file
void getImage(FILE *file, int H, int W, unsigned char image[H][W], bool large){
    char current = 0;
    for(int i=0;i<H;i++){
        for(int j=0;j<W;j++){
            current = fgetc(file);
            image[i][j] = *(unsigned char*)(&current);
        }
    }
}

// return the index with the highest number (which colour appears the most)
int indexOfMaxVal(int maxval, int maxValColour[maxval]){
    int maxIndex = 0;
    for(int i=0;i<maxval+1;i++){
        if(maxValColour[i] > maxIndex) maxIndex = i;
    }
    return maxIndex;
}

// make all values of an integer array 0
void zeroIntArray(int n, int xs[n]){
    for(int i=0; i<n;i++){
        xs[i] = 0;
    }
}

// call functions for calculations and writing of file for the idea that:
// the lines do not have to be written in the colour of the most occuring colour if
// you set the background to the most occuring colour
int setBGMajority(FILE *out, int maxval, int H, int W, unsigned char image[H][W]){
    int maxValColour[maxval+1];
    zeroIntArray(maxval+1,maxValColour);
    int maxColour = 0;

    for(int i=0;i<H;i++){
        for(int j=0;j<W;j++){
            maxValColour[image[i][j]]++;
        }
    }
    
    maxColour = indexOfMaxVal(maxval, maxValColour);
    writeBackground(out, maxval, H, W, maxColour);

    return maxColour;
}

// create a 3d array of line data for each line for each colour
void giveColourLineValues(int tot, int numColours,int numLines[tot], lineData *colourLines,int H, int W, unsigned char image[H][W]){
    int currentColour = -1; 
    int prevcolour = -1;
    int currentLength = 0;
    int xStart = 0;
    int yStart = 0;
    
    for(int i=0;i<H;i++){
        for(int j=0;j<W;j++){
            currentColour = image[i][j];
            // if this colour is no the same as the last then add a new line for the respective colour
            if(currentColour == prevcolour || prevcolour <0) currentLength++;
            else{
                setItem(colourLines,prevcolour,numLines[prevcolour],0,xStart);
                setItem(colourLines,prevcolour,numLines[prevcolour],1,yStart);
                setItem(colourLines,prevcolour,numLines[prevcolour],3, currentLength);

                numLines[prevcolour]++;
                // new colour has been seen once
                currentLength=1;
                xStart=j;
                yStart = i;
            }
            prevcolour = currentColour;
        }
        // if a new line in the image always end the current drawing line and 
        // start a new one
        setItem(colourLines,prevcolour,numLines[prevcolour],0,xStart);
        setItem(colourLines,prevcolour,numLines[prevcolour],1,yStart);
        setItem(colourLines,prevcolour,numLines[prevcolour],3, currentLength);

        numLines[prevcolour]++;
        prevcolour = -1;
        currentLength=0;
        xStart=0;
        yStart = i+1;
    }
    // reduce length of first line created by one due to prev taking one iteration to update
    incItem(colourLines,image[0][0],0,3);
}

// write each line from line data for each colour
// working in a loop around the majority colour
void putLinesToFile(int tot, int majorityGrey, FILE *outputFile,int Maxval, int numLines[tot], lineData *colourLines){
    // needed so that the background colour does not have lines written in the same colour
    int start = majorityGrey+1>Maxval? 0: majorityGrey +1;
    
    // loop from most common grey to white
    for(int i=start; i<=Maxval;i++){
        if(numLines[i]==0) continue;
        writeColour(outputFile, Maxval,i );
        for(int j=0;j<numLines[i];j++){
            writeLine(outputFile, getItem(colourLines,i,j,0),getItem(colourLines,i,j,1),getItem(colourLines,i,j,3));
        }
    }

    // work from black back to the grey value before the most common
    // -1 to exclude majority
    for(int i=0; i<start-1;i++){
        if(numLines[i]==0) continue;
        writeColour(outputFile, Maxval,i );
        for(int j=0;j<numLines[i];j++){
            writeLine(outputFile, getItem(colourLines,i,j,0),getItem(colourLines,i,j,1),getItem(colourLines,i,j,3));
        }
    }
}

// convert the grey values into sketch commands and output those to the correct file name
// use a RLE esque method on the colours to do lines instead of pixels
void convertAndPutDataSketch(FILE *outputFile, int Maxval, int H, int W, unsigned char image[H][W]){
    // 1) make the background the most occuring grey value
    int majorityGrey = setBGMajority(outputFile, Maxval, H, W, image);

    // 2) for all grey values (traversing from top left across then down):
    // 2.1) See if the pixels horizontally right are same colour
    // 2.1) continue in this direction to find all lines
    // 2.3) store start pixel, direction, length (even if length 1)

    // 4 for:
    //      x start
    //      y start
    //      is horizontal (1 or 0)
    //      length
    int tot = H*W;
    int numLines[tot];
    zeroIntArray(tot,numLines);
    lineData *colourLines = createLineData(Maxval+1,tot,4);
    giveColourLineValues(tot,Maxval+1,numLines,colourLines,H,W,image);

    // 3) colour all lines of one colour (starting at bg colour)
    // then move to the next colour sequentially
    putLinesToFile(tot,majorityGrey, outputFile, Maxval, numLines,colourLines);
    freeLineData(colourLines);
}

// only for instances when grey values are represented with 2 bytes
// convert the grey values into sketch commands and output those to the correct file name
// use a RLE esque method on the colours to do lines instead of pixels
void convertAndPutDataSketchLarge(FILE *outputFile, int Maxval, int H, int W, unsigned char largeImage[H][W]){
    W=W/2;
    unsigned char image[H][W];
    double val=0;

    // here the two bytes are condensed into the correct grey values so that they can 
    // be processed as normal
    for(int i=0; i<H;i++){
        for(int j=0;j<W;j++){
            val += largeImage[i][2*j] << 8;
            val += largeImage[i][2*j+1];
            val = (val/Maxval)*255 +0.5;
            image[i][j/2] = val;
            val=0;
        }
    }
    convertAndPutDataSketch(outputFile,255,H,W,image);
}

// extract header data
// extract the grey values
// use this to call the respective converting function
void makeSketch(FILE *in, FILE *out){
    const int max = 50;
    char header[max];
    fgets(header, max, in);
    strcat(header, " ");

    int H = getHeightHeader(header);
    int W = getWidthHeader(header);
    int Maxval = getMaxvalHeader(header);

    if(Maxval >255){
        // every pixel now has two bytes
        unsigned char image[H][W];
        getImage(in, H, W, image, true);

        convertAndPutDataSketchLarge(out, Maxval,H,W,image);
    }
    else{
        unsigned char image[H][W];
        getImage(in, H, W, image,false);

        convertAndPutDataSketch(out, Maxval,H,W,image);
    }
}

// get the output file name from the input file name
// then run the correct conveting function
int convert(char name[]){
    if(!acceptableFileName(name)){
        displayFileError();
        return 0;
    }

    bool convertingToSketch = toSketch(name);

    char outName[256];
    changeFileType(name,outName);

    FILE *in = fopen(name, "rb");
    FILE *out = fopen(outName, "wb");

    if(convertingToSketch) makeSketch(in,out);

    fclose(in);
    fclose(out);
    printf("File %s has been written\n", outName);
    return 0;
}

// ---------------- Testing Start -----------------

// Enumeration used for determining which function to call
enum {fileName, headerH, headerW, headerMV, background};
typedef int function;

// A replacement for the library assert function.
// Taken from sketch.c tests
static void assert(int line, bool b) {
  if (b) return;
  fprintf(stderr, "ERROR: The test on line %d in test.c fails.\n", line);
  exit(1);
}

static void assert2DLoop(int line, int dim1, int dim2, bool b ){
    if (b) return;
  fprintf(stderr, "ERROR: The test on line %d with dim1: %d, dim2: %d in test.c fails.\n", line, dim1, dim2);
  exit(1);
}

// Call respective function with given input
// Input: String | Output:Bool
bool callStringBool(function f, char dataIn[]) {
    bool result = false;
    switch (f) {
        case fileName: result = acceptableFileName(dataIn); break;
        default: assert(__LINE__, false);
    }
    return result;
}

// Call respective function with given input
// Input: String | Output: Int
int callStringInt(function f, char dataIn[]) {
    int result = -1;
    switch (f) {
        case headerH: result = getHeightHeader(dataIn); break;
        case headerW: result = getWidthHeader(dataIn); break;
        case headerMV: result = getMaxvalHeader(dataIn); break;
        default: assert(__LINE__, false);
    }
    return result;
}

// Checks that the expected output matches when data is run through
// Input: String | Output:Bool
bool checkStringBool(function f, char dataIn[], bool expOut) {
    return callStringBool(f, dataIn) == expOut;
}

// Checks that the expected output matches when data is run through
// Input: String | Output:Int
bool checkStringInt(function f, char dataIn[], int expOut) {
    return callStringInt(f, dataIn) == expOut;
}

// Checks that the expected output matches when data is run through
// Input: Unsigned char [][] | Output: Unsigned char [][]
bool checkUC2dArrUC2dArr(int h, int w, unsigned char dataIn[h][w], unsigned char expOut[h][w]) {
    for(int i=0; i<h; i++){
        for(int j=0;j<w;j++){
            if (dataIn[i][j] != expOut[i][j]) return false;
        }
    }

    return true;
}

// tests for converting file name
void checkValidNames(){
    assert(__LINE__, checkStringBool(fileName,"fractal.pgm",true));
    assert(__LINE__, checkStringBool(fileName,"bands.pgm",true));
    assert(__LINE__, checkStringBool(fileName,"bands.sk",true));
    assert(__LINE__, checkStringBool(fileName,"hello.txt", false));
    assert(__LINE__, checkStringBool(fileName,"fractal", false));
}

// tests for extraction of header data
void checkHeaderDataExtraction(){
    assert(__LINE__, checkStringInt(headerH,"P5 256 512 65535 ", 512));
    assert(__LINE__, checkStringInt(headerW,"P5 256 512 65535 ", 256));
    assert(__LINE__, checkStringInt(headerMV,"P5 256 512 65535 ", 65535));
}

// get the image data from a given file
void getImageTest(FILE *file, int H, int W, unsigned char image[H][W], bool large){
    const int max = W + 1;
    char line[max];

    fgets(line,max,file);
    char current = 0;
    for(int i=0;i<H;i++){
        for(int j=0;j<W;j++){
            current = fgetc(file);
            image[i][j] = *(unsigned char*)(&current);
        }
    }

}

// create a test file for one byte input data
void createPGMtest255(){
    // code given for writing a pgm file

    int H=50, W=50;
	unsigned char imageOut[H][W];

	//Scale 0 -> 255
	for (int i = 0; i < H; i++){
		for (int j = 0; j < W; j++){
			imageOut[i][j] = 255;
		}
	}

	FILE *ofp = fopen("test.pgm", "wb");
	if (ofp == NULL) {
		fprintf(stderr, "Cannot write test.pgm\n");
		exit(1);
	}

	//Write header
	fprintf(ofp,"P5 %d %d 255\n", W, H);
	//Write bytes
	fwrite(imageOut, 1, H*W, ofp);

	fclose(ofp);
}

// create a test file for two byte input data
void createPGMtest256(){
    // code given for writing a pgm file

    int H=50, W=100;
	unsigned char imageOut[H][W];

	//Scale 0 -> 256
	for (int i = 0; i < H; i++){
		for (int j = 0; j < W; j++){
            // most significant bytes are even (0,2,4...) so should be 1
            // least signifcant bytes are odd and should be 0
			imageOut[i][j] = j%2==0? 0x1 : 0x0;
		}
	}

	FILE *ofp = fopen("test2.pgm", "wb");
	if (ofp == NULL) {
		fprintf(stderr, "Cannot write test2.pgm\n");
		exit(1);
	}

	//Write header
	fprintf(ofp,"P5 %d %d 256\n", W, H);
	//Write bytes
	fwrite(imageOut, 1, H*W, ofp);

	fclose(ofp);
}

// tests for extracting large and small image data
void checkImageDataExtraction(){

    //create a fully black 50 by 50 image .PGM file for testing
    //max value set as 255 (1 byte long)
    createPGMtest255();

    int H=50,W=50;
    unsigned char image[50][50];
    unsigned char expOut[50][50];

    for (int i = 0; i < H; ++i){
		for (int j = 0; j < W; ++j){
			expOut[i][j] = 255;
		}
	}
    FILE *test = fopen("test.pgm", "rb");
    getImageTest(test, 50,50,image, false);
    fclose(test);

    //run test
    assert(__LINE__, checkUC2dArrUC2dArr(50,50,image, expOut));

    //create a fully black 50 by 50 image .PGM file for testing
    //max value set as 256 (2 bytes long)
    createPGMtest256();

    W=100;
    unsigned char image2[50][100];
    unsigned char expOut2[50][100];

    for (int i = 0; i < H; i++){
		for (int j = 0; j < W; j++){
			// most significant bytes are even (0,2,4...) so should be 1
            // leas signifcant bytes are odd and should be 0
			expOut2[i][j] = j%2==0? 1 : 0; 
		}
	}
    FILE *test2 = fopen("test2.pgm", "rb");
    getImageTest(test2,50,100,image2, true);
    fclose(test2);

    //run test
    assert(__LINE__, checkUC2dArrUC2dArr(50,100,image2, expOut2));
    
}

// creates a file that can be used by the sketch program
// it tests that a medium grey background is created
void checkBackgroundChange(){
    FILE *in = fopen("testBG.sk", "wb");
    //background should be set to a 'dark grey'
    //200x200 grid as specified
    writeBackground(in,255,200,200,40);
    fclose(in);
}

// create a PGM file where over half the pixels are one grey
// and the rest a different grey
// returns the majority grey value
int giveImageBGTest(int H, int W, unsigned char imageOut[H][W]){

	//Scale 0 -> 255
	for (int i = 0; i < H; i++){
		for (int j = 0; j < W; j++){
			if(j%3==0 || j%3 ==1)imageOut[i][j] = 128;
            else imageOut[i][j] = 0;
		}
	}

    return 128;
}

// tests for determining what colour appears the most in a PGM
void checkMajorityBackground(){
   unsigned char image[50][50];
   int majority = giveImageBGTest(50, 50, image);

   FILE *temp = fopen("temp", "wb");
   assert(__LINE__, majority == setBGMajority(temp, 255,50,50,image));
   fclose(temp);
}

// **image array creation with the following features:
    // there are 100 white lines
    // there are 99 half white half grey lines
    // there is one line that alternates between grey and black
void createHorizontalLineTest(int H, int W, unsigned char image[H][W]){
    // there are 100 white lines
    for(int i=0; i<100;i++){
        for(int j=0;j<200;j++){
            image[i][j] = 255;
        }
    }

    // there are 99 half white half grey lines
    for(int i=100; i<199;i++){
        for(int j=0;j<100;j++){
            image[i][j] = 255;
        }
    }
    for(int i=100; i<199;i++){
        for(int j=100;j<200;j++){
            image[i][j] = 128;
        }
    }

    // there is one line that alternates between grey and black
    for(int j=0;j<200;j++){
        image[199][j] = j%2==0? 128 : 0;
    }
}

// tests for determining wether lines are created as they should be
void checkColourLineValues(){
    // **image array creation with the following features:
    // there are 100 white lines
    // there are 99 half white half grey lines
    // there is one line that alternates between grey and black
    unsigned char image[200][200];
    createHorizontalLineTest(200,200,image);

    int numColours = 256;
    int tot = 200*200;
    int numLines[tot];
    zeroIntArray(tot,numLines);

    lineData *testing = createLineData(255,tot,4);

    FILE *test = fopen("htest.pgm","rb");
    giveColourLineValues(tot,numColours,numLines,testing,200,200,image);

    // second white line should be 200 long
    // starting at x=0 y=1
    assert(__LINE__,getItem(testing,255,1,3) == 200);
    assert(__LINE__,getItem(testing,255,1,0) == 0);
    assert(__LINE__,getItem(testing,255,1,1) == 1);
    
    // there should be 199 white lines in total
    // so the length should be 100 for the 199th white line
    assert(__LINE__,getItem(testing,255, 198,3) == 100);

    // the first 99 grey lines all of length 100
    assert(__LINE__,getItem(testing,128,0,3) == 100);
    assert(__LINE__,getItem(testing,128,98,3) == 100);

    // there should be 100 black lines all of length 1
    assert(__LINE__, getItem(testing,0,0,3) == 1);
    assert(__LINE__, getItem(testing,0,99,3) == 1);

    fclose(test);
    freeLineData(testing);
}

// checks that large grey value data is condensed into data that can be shown by sk
void checkLargeData(){
    unsigned char image[50][100];
    //Scale 0 -> 256
	for (int i = 0; i < 50; i++){
		for (int j = 0; j < 100; j++){
            // most significant bytes are even (0,2,4...) so should be 1
            // least signifcant bytes are odd and should be 0
			image[i][j] = j%2==0? 0x1 : 0x0;
		}
	}
    FILE *temp = fopen("temp.pgm","wb");
    convertAndPutDataSketchLarge(temp, 512,50,100,image);
    fclose(temp);
}

void checkLinesToSketch(){
    int H=200, W=200;
	unsigned char image[H][W];

	//Scale 0 -> 255
	// there are 100 white lines
    for(int i=0; i<100;i++){
        for(int j=0;j<200;j++){
            image[i][j] = 255;
        }
    }

    // there are 99 half white half grey lines
    for(int i=100; i<199;i++){
        for(int j=0;j<100;j++){
            image[i][j] = 255;
        }
    }
    for(int i=100; i<199;i++){
        for(int j=100;j<200;j++){
            image[i][j] = 128;
        }
    }

    // there is one line that alternates between grey and black
    for(int j=0;j<200;j++){
        image[199][j] = j%2==0? 128 : 0;
    }

	FILE *ofp = fopen("lineTest.pgm", "wb");
	if (ofp == NULL) {
		fprintf(stderr, "Cannot write test.pgm\n");
		exit(1);
	}

	//Write header
	fprintf(ofp,"P5 %d %d 255\n", W, H);
	//Write bytes
	fwrite(image, 1, H*W, ofp);

	fclose(ofp);
}

// create a test file that prints contigous grey values
void createContig(){
    // code given for writing a pgm file

    int H=50, W=50;
	unsigned char imageOut[H][W];

	//Scale 0 -> 256
	for (int i = 0; i < H; i++){
		for (int j = 0; j < W; j++){
            imageOut[i][j] = (i*H+j)%256;
		}
	}

	FILE *ofp = fopen("testCont.pgm", "wb");
	if (ofp == NULL) {
		fprintf(stderr, "Cannot write test2.pgm\n");
		exit(1);
	}

	//Write header
	fprintf(ofp,"P5 %d %d 255\n", W, H);
	//Write bytes
	fwrite(imageOut, 1, H*W, ofp);

	fclose(ofp);
}

void checkContig(){
    createContig();
    unsigned char image[50][50];
    for (int i = 0; i < 50; i++){
		for (int j = 0; j < 50; j++){
            image[i][j] = 0;
		}
	}
    FILE *in = fopen("testCont.pgm", "rb");
    getImageTest(in, 50,50,image,false);
    for (int i = 0; i < 50; i++){
		for (int j = 0; j < 50; j++){
            assert2DLoop(__LINE__,i,j,image[i][j] == (i*50+j)%256);
		}
	}
}

// Run all tests
void tests(){

    checkValidNames();
    checkHeaderDataExtraction();
    checkImageDataExtraction();
    checkMajorityBackground();
    checkColourLineValues();
    checkLargeData();
    checkLinesToSketch();
    checkContig();

    printf("All tests run OK!\n");
}

// ------------------ Main -----------------

// Run testing if no arguments
// Accept only 1 additional argument - the name of the file to convert
int main(int n, char*args[n]){
    switch(n){
        case 1: tests(); break;
        case 2: convert(args[1]); break;
        default: displayFileError();
    }
    return 0;
}
