#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

#include <time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h> 

#include <fcntl.h>
#include <string.h>

using namespace std; 

typedef unsigned char BYTE; //match BMP layout  
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;

typedef struct {
    WORD bfType; 
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
}BITMAPFILEHEADER ;

typedef struct {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;



int main(int argc, char *argv[]) {
    

    const char *inputFile = "input.bmp";
    const char *outputFile = "output.bmp";
    
    FILE *f = fopen(inputFile, "rb");
    
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER fih;

    fread(&fh.bfType, 2, 1, f);
    fread(&fh.bfSize, 4, 1, f);
    fread(&fh.bfReserved1, 2, 1, f);
    fread(&fh.bfReserved2, 2, 1, f);
    fread(&fh.bfOffBits, 4, 1, f);
    fread(&fih, sizeof(BITMAPINFOHEADER), 1, f);

    BYTE* inpPixData = (BYTE*)malloc(fih.biSizeImage);
    fread(inpPixData, fih.biSizeImage, 1, f); //make exact copy
    fclose(f);//close

    int padding = (4 - (fih.biWidth * 3) % 4) % 4;

    

    int fd = shm_open("sharing", O_RDWR|O_CREAT,0777); //share memory
    int fd2 = shm_open("height", O_RDWR |O_CREAT,0777);
    int fd3 = shm_open("width", O_RDWR|O_CREAT,0777);
    int fd0 = shm_open("flag", O_RDWR|O_CREAT,0777); 
    int fd4 = shm_open("size",  O_RDWR|O_CREAT,0777); 



    ftruncate(fd, fih.biSizeImage);
    ftruncate(fd2, sizeof(fih.biHeight));
    ftruncate(fd3, sizeof(fih.biWidth));
    ftruncate(fd4, sizeof(fih.biSizeImage));
    ftruncate(fd0, 1020);


    BYTE *a = (BYTE*)mmap(NULL,fih.biSizeImage, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    int *h = (int*)mmap(NULL,sizeof(fih.biHeight), PROT_READ|PROT_WRITE,MAP_SHARED,fd2,0);
    int *w = (int*)mmap(NULL,sizeof(fih.biWidth), PROT_READ|PROT_WRITE,MAP_SHARED,fd3,0);
    int *flag = (int*)mmap(NULL,1020, PROT_READ|PROT_WRITE,MAP_SHARED,fd0,0);
    int *size = (int*)mmap(NULL,sizeof(fih.biSizeImage), PROT_READ|PROT_WRITE,MAP_SHARED,fd4,0);
    
    *flag = 0; 
    memcpy(a, inpPixData, fih.biSizeImage);
    *h = fih.biHeight;
    *w = fih.biWidth;
    *size = fih.biSizeImage;

    
    for(int h = 0; h < fih.biHeight; h++) {
        for(int w = 0; w < fih.biWidth/2; w++) {
            int idx = h * (fih.biWidth * 3 + padding) + w * 3;
            *(a + idx) = inpPixData[idx] ;
            *(a + idx + 1) = inpPixData[idx + 1];
            *(a + idx + 2) = 0;

        }   
    }

    while (*flag == 0){
    }

    //write new image 
    FILE *fOut = fopen(outputFile, "wb");
    
    //(ptr to data, size of each element, number of each element, outfile pter)
    fwrite(&fh.bfType, 2, 1, fOut);
    fwrite(&fh.bfSize, 4, 1, fOut);
    fwrite(&fh.bfReserved1, 2, 1, fOut);
    fwrite(&fh.bfReserved2, 2, 1, fOut);
    fwrite(&fh.bfOffBits, 4, 1, fOut);
    fwrite(&fih, sizeof(BITMAPINFOHEADER), 1, fOut);
    fwrite(a, fih.biSizeImage, 1, fOut);

    fclose(fOut);
    free(inpPixData);
    close(fd);
    close(fd4);
    close(fd2);
    close(fd3);
    close(fd0);
    shm_unlink("sharing");
    shm_unlink("height");
    shm_unlink("width");
    shm_unlink("flag");
    shm_unlink("size");
    munmap(a, fih.biSizeImage);
    munmap(w, sizeof(fih.biHeight));
    munmap(h, sizeof(fih.biWidth));
    munmap(flag,1020);
    munmap(a,sizeof(fih.biSizeImage));

    return 0;
}