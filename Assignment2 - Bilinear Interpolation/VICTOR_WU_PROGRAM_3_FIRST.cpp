#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <time.h>
#include <algorithm>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h> 
#include <fcntl.h>
#include <chrono>

using namespace std; 
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char BYTE; //match BMP layout  

struct tagBITMAPFILEHEADER{
    WORD bfType; //specifies the file type
    DWORD bfSize; //specifies the size in bytes of the bitmap file
    WORD bfReserved1; //reserved; must be 0
    WORD bfReserved2; //reserved; must be 0
    DWORD bfOffBits; //species the offset in bytes from the bitmapfileheader to the bitmap bits
};

struct tagBITMAPINFOHEADER{
    DWORD biSize; //specifies the number of bytes required by the struct
    LONG biWidth; //specifies width in pixels
    LONG biHeight; //species height in pixels
    WORD biPlanes; //specifies the number of color planes, must be 1
    WORD biBitCount; //specifies the number of bit per pixel
    DWORD biCompression;//spcifies the type of compression
    DWORD biSizeImage; //size of image in bytes
    LONG biXPelsPerMeter; //number of pixels per meter in x axis
    LONG biYPelsPerMeter; //number of pixels per meter in y axis
    DWORD biClrUsed; //number of colors used by th ebitmap
    DWORD biClrImportant; //number of colors that are important
};

// Gets data array, the width and height, the COORD of pixels, color (R, G, B), and returns the color on the given xy.
unsigned char get_color(BYTE *imagedata, int x, int y, int width, int height, int padding, int color_channel) 
{
    // Keep within bounds
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;

    // Calculate pixel position: row * row_size + column * 3 (BGR format)
    BYTE *pixel = imagedata + (y * (width * 3 + padding) + x * 3);
    
    // BMP stores pixels as BGR (Blue, Green, Red)
    // color_channel: 0 = Blue, 1 = Green, 2 = Red
    return pixel[color_channel];
}

// Calculate a color value at a floating point COORD for the smaller image, for any colorvia interpolating b/t the 4 surrounding pixels
unsigned char get_color_bilinear(BYTE *imagedata, float x, float y, int width, int height, int padding, int color_channel) {
    // Get the four surrounding pixels as a grid format AROUND floats (x, y)

    // Take integer parts of x and y, lower bdds
    int x1 = (int)floor(x);
    int y1 = (int)floor(y);

    // Get next integer coords, upper bbds
    int x2 = x1 + 1;
    int y2 = y1 + 1;

    // Bound to image bdd
    if (x2 >= width) x2 = width - 1;
    if (y2 >= height) y2 = height - 1;

    // Get color of the four corners
    BYTE left_lower = get_color(imagedata, x1, y1, width, height, padding, color_channel);
    BYTE right_lower = get_color(imagedata, x2, y1, width, height, padding, color_channel);
    BYTE left_upper = get_color(imagedata, x1, y2, width, height, padding, color_channel);
    BYTE right_upper = get_color(imagedata, x2, y2, width, height, padding, color_channel);

    // Calculate fractional offsets 
    // DX how far between left and right pixels, x = 3.25, tx = 0.25 = 25% between left and right pixel
    // DY how far between top and bottom pixels
    float tx = x - x1;
    float ty = y - y1;

    // Perform bilinear interpolation per column, blend between upper/ lower on both left and right sides
    // bc you dont know how much of each color to take
    float left = left_upper * (1 - ty) + left_lower * ty;
    float right = right_upper * (1 - ty) + right_lower * ty;

    // Weighted average of all 4 corners
    float result = left * (1 - tx) + right * tx;

    return (unsigned char)(result);
}

