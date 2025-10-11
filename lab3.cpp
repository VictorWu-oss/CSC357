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

// int argc, char *argv[]
int main() {
    // argc: # of arguments, argv: array of strings
    // Expects: ./[program name] [input.bmp] [output.bmp] [# of processes (1-4)] [angle in rads(float)]
    //             [argv[0]]      [argv[1]]   [argv[2]]         [argv[3]]               [argv[4]]

    // const char *input = argv[1];
    // const char *output = argv[2];
    // int operation = stoi(argv[3]);
    // float factor = atof(argv[4]);

    // Open input file in read and declare structs for headers
    const char *input = "jar.bmp";
    const char *output = "out.bmp";
    int operation = 1;
    float factor = 100.0f;

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

    // Map entire input file
    int in_fd = fileno(f);
    struct stat st{};
    fstat(in_fd, &st);
    size_t in_size = (size_t)st.st_size;

    // Allocate some mem,'Data', to hold pixel data using Mmap
    // Always CLOSE the file pter or OPENER for writing when FORKING
    //BYTE *data = (BYTE*)malloc(dataSize);
    BYTE *data_in = (BYTE*)mmap(nullptr, in_size, PROT_READ, MAP_PRIVATE, in_fd,0);

    // Prepare output file headers for writing later
    size_t out_size = (size_t)fh.bfOffBits + dataSize;
    FILE *out = fopen(output, "wb+");
    fseek(out, out_size - 1, SEEK_SET);
    fputc('\0', out);

    BYTE *out_data = (BYTE*)mmap(nullptr, out_size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(out), 0);

    // Copy original data to mapped region
    memcpy(out_data, data_in, fh.bfOffBits);

    tagBITMAPFILEHEADER *ofh = (tagBITMAPFILEHEADER *)out_data;
    tagBITMAPINFOHEADER *oih = (tagBITMAPINFOHEADER *)(out_data + sizeof(tagBITMAPFILEHEADER));
    oih -> biSizeImage = (DWORD)dataSize;
    ofh -> bfSize = (DWORD)out_size;

    // Pters to pixel blocks
    BYTE *src = data_in + fh.bfOffBits;
    BYTE *dst = out_data + fh.bfOffBits;

    fclose(f);

    // Begin processing the pixel data
    // Iterate through rows (y) and columns (x)
    for (int y = 0; y < height; y++) 
    {
        for (int x = 0; x < width; x++) 
        {
            // Pointer to pixel (BGR) inside the row with padding
            // File row index bottom up starting from bottom left
            BYTE *pixel = dst + (size_t)(y * rowSize + (size_t)x * bytesPerPixel);


            // Shifts origin to the center
            // Tracing backward from the OUTPUT pixel to figure out where its color originally came from before rotation
            double x_r = x - width/2.0;
            double y_r = y - height/2.0;

            // Apply the inverse rotation -alpha
            double x_ot = x_r * cos(factor) + y_r * sin(factor);
            double y_ot = -x_r * sin(factor) + y_r * cos(factor);

            // Shift back to normal coordinates, add the middle coords back
            double x_o = x_ot + width/2.0;
            double y_o = y_ot + height/2.0;

            int sx = (int)lrint(x_o);
            int sy = (int)lrint(y_o);

            if (sx >= 0 && sx < width && sy >= 0 && sy < height) 
            {
                // Pointer to the original pixel (BGR) inside the row with padding
                int fsy = (height - 1) - sy;
                BYTE *spx = src + (size_t)fsy * rowSize + (size_t)sx * bytesPerPixel;

                // BGR pixels
                pixel[0] = spx[0]; // Blue
                pixel[1] = spx[1]; // Green
                pixel[2] = spx[2]; // Red

                if (bytesPerPixel == 4) 
                {
                    pixel[3] = spx[3]; // Alpha
                }   
            }

            else
            {
                pixel[0] = pixel[1] = pixel[2] = 0; // Black
                if (bytesPerPixel == 4) 
                {
                    pixel[3] = 0; // Alpha
                }
            }
        }

        int srcRow = (height - 1) - y;
        memcpy(dst + (size_t)y * rowSize + padding, 
        src + (size_t)srcRow * rowSize + (size_t)rowSize,
        (size_t)padding);
    }

    // Flush and close
    msync(out_data, out_size, MS_SYNC);
    munmap(out_data, out_size);
    fclose(out);

    munmap(data_in, in_size);
    return 0;
}
