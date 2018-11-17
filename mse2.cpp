
#include <QPULib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
using namespace std;


// ============================================================================
// Vector version
// ============================================================================

struct Cursor {
  Ptr<Int> addr;
  Int values[5];

void gather_all(Ptr<Int> p){
  for (int i = 0 ; i< 5 ; i++){
    gather(p);
    p = p + 16;
    receive(values[i]);
  }


};

void step(Ptr<Int> input_image, Ptr<Int> input_block,Ptr<Int> Coordinates, Int WIDTH, Int HEIGHT, Int BLOCK_SIZE)
{
  Cursor image_row[5];
  Cursor block_row[5];
  input_image = input_image + WIDTH*me() + index();
  input_block = input_block +BLOCK_SIZE*me() + index(); 
  Int min = 1000000; 
  Int b = 0;
  Int image_result = 0;
  
  For (Int y = me(), y < HEIGHT, y=y+numQPUs())
   
    for (int i = 0; i < 5 ; i++) 
	block_row[i].gather_all(input_image + i*WIDTH);
    
Int MSE = 0;
      For (Int x = 0, x < WIDTH, x=x+16)
            for (int i = 0; i < 5; i++){
              for (int j = 0 ; j< 5;j++){
               
                Ptr<Int> addr = (input_image + (y+i) * HEIGHT + x+j);
                image_result = 0;
                gather (addr);
                receive(image_result);
                MSE = MSE + (image_result - block_row[i].values[j]) * (image_result - block_row[i].values[j]); 

              }
            } 

      Where((min > MSE)) 
          b = 0;
          Ptr<Int> q = Coordinates ;
          store ( y,q);
          b  = b + 1;  
          q=q+100;
          store ( x,q);

      End
      
      Where(MSE == min)
         Ptr<Int> q = Coordinates + b ;
          store ( y,q);
          q=q+100;  
          store ( x,q);
          b= b+1;
          End

    min = MSE;

    End

      

    // Move to the next input rows
    input_image = input_image + HEIGHT*numQPUs();

  End
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  
int image[600][800];
int block[5][5];
float MSE = 0;
float min = 1000000;
const int NQPUS  = 1;
const int WIDTH  = 800;
const int NCOLS  = WIDTH;
const int HEIGHT = 600;
const int BLOCK_HEIGHT = 5;
const int BLOCK_WIDTH = 5;
const int NROWS  = HEIGHT;


  // Allocate and initialise input and output maps
  SharedArray<int> input_image(HEIGHT*WIDTH), input_block(BLOCK_HEIGHT*BLOCK_WIDTH);
  SharedArray<int> Coordinates(BLOCK_HEIGHT*2);
 
for (int i = 0; i < sizeof image / sizeof image[0]; i++) {
		for (int j = 0; j < sizeof image[0] / sizeof(int); j++) {
			input_image[i*(sizeof image /sizeof input_image[0])+j] =  rand() % 256;
      image[i][j] =rand() % 256;
    }
	}

 for (int i = 0; i < sizeof block / sizeof block[0]; i++) {
		for (int j = 0; j < sizeof block[0] / sizeof(int); j++) {
			input_block[i*sizeof block / sizeof block[0]] = rand() % 256;
			cout<<block[i][j]<<" ";
      block[i][j] = rand() % 256;
		}
		cout<<"\n";
	}

 

  // Compile kernel
  auto k = compile(step);

  // Invoke kernel
  k.setNumQPUs(NQPUS);
  gettimeofday(&tvStart, NULL);
 
  k(&input_image,  &input_block ,&Coordinates, WIDTH, HEIGHT,BLOCK_HEIGHT);
  
  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);

  // Display results
 printf("P2\n%i %i\n255\n", WIDTH, HEIGHT);
  



 // cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );
  //cv::imshow( "Display window", image );

  // Run-time of simulation
  printf("# %ld.%06lds\n", tvDiff.tv_sec, tvDiff.tv_usec);

  return 0;
}