void process(BYTE *bigdata, BYTE *smalldata, BYTE* outputdata, 
    int height, int width, int padding, 
    int small_padding, int small_width, int small_height, 
    float ratio, const tagBITMAPINFOHEADER &fih1, const tagBITMAPINFOHEADER &fih2,
    int start, int end)
{
    // Loop over bigger image dimensions
    for(int y = start; y < end; y++) 
    {
        for(int x = 0; x < width; x++) 
        {
            // Grab colors RGB from big image
            float bb = get_color_bilinear(bigdata, x, y, width, height, padding, 0);
            float gb = get_color_bilinear(bigdata, x, y, width, height, padding, 1);
            float rb = get_color_bilinear(bigdata, x, y, width, height, padding, 2);
            
            // Get coords from the smaller image
            // Smaller coord = size of small / size of big
            float xs = (x * (small_width / (float)width));
            float ys = (y * (small_height / (float)height));

            // For PLTL 7 
            //Just use get color, W/O Bilinear Interpolation
            // float rs = get_color(smalldata, xs, ys, small_width, small_height, small_padding,2);
            // float gs = get_color(smalldata, xs, ys, small_width, small_height, small_padding,1);
            // float bs = get_color(smalldata, xs, ys, small_width, small_height, small_padding,0);

            // Get pixel colors from small image scaled fractionally
            //get_color_bilinear function and calculate the floating point coordinates for the smaller image
            float rs = get_color_bilinear(smalldata, xs, ys, small_width, small_height, small_padding, 2);
            float gs = get_color_bilinear(smalldata, xs, ys, small_width, small_height, small_padding, 1);
            float bs = get_color_bilinear(smalldata, xs, ys, small_width, small_height, small_padding, 0);

            // Mix the pixels
            // Explicitly state how much of each image to use. If ratio=0.7, then 70% img1, 30% img2
            float red_result, green_result, blue_result;
            if(fih1.biWidth >= fih2.biWidth)
            {
                // big is image 1, ratio weight about img1
                red_result = (rb / 255.0f)* ratio + (rs / 255.0f )* (1.0f - ratio);
                green_result = (gb / 255.0f) * ratio + (gs / 255.0f) * (1.0f - ratio);
                blue_result = (bb / 255.0f) * ratio + (bs / 255.0f) * (1.0f - ratio);
            }
            else
            {
                // big is image 2, ratio weight about img2
                red_result = (rs / 255.0f)* ratio + (rb / 255.0f )* (1.0f - ratio);
                green_result = (gs / 255.0f) * ratio + (gb / 255.0f) * (1.0f - ratio);
                blue_result = (bs / 255.0f) * ratio + (bb / 255.0f) * (1.0f - ratio);
            }

            // Assign back into target array
            BYTE *pixel_out = outputdata + (y * (width * 3 + padding) + x * 3);
            pixel_out[0] = (unsigned char)(blue_result * 255.0f);
            pixel_out[1] = (unsigned char)(green_result * 255.0f);
            pixel_out[2] = (unsigned char)(red_result * 255.0f);
        }
    }
}
    
