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
    
    sleep(2);

    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER fih;

    int fd = shm_open("sharing", O_RDWR,0777); //share memory
    int fd2 = shm_open("height", O_RDWR ,0777);
    int fd3 = shm_open("width", O_RDWR,0777);
    int fd0 = shm_open("flag", O_RDWR,0777); 
    int fd4 = shm_open("size",  O_RDWR,0777); 

    int *h = (int*)mmap(NULL,sizeof(fih.biHeight), PROT_READ|PROT_WRITE,MAP_SHARED,fd2,0);
    int *w = (int*)mmap(NULL,sizeof(fih.biWidth), PROT_READ|PROT_WRITE,MAP_SHARED,fd3,0);
    int *flag = (int*)mmap(NULL,1020, PROT_READ|PROT_WRITE,MAP_SHARED,fd0,0);
    int *size = (int*)mmap(NULL,sizeof(fih.biSizeImage), PROT_READ|PROT_WRITE,MAP_SHARED,fd4,0);
    BYTE *a = (BYTE*)mmap(NULL,*size, PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

    int padding = (4 - (*w * 3) % 4) % 4;

    for(int he = 0; he < *h; he++) {
        for(int wi = *w/2; wi < *w; wi++) {
            int idx = he * (*w * 3 + padding) + wi * 3;
            a[idx ] =0 ;
            a[idx + 1] = a[idx + 1];
            a[idx + 2] = a[idx + 2];

        }   
    }    
    
    *flag = 1;  

    return 0;
}