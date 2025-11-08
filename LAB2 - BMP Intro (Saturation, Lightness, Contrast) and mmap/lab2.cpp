#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <ctime>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

typedef unsigned short WORD;    
typedef unsigned int DWORD; 
// Change long to signed int
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

// For each row (y, height) and each column (x, width)
// y=0 bottom row to top row
// x=0 left to right each column 
// (bottom left to bottom right)
void process (BYTE *data, int width, int height, int rowSize, int bytesPerPixel, string operation, double factor) 
    {
        for (int y = 0; y < height; y++) 
        {
            for (int x = 0; x < width; x++) 
            {
                // Pointer to pixel (BGR) inside the row with padding
                BYTE *pixel = data + (y * rowSize + x * bytesPerPixel);

                // Initialize pixels as normal
                float blue = pixel[0] / 255.0f;
                float green = pixel[1] / 255.0f;
                float red = pixel[2] / 255.0f;

                // Apply operation
                if (operation == "contrast") 
                {
                    blue = powf(blue, (float)factor);
                    green = powf(green, (float)factor);
                    red = powf(red, (float)factor);

                } 

                else if (operation == "saturation") 
                {
                    float average = (red + green + blue) / 3.0f;
                    red = red + (red - average) * (float)factor;
                    green = green + (green - average) * (float)factor;
                    blue = blue + (blue - average) * (float)factor;

                } 

                else if (operation == "lightness") 
                {
                    red = red + (float)factor;
                    green = green + (float)factor;
                    blue = blue + (float)factor;
                }

                // POST-OPERATIOn forces pixel  colors values to be between [0,1] and convert to bytes with rounding
                blue = max(0.0f, min(1.0f, blue));
                green = max(0.0f, min(1.0f, green));
                red = max(0.0f, min(1.0f, red));
                pixel[0] = (BYTE)lrintf(blue * 255.0f);
                pixel[1] = (BYTE)lrintf(green * 255.0f);
                pixel[2] = (BYTE)lrintf(red * 255.0f);
            }
        }
    }

void flip(BYTE *data, int width, int height, int rowSize, int bytesPerPixel) 
    {
        // Outside -> in swapping
        // Loop through top half of the rows
        for (int y = 0; y < height / 2; y++) 
        {
            for (int x = 0; x < width; x++) 
            {
                // Top pixel is addy of pixel at very top half 1st row
                // y * rowSize = start of row
                // x * bytesPerPixel = move x pixels/ start of pixel in that row
                BYTE *topPixel = data + (y * rowSize + x * bytesPerPixel);

                // Bottom pixel is addy of pixel in bottom half, 0,0
                // Very top row is height - 1 - y, and it keeps decreasing as y increases
                // SO youre moving down
                // rowSize * (height - 1 - y) = start of row
                // x * bytesPerPixel = move x pixels/ start of pixel in that row
                BYTE *bottomPixel = data + ((height - 1 - y) * rowSize + x * bytesPerPixel);

                // Swap the pixels
                for (int i = 0; i < bytesPerPixel; i++) 
                {
                    BYTE temp = topPixel[i];
                    topPixel[i] = bottomPixel[i];
                    bottomPixel[i] = temp;
                }
            }
        }
    }

void flipSide(BYTE *data, int width, int height, int rowSize, int bytesPerPixel) 
    {
        // Outside -> in swapping
        // Loop through all rows
        // Loop through left half of the columns
        for (int y = 0; y < height; y++) 
        {
            for (int x = 0; x < width / 2; x++) 
            {
                // Left pixel is addy of pixel at very top half 1st row
                // y * rowSize = start of row
                // x * bytesPerPixel = move x pixels/ start of pixel in that row
                BYTE *leftPixel = data + (y * rowSize + x * bytesPerPixel);

                // Right pixel is addy of pixel in the same row, mirrored horizontally
                // x * bytesPerPixel = move x pixels/ start of pixel in that row
                // Doing width - 1 -x means starting on the right, width - 1 = last row
                // Increasing x will move the pixel from right to left
                BYTE *rightPixel = data + (y * rowSize + (width - 1 - x) * bytesPerPixel);

                // Swap the pixels
                for (int i = 0; i < bytesPerPixel; i++) 
                {
                    BYTE temp = leftPixel[i];
                    leftPixel[i] = rightPixel[i];
                    rightPixel[i] = temp;
                }
            }
        }
    }