int main(int argc, char *argv[]) {
    // Expects: ./[program name] [imagefile1] [imagefile2] [ratio] [process #] [outputfile]
    //             argv[0]          argv[1]   argv[2]      argv[3]   argv[4]     argv[5]

    // Catch wrong/ missing parameters and print a brief manual page
    if (argc != 6)
    {
        std::cerr << "Incorrect or missing parameters" << endl;
        return -1;
    }

    const char *image1 = argv[1];
    const char *image2 = argv[2];
    float ratio = atof(argv[3]);  // Convert string to float
    int processes = stoi(argv[4]);  // Convert string to int
    const char *output = argv[5]; 
    
    FILE *f1 = fopen(image1, "rb");
    // Read file header 
    tagBITMAPFILEHEADER fh1;
    fread(&fh1.bfType, 2, 1, f1);
    fread(&fh1.bfSize, 4, 1, f1);
    fread(&fh1.bfReserved1, 2, 1, f1);
    fread(&fh1.bfReserved2, 2, 1, f1);
    fread(&fh1.bfOffBits, 4, 1, f1);
    // Read info header 
    tagBITMAPINFOHEADER fih1;
    fread(&fih1, sizeof(fih1), 1, f1);

    // Read second image
    FILE *f2 = fopen(image2, "rb");
    tagBITMAPFILEHEADER fh2;
    fread(&fh2.bfType, 2, 1, f2);
    fread(&fh2.bfSize, 4, 1, f2);
    fread(&fh2.bfReserved1, 2, 1, f2);
    fread(&fh2.bfReserved2, 2, 1, f2);
    fread(&fh2.bfOffBits, 4, 1, f2);
    tagBITMAPINFOHEADER fih2;
    fread(&fih2, sizeof(fih2), 1, f2);

    // Read pixel data for both images
    BYTE *img1data = (BYTE*) mmap(NULL, fih1.biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fseek(f1, fh1.bfOffBits, SEEK_SET);
    fread(img1data, fih1.biSizeImage, 1, f1);
    fclose(f1);

    BYTE *img2data = (BYTE*) mmap(NULL, fih2.biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    fseek(f2, fh2.bfOffBits, SEEK_SET);
    fread(img2data, fih2.biSizeImage, 1, f2);
    fclose(f2);

    // Debug: print image info
    cout << "Image 1: " << fih1.biWidth << "x" << fih1.biHeight << " size=" << fih1.biSizeImage << endl;
    cout << "Image 2: " << fih2.biWidth << "x" << fih2.biHeight << " size=" << fih2.biSizeImage << endl;

    // Determine which image is bigger (by WIDTH only)
    // Use bigger image dimensions for output
    tagBITMAPFILEHEADER fh = (fih1.biWidth >= fih2.biWidth) ? fh1 : fh2;
    tagBITMAPINFOHEADER fih = (fih1.biWidth >= fih2.biWidth) ? fih1 : fih2;
    BYTE *bigdata = (fih1.biWidth >= fih2.biWidth) ? img1data : img2data;
    BYTE *smalldata = (fih1.biWidth >= fih2.biWidth) ? img2data : img1data;

    tagBITMAPINFOHEADER fih_small = (fih1.biWidth >= fih2.biWidth) ? fih2 : fih1;
    cout << "Check if actually using the bigger image: " << fih.biWidth << "x" << fih.biHeight << endl;

    // Create output buffer with bigger image size
    BYTE *outputdata = (BYTE*) mmap(NULL, fih.biSizeImage, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // Start with copy of big image, copy n# bytes (fih.biSizeImage) from bigdata to outputdata
    //memcpy(outputdata, bigdata, fih.biSizeImage);  
    // Dont do this, unneccessary write pixel by pixel

    int padding = (4 - (fih.biWidth * 3) % 4) % 4;
    int height = fih.biHeight;
    int width = fih.biWidth;
    int rowsize = width * 3 + padding;

    int small_height = fih_small.biHeight;
    int small_width = fih_small.biWidth;
    int small_padding = (4 - (small_width * 3) % 4) % 4;
    
    auto start = chrono::high_resolution_clock::now();

    // Fork N (1-4) processes, distribute all rows evenly (including remainder)
    int rowsPerProc = height / processes;
    int remainder = height % processes;
    
    for (int i = 0; i < processes; i++) 
    {
        int startRow = i * rowsPerProc + min(i, remainder);
        int endRow;
        if (i < remainder) 
        {
            endRow = startRow + rowsPerProc + 1;
        } 
        else 
        {
            endRow = startRow + rowsPerProc;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            // Child process - compute its assigned rows and exit
            process(bigdata, smalldata, outputdata, 
                height, width, padding, 
                small_padding, small_width, small_height, 
                ratio, fih1, fih2, 
                startRow, endRow);
            _exit(0);
        }
    }

    // Parent waits for all children
    for (int i = 0; i < processes; i++) wait(0);
    
    auto end = chrono::high_resolution_clock::now();
    double elapsed = chrono::duration<double, micro>(end - start).count();
    cout << "Elapsed time: " << elapsed << " microseconds" << endl;

    // Write output file
    FILE *fout = fopen(output, "wb");
    
    // Write file header 
    fwrite(&fh.bfType, 2, 1, fout);
    fwrite(&fh.bfSize, 4, 1, fout);
    fwrite(&fh.bfReserved1, 2, 1, fout);
    fwrite(&fh.bfReserved2, 2, 1, fout);
    fwrite(&fh.bfOffBits, 4, 1, fout);
    // Write info header 
    fwrite(&fih, sizeof(fih), 1, fout);

    // Write pixel data (blended and interpolated output)
    fwrite(outputdata, fih.biSizeImage, 1, fout);
    fclose(fout);

    // Cleanup
    munmap(img1data, fih1.biSizeImage);
    munmap(img2data, fih2.biSizeImage);
    munmap(outputdata, fih.biSizeImage);
    
    return 0;
}

