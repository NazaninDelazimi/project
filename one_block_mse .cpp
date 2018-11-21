#include <stdlib.h>
#include "QPULib.h"
#include <iostream>
using namespace std;

void multi(Ptr<Int> input_image, Ptr<Int> input_block,Ptr<Int> MSE_out,Ptr<Int> diff1 , Ptr<Int> diff2,Int BLOCK_SIZE )
{ 
  input_image = input_image + BLOCK_SIZE*me() + index();
  input_block = input_block +BLOCK_SIZE*me() + index(); 

   Int MSE1=0;
   
Int image_result,block_result;
Int difrence = 0;


        For (Int m = me() , m < BLOCK_SIZE , m = m + numQPUs())
           For (Int n = 0 ,  n< BLOCK_SIZE , n = n + 16 )
                image_result = 0;
                block_result = 0;

                gather (input_image);
                gather(input_block);

                input_image = input_image +16;
                input_block = input_block + 16;


                receive(image_result);
                receive(block_result);

                Where(image_result > block_result )
                difrence =image_result - block_result ;
                End

                Where(block_result >= image_result )
                difrence = block_result -image_result ; 
                End

              store(image_result ,diff1);
               diff1= diff1+16;
                MSE1 = MSE1 +  difrence*difrence;

		store(block_result, diff2);
		diff2 = diff2+16;
                

                 End
              input_image = input_image +BLOCK_SIZE * numQPUs(); 
              input_block = input_block + BLOCK_SIZE * numQPUs();

        End
cout<<"in step function "<<endl;
//store(MSE1 ,diff);
//                diff= diff+16;

Int result = 0;
result = (MSE1);
result = result + rotate(result, 8);
result = result + rotate(result, 4);
result = result + rotate(result, 2);
result = result + rotate(result, 1);
store (result,MSE_out);



 }


int main()
{
  // Construct kernel
  int size = 96;
  auto k = compile(multi);

  // Allocate and initialise arrays shared between ARM and GPU
  SharedArray<int> a(size*size), b(size*size), r(16),diff1(size*size),diff2(size*size);
  srand(0);
  for (int i = 0; i < size*size; i++) {
    a[i] = rand()%256;
    b[i] = rand()%256;
    diff1[i]=0;
    diff2[i] =0;
  }

for (int i = 0 ; i<16 ;i++){
r[i] = 0;
}

k.setNumQPUs(1);
  // Invoke the kernel and display the result
  k(&a, &b, &r,&diff1,&diff2,100);
 
cout << "image_result : "<<endl;
 for (int i = 0; i < size*size; i++)
{      if (i%16 ==0 )cout<<endl;
         	cout<<diff1[i]<<'\t';
 } 
cout << "block_result : "<<endl;
  for (int i = 0; i < size*size; i++)
{      if (i%16 ==0 )cout<<endl;
                cout<<diff2[i]<<'\t';
 } 



  return 0;
}
