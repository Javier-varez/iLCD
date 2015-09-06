#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

typedef struct __attribute__((__packed__)) {
    char bfType[2];		// 0
    uint32_t bfSize;		// 2
    uint32_t bfReserved;	// 6
    uint32_t bfOffBits;		// 10
} BMP_FileHeader;		// 14 bits total

typedef struct __attribute__((__packed__)) {
    uint32_t biSize;		// 0
    uint32_t biWidth;		// 4
    int32_t biHeight;		// 8
    uint16_t biPlanes;		// 12
    uint16_t biBitCount;	// 14
    uint32_t biCompression;	// 16
    uint32_t biSizeImage;	// 20
    uint32_t biXPelsPerMeter;	// 24
    uint32_t biYPelsPerMeter;	// 28
    uint32_t biClrUsed;		// 32
    uint32_t biClrImportant;	// 36
} BMP_ImageHeader;		// 40 bits total

void rejectFile(FILE *file, char name[]);
uint8_t obtainBMPFileHeader(FILE *fp, BMP_FileHeader *fileHeader);
uint8_t obtainBMPImageHeader(FILE *fp, BMP_ImageHeader *ImageHeader);
uint8_t parseBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile, uint8_t orientation);
uint8_t parseVBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile);
uint8_t parseHBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile);
void printResult(uint8_t orientation, BMP_ImageHeader *imageHeader, uint8_t *decodedFile, char *arrayName);
void toUppercase(char from[], char to[]);
void usage();

// Main Function
int main(int argc, char *argv[]) {

    char arrayName[11];
    char orientation = 'v';

    strcpy(arrayName, "image");

    // If call had no arguments
    if (argc == 1) {
      usage();
      return 0;
    }

    // Parse all the options
    int option = 0;
    while ((option = getopt(argc, argv, "vhn:")) != -1) {
      switch (option) {
        case 'n':
          strcpy(arrayName, optarg);
          break;
        case 'v':
          break;
        case 'h':
          orientation = 'h';
          break;
        case '?':
          usage();
          return 0;
      }
    }
    argc -= optind;
    argv += optind;

    // If no file name was provided after the options
    if (argc == 0) {
      usage(); // Display usage and exit;
      return 0;
    }

    // Open provided file
    char fileName[51];
    strcpy(fileName, argv[0]);
    FILE *wFile = fopen(fileName, "r");

    BMP_FileHeader fileHeader;
    BMP_ImageHeader imageHeader;

    // Check file for format issues
    uint8_t valid_file = 0;

    valid_file = (obtainBMPFileHeader(wFile, &fileHeader) == 0);
    if (valid_file) valid_file = (obtainBMPImageHeader(wFile, &imageHeader) == 0);

    while (!valid_file) {
        rejectFile(wFile, fileName);
        valid_file = (obtainBMPFileHeader(wFile, &fileHeader) == 0);
        if (valid_file) valid_file = (obtainBMPImageHeader(wFile, &imageHeader) == 0);
    }

    // Allocate enough memory to hold the image
    uint8_t *decodedFile;
    if (orientation == 'v')  decodedFile = calloc(imageHeader.biWidth * ceil(imageHeader.biHeight/8.0), 1);
    else decodedFile = calloc(imageHeader.biHeight * ceil(imageHeader.biWidth/8.0), 1);

    // parse the image file and collect the data in an ordered fashion.
    parseBMPImage(wFile, &fileHeader, &imageHeader, decodedFile, orientation);

    // print the result to stdout
    printResult(orientation, &imageHeader, decodedFile, arrayName);

    // Dispose of the pointers and free memory on the heap.
    free(decodedFile);
    fclose(wFile);

    return 0;
}

void rejectFile(FILE *file, char name[]) {
  fclose(file);
  fprintf(stderr, "Please, provide a valid 1-bit BMP file: ");
  scanf("%s", name);
  file = fopen(name, "r");
}

uint8_t obtainBMPFileHeader(FILE *fp, BMP_FileHeader *fileHeader) {

    //Set Start Position
    fseek(fp, 0, SEEK_SET);

    // Read file Header
    fread(fileHeader, sizeof(BMP_FileHeader), 1, fp);

    // Check for correct file format
    if ((fileHeader->bfType[0] != 'B') || (fileHeader->bfType[1] != 'M')) {
        fprintf(stderr, "Error: file is not a bitmap\n");
	      return -1;
    }

    return 0;
}

uint8_t obtainBMPImageHeader(FILE *fp, BMP_ImageHeader *ImageHeader) {
  fseek(fp, 14, SEEK_SET);

  // Read file Header
  fread(ImageHeader, sizeof(BMP_ImageHeader), 1, fp);

  if (ImageHeader->biClrUsed > 0) {
    fprintf(stderr, "Color map entries: %d\n", ImageHeader->biClrUsed);
    fprintf(stderr, "Error, file can't use color map entries, should be 1-bit color depth\n");
    return -1;
  }

  if (ImageHeader->biCompression != 0) {
    fprintf(stderr, "Error, file can't be compressed\n");
    return -1;
  }

  return 0;
}

