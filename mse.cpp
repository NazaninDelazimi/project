#include <QPULib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
using namespace std;

// ============================================================================
// Vector version
// ============================================================================



void step(Ptr<Int> input_image, Ptr<Int> input_block,Ptr<Int> MSE , Int BLOCK_SIZE)
{
  input_image = input_image + BLOCK_SIZE*me() + index();
  input_block = input_block +BLOCK_SIZE*me() + index(); 
  Int min = 1000000; 
  

  
   MSE[0] = 0;

      For (Int m = me() , m < BLOCK_SIZE , m = m + numQPUs())
           For (Int n = 0 ,  n< BLOCK_SIZE , n = n + 16 )
                  Int image_result = 0;
                Int block_result = 0;
                gather (input_image);
                gather(input_block);
                receive(image_result);
                receive(block_result);
                MSE = MSE + (image_result - block_result)*(image_result - block_result);
                input_image = input_image +16;
                input_block = input_block + 16;
                 End
              input_image = input_image +BLOCK_SIZE * numQPUs(); 
              input_block = input_block + BLOCK_SIZE * numQPUs();

        End
cout<<"in step function "<<endl;
Int result = 0;

result = MSE[0];
//result = result + rotate(result, 8);
result = result + rotate(result, 8);
result = result + rotate(result, 4);
result = result + rotate(result, 2);
result = result + rotate(result, 1);
MSE[0] = result;  


 }

// ============================================================================
// Main
// ============================================================================

int main()
{
int image[1280][768];
int block[100][100];
int min = 1000000;
int b= 0;
const int NQPUS  = 1;
const int WIDTH  = 1280;
const int NCOLS  = WIDTH;
const int HEIGHT = 768;
const int BLOCK_HEIGHT = 100;
const int BLOCK_WIDTH = 100;
const int NROWS  = HEIGHT;
int Coordinates [100][2];

cout<<"invoke the function main 2"<<endl; 

  // Allocate and initialise input and output maps
  SharedArray<int> MSE(16),input_image(BLOCK_HEIGHT*BLOCK_WIDTH), input_block(BLOCK_HEIGHT*BLOCK_WIDTH);
  cout<<"invoke the function main 3"<<endl; 


for (int i = 0; i < 768; i++) {
    for (int j = 0; j <1280; j++) {
          image[i][j] =rand() % 256;
    }
  }

 for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 100 ; j++) {
      //input_block[i*(100)+j] = rand() % 256;
      block[i][j] = rand() % 256;
      //cout<<block[i][j]; 
 }
    //cout<<"\n";
  }

 cout<<"invoke the function main 4"<<endl; 
for(int i =0 ; i<16 ;i++){
MSE[i] = 0;
}

  // Compile kernel
  auto k = compile(step);
  timeval tvStart, tvEnd, tvDiff;
  // Invoke kernel


  //k.setNumQPUs(NQPUS);
  gettimeofday(&tvStart, NULL);
  cout<<"invoke the function main 5 "<<endl; 
  