int main(int argc, char *argv[]) {
    // argc: # of arguments, argv: array of strings
    // Expects: ./[program name] [input.bmp] [output.bmp] [operation] [factor]
    //             [argv[0]]      [argv[1]]   [argv[2]]   [argv[3]]   [argv[4]]

    const char *input = argv[1];
    const char *output = argv[2];
    string operation = argv[3];
    double factor = atof(argv[4]);

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
    int dataSize = rowSize * height;

    // Allocate some mem,'Data', to hold pixel data
    // BYTE *data = (BYTE*)malloc(dataSize);
    // Using mmmap
    BYTE *data = (BYTE*)mmap(NULL, dataSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // fseek moves file pointer to the start of pixel data (fh.bfOffBits is the offset to the start of pixel data)
    // SEEK_SET LITERALLY means from the beginning of the file
    fseek(f, fh.bfOffBits, SEEK_SET);

    // Read dataSize bytes into data buffer
    if (fread(data, 1, dataSize, f) != (size_t)dataSize) 
    { 
        fclose(f); 
        munmap(data, dataSize);
        return 1; 
    }

    fclose(f);

    // Begin processing the pixel data
    // Iterate through rows (y) and columns (x)
    clock_t start = clock();
    pid_t pid2, pid3;       // Declare pid2 and pid3 in the parent scope
    pid_t pid = fork();

    if (pid == 0) 
    {   // Child process
            // Process bottom-left QUADRANT of image
            // Since starting at (0,0) already at bottom-left
            // Left: width/2
            // Bottom: height/2
            process(data, width/2, height/2, rowSize, bytesPerPixel, operation, factor);

            // Exit the child process normally
            return 0;
    }

    else
    {
        // Parent process
        // Fork for multiple processes
        pid2 = fork();
        if (pid2 == 0) 
        {
            // Process bottom-right QUADRANT of image
            // Right half base: data + (width/2 * bytesPerPixel)
            // Right: width - width/2
            // Bottom: height/2
            // Parent for other child, but now child for the other processes
            process(data + (width/2 * bytesPerPixel), width - width/2, height/2, rowSize, bytesPerPixel, operation, factor);
            return 0;
        }
        else 
        {
            // Process top-left QUADRANT of image
            // Top half base: data + (height/2 * rowSize)
            // Left: width/2
            // Top: height - height/2
            // Parent for other child, but now parent for the other processes
            process(data + (height/2 * rowSize), width/2, height - height/2, rowSize, bytesPerPixel, operation, factor);
            
            // Fork for multiple processes
            pid3 = fork();
            if (pid3 == 0) 
            {
                // Process top-right QUADRANT of image
                // Top-right half base: data + (height/2 * rowSize) + (width/2 * bytesPerPixel)
                // Right: width - width/2
                // Top: height - height/2
                process(data + (height/2 * rowSize) + (width/2 * bytesPerPixel), width - width/2, height - height/2, rowSize, bytesPerPixel, operation, factor);
                return 0;
            }
        }
    }

    // Wait for all child processes to finish  
    int status;
    waitpid(pid, &status, 0); 
    waitpid(pid2, &status, 0);
    waitpid(pid3, &status, 0);

    // Flip upside down and left to right
    flip(data, width, height, rowSize, bytesPerPixel);
    flipSide(data, width, height, rowSize, bytesPerPixel);
    clock_t end = clock();
    double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
    cout << "Elapsed time: " << elapsed_secs << " seconds\r";
    cout.flush();

    // End processing pixel data
    cout << endl;

    // Update size fields (recompute image size and total file size)
    ih.biSizeImage = (DWORD)dataSize;
    fh.bfSize = fh.bfOffBits + ih.biSizeImage;

    // Write output file: headers then pixel data
    FILE *out = fopen(output, "wb");
    fwrite(&fh, sizeof(fh), 1, out);
    fwrite(&ih, sizeof(ih), 1, out);
    fwrite(data, 1, dataSize, out);
    fclose(out);

    munmap(data, dataSize);
    // free(data);
    return 0;
}