uint8_t parseBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile, uint8_t orientation) {
  if (orientation == 'v') return parseVBMPImage(fp, fileHeader, imageHeader, decodedFile);
  return parseHBMPImage(fp, fileHeader, imageHeader, decodedFile);
}

uint8_t parseVBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile) {
  uint32_t rowLength = ceil(imageHeader->biWidth/32.0)*4; // Row length in bytes

// We need to go through all the rows and process them...
  uint32_t i = 0;
  for (i = 0; i < imageHeader->biHeight; i++) {
    uint32_t rowPointer = (rowLength * (imageHeader->biHeight - 1 - i)) + fileHeader->bfOffBits;
    fseek(fp, rowPointer, SEEK_SET);

    uint8_t *currentRow = malloc(rowLength);
    fread(currentRow, sizeof(uint8_t), rowLength, fp);

    // Now lets translate the content of each row to the output array
    uint32_t j = 0;
    for (j = 0; j < ceil(imageHeader->biWidth/8.0); j++) {
      uint32_t k = 0;
      for (k = 0; k < 8; k++) {
        if ((k+j*8) >= imageHeader->biWidth) break; // In this case, the image has been padded to complete the multiplicity of 4 bytes
        // That's some crazy shit goin' on there!
        decodedFile[(i/8)*imageHeader->biWidth + 8*j + k] |= (currentRow[j] & (0x80 >> k)) ? 1 << i%8 : 0;
      }
    }

    free(currentRow);
  }

  return 0;
}

uint8_t parseHBMPImage(FILE *fp, BMP_FileHeader *fileHeader, BMP_ImageHeader *imageHeader, uint8_t *decodedFile) {
  uint32_t rowLength = ceil(imageHeader->biWidth/32.0)*4; // Row length in bytes

  uint32_t i = 0;
  for (i = 0; i < imageHeader->biHeight; i++) {
    uint32_t rowPointer = (rowLength * (imageHeader->biHeight - 1 - i)) + fileHeader->bfOffBits;
    fseek(fp, rowPointer, SEEK_SET);

    uint8_t *currentRow = malloc(rowLength);
    fread(currentRow, sizeof(uint8_t), rowLength, fp);

    uint32_t j = 0;
    for (j = 0; j < ceil(imageHeader->biWidth/8.0); j++) {
      decodedFile[j + i*(int32_t)ceil(imageHeader->biWidth/8.0)] = currentRow[j];
    }
    free(currentRow);
  }
  return 0;
}

void printResult(uint8_t orientation, BMP_ImageHeader *imageHeader, uint8_t *decodedFile, char *arrayName) {
  // Print to screen the generated code.
  uint32_t i = 0;

  // Create uppercase arrayName
  char upperArrayName[11];
  toUppercase(arrayName, upperArrayName);

  // Set number of rows and columns to display
  uint32_t rows = (orientation == 'v') ? ceil(imageHeader->biHeight/8.0) : imageHeader->biHeight;
  uint32_t columns = (orientation == 'v') ? imageHeader->biWidth : ceil(imageHeader->biWidth/8.0);

  // #define upperArrayName_WIDTH
  // #define upperArrayName_HEIGHT
  printf("\n\n#define %s_WIDTH %d\n", upperArrayName, imageHeader->biWidth);
  printf("#define %s_HEIGHT %d\n", upperArrayName, imageHeader->biHeight);

  // declare and define the array
  printf("\n\nconst uint8_t %s[] = {\n", arrayName);

  // Print each hex value
  for(i = 0; i < rows; i++) {
    uint32_t j = 0;
    for (j = 0; j < columns; j++) {
      printf(" 0x%.2x", decodedFile[i*columns + j]);
      if ((columns * i + j) != rows * columns - 1) printf(","); // No comma needed if it's last value
    }
    printf("\n");
  }

  // Print closure
  printf("};\n");
}

void toUppercase(char from[], char to[]) {
  int i;
  for (i = 0; i < strlen(from); i++) {
    to[i] = toupper(from[i]);
  }
  to[strlen(from)] = 0x00;
}

void usage() {
  fprintf(stderr, "usage:\t\tiLCD\t\t[-n arrayName] [-v] [-h] file_name\n");
  fprintf(stderr, "options:\t-v:\t\tsets vertical byte orientation (default)\n");
  fprintf(stderr, "\t\t-h:\t\tsets horizontal byte orientation\n");
  fprintf(stderr, "\t\t-n:arrayName\tsets the name of the generated array\n");
}