for (int i=0 ; i <HEIGHT ; i++){
for (int j= 0 ; j<WIDTH ;j++){
        for (int a=0 ; a< BLOCK_HEIGHT ; a++){
                for (int b = 0; b< BLOCK_WIDTH ; b++){
                  if(i+a > BLOCK_HEIGHT || j+b > BLOCK_WIDTH)
                    {
                       input_image[a*BLOCK_HEIGHT+b] = 0;
                        input_block[a*BLOCK_HEIGHT+b] = 0;
                    }
                    else{ input_image[a*BLOCK_HEIGHT+b] = image[i+a][j+b];
                        input_block[a*BLOCK_HEIGHT+b] = block[i+a][j+b];
                        }
                       
                }
                        }
k.setNumQPUs(1);
k(&input_image,  &input_block ,&MSE,BLOCK_WIDTH);
cout<<"new mse "<<MSE[0]<<endl;
if (min >= MSE[0]) {
                        if (min > MSE[0]) {//new MSE found!
                                b = 0;
                                //add the coordination of MSE
                                Coordinates[b][0] = i;
                                Coordinates[b++][1] = j;
                        }
                        else {// same MSE found! just add the coordination
                                Coordinates[b][0] = i;
                                Coordinates[b++][1] = j;
                        }
                        min = MSE[0]; // set the new minimmum MSE 

}
  }
}
  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);

  // Display results
  printf("P2\n%i %i\n255\n", WIDTH, HEIGHT);
 for (int y = 0; y < 100*16 ; y++){
    for (int x = 0; x < 2; x++) {
        printf("block index :  %d  ",Coordinates[y][x] );
   } cout<<endl;
}

  // Run-time of simulation
  printf("# %ld.%06lds\n", tvDiff.tv_sec, tvDiff.tv_usec);

  return 0;
}
/*

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.609964] Internal error: Oops: 5 [#1] SMP ARM

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610165] Process systemd-cgroups (pid: 1256, stack limit = 0x9ec24210)

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610172] Stack: (0x9ec25d68 to 0x9ec26000)

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610180] 5d60:                   9ec25d84 9ec25d78 80282f70 80282f0c 9ec25d9c 9ec25d88

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610189] 5d80: 80262a68 80282f60 8dd31e00 9ec25e40 9ec25dd4 9ec25da0 80258f7c 80262a24

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610197] 5da0: 00000000 ae7894e0 9ec25e04 af79049c 00000001 00000000 ae782764 00000052

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610206] 5dc0: 9ec25e40 ae7894dc 9ec25e24 9ec25dd8 8021e998 80258ea4 0006955f 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610214] 5de0: a44fb3c0 0000004c 0000004c 00000080 00000001 ae782720 8e48c994 00000043

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610223] 5e00: 00000052 00000040 9ec25e40 769bf000 ad4d737c 00000054 9ec25ea4 9ec25e28

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610232] 5e20: 80259bac 8021e62c 8066f27c 80278284 8066f240 96bf307c 9ec25e5c 9ec25e48

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610240] 5e40: 8dc83a80 00000054 014200ca 00000052 769b9000 8dd49da0 8dd49da0 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610249] 5e60: 00000000 00000000 00000000 a447d6e4 af64e1b0 00000000 9ec25ea4 9ec25fb0

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610257] 5e80: 8dd31e00 80000007 769bfbd0 ad4d7340 ad4d737c 00000054 9ec25efc 9ec25ea8

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610266] 5ea0: 807a458c 802592a8 9ec25ecc 9ec25eb8 802a23c8 80278284 00000003 a885c440

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610274] 5ec0: 9ec25eec 00000000 00000000 00000100 00000000 80c095bc 00000007 807a425c

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610283] 5ee0: 769bfbd0 9ec25fb0 76f93cf0 769e4d78 9ec25fac 9ec25f00 801012a0 807a4268

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610291] 5f00: 802ae18c 802adf30 9ec25f54 9ec25f18 8028c560 802ae164 00000020 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610300] 5f20: 8028c654 8dd32344 8dd32328 8dd31e00 8dd32344 80c98e04 00000000 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610308] 5f40: 9ec25f64 00000000 8dd32328 8dd31e00 8dd32344 80c98e04 9ec25f8c 9ec25f68

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610316] 5f60: 8013be3c 9ec24000 9ec24010 80108204 9ec25fb0 80108204 9ec24000 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610325] 5f80: 9ec25fac 769bfbd0 20000010 769bfbd0 20000010 ffffffff 10c5383d 10c5387d

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610333] 5fa0: 00000000 9ec25fb0 807a4024 80101268 769e501c 76d5c2b4 00052bd0 769bfbd0

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610342] 5fc0: 76f8f720 769e4d78 0000000e 00000015 7ec7aca0 76f93cf0 769e4d78 7ec7ad2c

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610350] 5fe0: 00000000 7ec7ac68 76f74528 769bfbd0 20000010 ffffffff 00000000 00000000

Message from syslogd@raspberrypi at Nov 10 09:51:23 ...
 kernel:[  149.610486] Code: 089da800 e1a0300d e3c33d7f e3c3303f (e592117c) 
^C
