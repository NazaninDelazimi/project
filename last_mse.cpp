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




void step(Ptr<Int> input_image, Ptr<Int> input_block,Ptr<Float> MSE_out,Ptr<Int> diff, Int BLOCK_SIZE , Int I , Int J , Int WIDTH)
{  
  input_image = input_image + (I *WIDTH +J);
  input_image = input_image + BLOCK_SIZE*me() + index();
  input_block = input_block +BLOCK_SIZE*me() + index(); 

   Int MSE1=0;
   Int image_result,block_result;
   Int diference = 0;
Ptr<Int> p = input_image;
Ptr<Int> q = input_block;


	For (Int m = me() , m < BLOCK_SIZE , m = m + numQPUs())
           For (Int n = 0 ,  n< BLOCK_SIZE , n = n + 16 )
                image_result = 0;
                block_result = 0;

	        gather (p);
                gather(q);

		p = p +16;
                q = q + 16;

		
                receive(image_result);
                receive(block_result);

		store(image_result,diff);
		diff = diff+16;
//		store( block_result,diff);
//		diff = diff+16;

		Where(image_result > block_result )
		diference =image_result - block_result ;
		End

		Where(block_result >= image_result )
		diference = block_result -image_result ; 
		End

                MSE1 = MSE1 +  diference*diference;

                 End
              input_image = input_image +BLOCK_SIZE * numQPUs(); 
              input_block = input_block + BLOCK_SIZE * numQPUs();
	      p = input_image;
	      q= input_block;
        End
cout<<"in step function "<<endl;
//store(MSE1 ,diff);
//                diff= diff+16;

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
const int NQPUS  = 1;
const int WIDTH  = 1280;
const int HEIGHT = 768;
const int BLOCK_HEIGHT =96;
const int BLOCK_WIDTH =96;
double result_mse=0;
double  function_out = 0;
int Coordinates [100][2];

cout<<"invoke the function main 2"<<endl; 

  // Allocate and initialise input and output maps
  SharedArray<int> input_image(HEIGHT*WIDTH), input_block(BLOCK_HEIGHT*BLOCK_WIDTH);
  SharedArray<float> MSE_out(32);
  SharedArray<int> diff(BLOCK_HEIGHT*BLOCK_WIDTH); 
  cout<<"invoke the function main 3"<<endl; 


 ifstream in("image2.txt",ios::in);

for (int i = 0; i <HEIGHT  ; i++) {
	for(int j = 0 ; j< WIDTH ; j++){
          in >> input_image[i*WIDTH+j] ;
//	cout<<input_image[i*WIDTH+ j]<<"  ,";
 }//cout<<endl;
}
in.close();

ifstream in2("block.txt",ios::in);

 for (int i = 0; i < BLOCK_HEIGHT  ; i++) {
for(int j = 0; j <BLOCK_WIDTH ; j++)
      in2 >> input_block[i*BLOCK_WIDTH + j] ;
      diff[i]=0;
  }
in2.close();

for(int i =0 ; i<100 ; i++){
Coordinates[i][0] = 0;
Coordinates[i][1] = 0;
}

 cout<<"invoke the function main 4"<<endl; 
for(int i =0 ; i<32 ;i++){
MSE_out[i] = 0.0;

}

  // Compile kernel
  auto k = compile(step);
  timeval tvStart, tvEnd, tvDiff;
  // Invoke kernel


  k.setNumQPUs(1);
  gettimeofday(&tvStart, NULL);
  cout<<"invoke the function main 5 "<<endl; 
  
for (int i=0 ; i+BLOCK_HEIGHT <= HEIGHT ; i++){
for (int j= 0 ; j + BLOCK_WIDTH <= WIDTH ;j++){

//cout<<i<<" , ";
k(&input_image,  &input_block ,&MSE_out,&diff,BLOCK_WIDTH , i ,j ,WIDTH);
/*
for(int a = 0 ; a< 96*96 ; a++){
cout<<diff[a]<<", ";
}
*/
function_out = (double)MSE_out[0];
result_mse = function_out * 0.0001;

cout<<"resulr_mse :"<<result_mse<<endl;

if (min >= result_mse && result_mse > 0 ) {
//cout<<"result_mse : "<<min<<endl;
                        if (min > result_mse) {//new MSE found!
                                b = 0;
                                //add the coordination of MSE
                                Coordinates[b][0] = i;
                                Coordinates[b++][1] = j;
                        }
                        else {// same MSE found! just add the coordination
                                Coordinates[b][0] = i;
                                Coordinates[b++][1] = j;
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
cout<<"min :"<< min;
  return 0;
}

