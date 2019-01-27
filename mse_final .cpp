#include <QPULib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
using namespace std;
#include <fstream>
#include <vector>

// ============================================================================
// Vector version
// ============================================================================




void step(Ptr<Int> input_image, Ptr<Int> input_block,Ptr<Float> MSE_out, Int BLOCK_HEIGHT ,Int BLOCK_WIDTH , Int I , Int J , Int WIDTH)
{  
  input_image = input_image + (I *WIDTH +J);
  input_image = input_image + WIDTH       *me() + index();
  input_block = input_block + BLOCK_WIDTH *me() + index(); 
  MSE_out = MSE_out + 16*me() +index();
   Int MSE1,temp=0;
   Int image_result,block_result;
   Int diference = 0;

Ptr<Int> p = input_image;
Ptr<Int> q = input_block;


	For (Int m = me() , m < BLOCK_HEIGHT , m = m + numQPUs())
           For (Int n = 0 ,  n< BLOCK_WIDTH , n = n + 16 )
		temp=0;
		image_result = 0;
                block_result = 0;

	        gather (p);
                gather(q);

		p = p +16;
                q = q + 16;


                receive(image_result);
                receive(block_result);


		Where(image_result > block_result )
		diference =image_result - block_result ;
		End

		Where(block_result >= image_result )
		diference = block_result -image_result ; 
		End

		Where( n < 96)
                   temp =  diference*diference;
		End

		Where(n >= 96)
			Where(index() < 4 )
				temp = diference*diference;
			End
		End
		MSE1 = MSE1+temp;
                 End
              input_image = input_image +WIDTH * numQPUs(); 
              input_block = input_block + BLOCK_WIDTH * numQPUs();
	      p = input_image;
	      q= input_block;
        End
cout<<"in step function "<<endl;


Float result = 0;
result = toFloat(MSE1);
result = result + rotate(result, 8);
result = result + rotate(result, 4);
result = result + rotate(result, 2);
result = result + rotate(result, 1);
store (result,MSE_out);



 }

// ============================================================================
// Main
// ============================================================================

int main()
{


double min = 1000000.0;
int b= 0;
const int NQPUS  = 10;
const int WIDTH  = 768;
const int HEIGHT = 1280;
const int BLOCK_HEIGHT = 100;
const int BLOCK_WIDTH = 100;
double result_mse=0;
double  function_out = 0;
int Coordinates [100][2];


  // Allocate and initialise input and output maps
  SharedArray<int> input_image( HEIGHT*WIDTH ), input_block( BLOCK_HEIGHT*BLOCK_WIDTH );
  SharedArray<float> MSE_out( 16*NQPUS );



 ifstream in("image2.txt",ios::in);

for (int i = 0; i <HEIGHT  ; i++) {
	for(int j = 0 ; j< WIDTH ; j++){
          in >> input_image[i*WIDTH+j] ;
 }}
in.close();

ifstream in2("block.txt",ios::in);

 for (int i = 0; i < BLOCK_HEIGHT  ; i++) {
	for(int j = 0; j <BLOCK_WIDTH ; j++)
      	in2 >> input_block[i*BLOCK_WIDTH + j] ;
   }
in2.close();

for(int i =0 ; i<100 ; i++){
Coordinates[i][0] = 0;
Coordinates[i][1] = 0;
}

 for(int i =0 ; i<32 ;i++){
MSE_out[i] = 0.0;

}

 
  auto k = compile(step);
  
timeval tvStart, tvEnd, tvDiff;
  // Invoke kernel


  k.setNumQPUs(NQPUS);
  gettimeofday(&tvStart, NULL);
  cout<<"invoke the function main 5 "<<endl; 
  
for (int i=0 ; i+BLOCK_HEIGHT <= HEIGHT ; i++){
for (int j= 0 ; j + BLOCK_WIDTH <= WIDTH ;j++){

//	cout<<i<<" , ";

	k(&input_image,  &input_block ,&MSE_out,BLOCK_HEIGHT ,BLOCK_WIDTH , i ,j ,WIDTH);

	function_out =0;

	for(int q=0 ; q < NQPUS ; q++ ){
 		function_out += (double)MSE_out[q*16];
	}
//cout<<function_out<<endl;
	result_mse = function_out /(BLOCK_WIDTH*BLOCK_HEIGHT);



if (min >= result_mse && result_mse > 0 ) {

                        if (min > result_mse) {//new MSE found!
                                b = 0;
                                //add the coordination of MSE
                                Coordinates[b][0] = j;
                                Coordinates[b++][1] = i;
                        }
                        else {// same MSE found! just add the coordination
                                Coordinates[b][0] = j;
                                Coordinates[b++][1] = i;
                        }
                        min = result_mse; // set the new minimmum MSE 

}
  }
}
  gettimeofday(&tvEnd, NULL);
  timersub(&tvEnd, &tvStart, &tvDiff);

  // Display results
  printf("P2\n%i %i\n255\n", WIDTH, HEIGHT);

 for (int y = 0; y < 100 ; y++){
    for (int x = 0; x < 2; x++) {
	if(Coordinates[y][x] !=0)
        printf("block index :  %d  \n",Coordinates[y][x] );
   } //cout<<endl;
}

  // Run-time of simulation
  printf("# %ld.%06lds\n", tvDiff.tv_sec, tvDiff.tv_usec);
  cout<<"min :"<< min<<endl;
  return 0;
}

