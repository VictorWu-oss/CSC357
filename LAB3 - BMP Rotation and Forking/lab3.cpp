#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include <sys/wait.h>
using namespace std;

typedef unsigned short WORD;    
typedef unsigned int DWORD; 
typedef signed int LONG;
typedef unsigned char BYTE;

#pragma pack(push,1)

struct tagBITMAPFILEHEADER
{
    WORD bfType;        // specifies the file type for BMP, needs to start with "BM"
    DWORD bfSize;       // specifies the size in bytes of the bitmap file
    WORD bfReserved1;   // reserved; must be zero
    WORD bfReserved2;   // reserved; must be zero
    DWORD bfOffBits;    // specifies the offset to the start of the pixel data
};

struct tagBITMAPINFOHEADER
{
    DWORD biSize;           // specifies the number of bytes required by the struct
    LONG biWidth;           // specifies width in pixels
    LONG biHeight;          // specifies height in pixels
    WORD biPlanes;          // specifies the number of color planes
    WORD biBitCount;        // specifies the number of bits per pixel
    DWORD biCompression;    // specifies the type of compression
    DWORD biSizeImage;      // specifies the size of the image data
    LONG biXPelsPerMeter;   // specifies the horizontal resolution
    LONG biYPelsPerMeter;   // specifies the vertical resolution
    DWORD biClrUsed;        // specifies the number of colors in the color palette
    DWORD biClrImportant;   // specifies the number of important colors
};
#pragma pack(pop)

void rotate(BYTE *data, BYTE *src, int width, int height, int rowSize, int bytesPerPixel, float angle, int start, int end)
{
    // Iterate through the output rows (y) and columns (x)
    for (int y = start; y <= end; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            // Pointer to pixel (BGR) inside the row with padding
            // File row index bottom up starting from bottom left
            BYTE *pixel = data + (size_t)(y * rowSize + (size_t)x * bytesPerPixel);
        

            // Turns (0,0) into the center of the image in the output
            // Tracing backward from the OUTPUT pixel to figure out where its color originally came from before rotation
            double x_r = x - width/2.0;
            double y_r = y - height/2.0;

            // Apply the inverse rotation -alpha
            // Gives original (unrotated) coordinates relative to center
            double x_ot = x_r * cos(angle) + y_r * sin(angle);
            double y_ot = -x_r * sin(angle) + y_r * cos(angle);

            // Shift back to normal coordinates (center of input), now x_o and y_o are the original coordinates 
            double x_o = (int)lrint(x_ot + width/2.0);
            double y_o = (int)lrint(y_ot + height/2.0);

            // If within bounds, sample the pixel from the source to destination
            if (x_o >= 0 && x_o < width && y_o >= 0 && y_o < height) 
            {
                // Pointer to the original pixel (BGR) inside the row with padding
                BYTE *spx = src +(size_t)y_o * rowSize + (size_t)x_o * bytesPerPixel;
                pixel[0] = spx[0]; // Blue
                pixel[1] = spx[1]; // Green
                pixel[2] = spx[2]; // Red
            }

            // Else out of bounds turn pixel black
            else
            {
                pixel[0] = pixel[1] = pixel[2] = 0; // Black
            }
        }
    }
}

// int argc, char *argv[]
int main(int argc, char *argv[]) {
    // argc: # of arguments, argv: array of strings
    // Expects: ./[program name] [input.bmp] [output.bmp] [# of processes (1-4)] [angle in degrees]
    //             [argv[0]]      [argv[1]]   [argv[2]]         [argv[3]]               [argv[4]]

    const char *input = argv[1];
    const char *output = argv[2];
    int operation = stoi(argv[3]);
    float factor = atof(argv[4]);   // rotation in degrees in counterclockwise direction

    // This is for testing purposes
    // const char *input = "PLTL2.bmp";
    // const char *output = "out.bmp";
    // int operation = 1;
    // float factor = 2.0f;

    // Open input file in read and declare structs for headers
    FILE *f = fopen(input, "rb");
    tagBITMAPFILEHEADER fh;
    tagBITMAPINFOHEADER ih;

    // fread(pointer to data, size of each ele, # of element, file pointer)
    // Read bytes into header structs to match BMP memory format
    fread(&fh, sizeof(fh), 1, f);
    fread(&ih, sizeof(ih), 1, f);

    // Now extract info from the headers which was just read
    int width = ih.biWidth;
    int height = abs((int)ih.biHeight);
  
    // Remember each pixel = 3 bytes (BGR), each color is 1 byte (8bits)
    // 8 bits = 1 byte
    // 24 bits = 3 bytes per pixel

    // bytes per pixel = 24 / 8
    int bytesPerPixel = ih.biBitCount / 8;

    // width in pixels * bytes
    int unpadrowSize = (width * bytesPerPixel);

    // padding for multiples of 4 bytes, last % is when already multiple of 4 and remainder is 0.
    int padding = (4 - (unpadrowSize % 4)) % 4;
    int rowSize =  unpadrowSize + padding;

    // Total byte of ALL rows
    size_t dataSize = (size_t)rowSize * (size_t)height;

    // Allocate some mem,'Data', to hold pixel data using Mmap
    // Always CLOSE the file pter or OPENER for writing when FORKING
    //BYTE *data = (BYTE*)malloc(dataSize);
    BYTE *data = (BYTE*)mmap(NULL, dataSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Load source pixels into separate buffer
    fseek(f, fh.bfOffBits, SEEK_SET);
    BYTE *src = (BYTE*)mmap(NULL, dataSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fread(src, 1, dataSize, f);
    fclose(f);


    // Convert factor from degrees to radians
    factor = factor * (M_PI / 180.0);

    clock_t start = clock();

    // // Fork for multiple processes
    for (int i = 0; i < operation; i++)
    {
        if (fork() == 0)
        {
            rotate(data, src, width, height, rowSize, bytesPerPixel, factor, (dataSize / operation) * i, (dataSize / operation) * (i + 1));
            //rotate(data + (dataSize / operation) * i, src + (dataSize / operation) * (i + 1), width, height, rowSize, bytesPerPixel, factor);

            return 0;
        }
    }

    //rotate(data + (operation) * (dataSize / operation), src + (operation ) * (dataSize / operation) , newWidth, newHeight / operation, rowSize, bytesPerPixel, factor);
    
    // 3 Ways to wait for child processes to finish
    // 1. wait for a specific pid
    //for (int i = 0; i < operation; i++) wait(0);

    // 2. wait for all child processes to finish
    //while (wait(0) != -1);

    // 3. wait for any child process to finish
    while (waitpid(-1, NULL, 0) > 0);

    clock_t end = clock();
    double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
    cout << "Elapsed time: " << elapsed_secs << " seconds" << endl;
    cout.flush();

    // Write to output file
    FILE *out = fopen(output, "wb");
    ih.biSizeImage = (DWORD)dataSize;
    fh.bfSize = fh.bfOffBits + ih.biSizeImage;
    fwrite(&fh, sizeof(fh), 1, out);
    fwrite(&ih, sizeof(ih), 1, out);
    fwrite(data, 1, dataSize, out);

    // Flush and close
    msync(data, dataSize, MS_SYNC);
    munmap(data, dataSize);
    munmap(src, dataSize);
    fclose(out);

    return 0;
}
