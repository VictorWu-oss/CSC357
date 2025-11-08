#include <stdio.h>
#include <fstream>

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char BYTE;
typedef struct tagBITMAPFILEHEADER
{
WORD bfType; //specifies the file type
DWORD bfSize; //specifies the size in bytes of the bitmap file
WORD bfReserved1; //reserved; must be 0
WORD bfReserved2; //reserved; must be 0
DWORD bfOffBits; //specifies the offset in bytes from the bitmapfileheader to the bitmap
//bits
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
DWORD biSize; //specifies the number of bytes required by the struct
LONG biWidth; //specifies width in pixels
LONG biHeight; //specifies height in pixels
WORD biPlanes; //specifies the number of color planes, must be 1
WORD biBitCount; //specifies the number of bits per pixel
DWORD biCompression; //specifies the type of compression
DWORD biSizeImage; //size of image in bytes
LONG biXPelsPerMeter; //number of pixels per meter in x axis
LONG biYPelsPerMeter; //number of pixels per meter in y axis
DWORD biClrUsed; //number of colors used by the bitmap
DWORD biClrImportant; //number of colors that are important
}BITMAPINFOHEADER;

int main(){
    tagBITMAPFILEHEADER* fh = new tagBITMAPFILEHEADER;
    tagBITMAPINFOHEADER* fi = new tagBITMAPINFOHEADER;

    //read file
    FILE* infile = fopen("PLTL2.bmp", "rb");
    fread(&fh, 2, 1, infile);
    fread(&fh->bfType, 2, 1, infile);
    fread(&fh->bfSize, 4, 1, infile);
    fread(&fh->bfReserved1, 2, 1, infile);
    fread(&fh->bfReserved2, 2, 1, infile);
    fread(&fh->bfOffBits, 4, 1, infile);
    fread(&fi, sizeof(fi), 1, infile);

    fseek(infile, fh->bfOffBits, SEEK_SET);
    BYTE data[fi->biSizeImage];
    fread(data, fi->biSizeImage, 1, infile);
    BYTE target[fi->biSizeImage];
    LONG padding = (4 - (fi->biWidth % 4)) % 4;
    LONG rw = fi->biWidth * 3 + padding;

    //goes through each x and y for image
    for (LONG x = 0; x < fi->biWidth; x++){
        for (LONG y=0; y < fi->biHeight; y++){

            //get color at each position
            LONG pos = x * 3 + y * rw;
            BYTE b = data[pos];
            BYTE g = data[pos + 1];
            BYTE r = data[pos + 2];

            LONG opos = (fi->biWidth - x - 1) * 3 + y * rw;

            target[opos] = b;
         
            target[opos + 1] = g;
       
            target[opos + 2] = r;
         

        }
    }

    fclose(infile);
    FILE* outfile = fopen("PLTL2Output.bmp", "wb");
    fwrite(&fh, sizeof(fh), 1, outfile);
    fwrite(&fi, sizeof(fi), 1, outfile);
    fwrite(data, sizeof(data), 1, outfile);
    fclose(outfile);

    return 0;
}
