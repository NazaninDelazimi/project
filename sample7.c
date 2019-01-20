#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CL/opencl.h>


#define SUCCESS 0
#define FAILURE -1
#define N 10000
#define BLOCK_SIZE  10000
#define IMAGE_SIZE 983040
#define BL_SIZE 100
#define IM_WIDTH 1280

// OpenCL kernel. Each work item takes care of one element of c
const char *kernelSource =                                       "\n" \
"#pragma OPENCL EXTENSION cl_khr_fp64 : enable                    \n" \
"__kernel void vecAdd(  __global int *a,                       \n" \
"                       __global int *b,                       \n" \
"                       __global int *c,                       \n" \
"                        unsigned int n, unsigned int k,                    \n" \
"                        unsigned int i, unsigned int j,                    \n" \
"                        unsigned int bl_width, unsigned int im_width){      \n" \
"  a = a +(i+k)*im_width + j;                                                         \n" \
"  b = b +(k*bl_width); 								\n"
"  c = c + (k*bl_width);                                                          \n" \
"    //a= a+ i*n ; b = b+ i*n; c = c+ i*n;//Get our global thread ID                                  \n" \
"    int id = get_global_id(0);                                  \n" \
"                                                                \n" \
"    //Make sure we do not go out of bounds                      \n" \
"    if (id < n){                                                 \n" \
"        if (a[id] > b[id]) {c[id] = (a[id] - b[id])*(a[id]-b[id]);}                                  \n" \
"        else { c[id] = (b[id] - a[id] )*(b[id] - a[id]);         }                    \n" \
"     }                                                           \n" \
"      }                                                          \n" \

                                                                "\n" ;

int main( int argc, char* argv[] )
{

    // Length of vectors
    
 unsigned int n = N;

    // Device input buffers
    cl_mem d_a;
    cl_mem d_b;
    // Device output buffer
    cl_mem d_c;
    cl_event  event;
    cl_platform_id cpPlatform;        // OpenCL platform
    cl_device_id device_id;           // device ID
    cl_context context;               // context
    cl_command_queue queue;           // command queue
    cl_program program;               // program
    cl_kernel kernel;                 // kernel
    long long start, end;
    // Size, in bytes, of each vector

    size_t im_bytes = IMAGE_SIZE*sizeof(int);
    size_t bl_bytes = BLOCK_SIZE *sizeof(int);

// Allocate memory for each vector on host
    int h_a[IMAGE_SIZE*sizeof(int)];
    int h_b[BLOCK_SIZE*sizeof(int)];
    int h_c[BLOCK_SIZE*sizeof(int)];
    int Coordinates[100][2];
printf("define h_* arrays \n");

    // Initialize vectors on host
    unsigned int s;
    for( s = 0; s < IMAGE_SIZE; s++ )
    {
        h_a[s] = 255;
    }

 for( s = 0; s < BLOCK_SIZE; s++ )
    {
        h_b[s] = 1;
    }
 
    size_t globalSize, localSize;
    cl_int err;

    // Number of work items in each local work group
    localSize = 4;

    // Number of total work items - localSize must be devisor
    globalSize = ceil(n/(int)localSize)*localSize;

    // Bind to platform
    err = clGetPlatformIDs(1, &cpPlatform, NULL);
    if (err != CL_SUCCESS) {
     printf("Error: Getting platforms!\r\n");
        return FAILURE ;
    }

 char buffer[10240];
 printf("  -- %d --\n", 1);
 clGetPlatformInfo(cpPlatform, CL_PLATFORM_PROFILE, 10240, buffer, NULL);
 printf("  PROFILE = %s\n", buffer);
 clGetPlatformInfo(cpPlatform, CL_PLATFORM_VERSION, 10240, buffer, NULL);
 printf("  VERSION = %s\n", buffer);
 clGetPlatformInfo(cpPlatform, CL_PLATFORM_NAME, 10240, buffer, NULL);
 printf("  NAME = %s\n", buffer);
 clGetPlatformInfo(cpPlatform, CL_PLATFORM_VENDOR, 10240, buffer, NULL);
 printf("  VENDOR = %s\n", buffer);
 clGetPlatformInfo(cpPlatform, CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL);
 printf("  EXTENSIONS = %s\n", buffer);
 printf("\n\n");

    // Get ID for the device
    err = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
     printf("Error: Getting devices!\r\n");
        return FAILURE ;
    }
    //==============================================================================

    cl_uint uNum;
    size_t  size;
    size_t workitem_size[3];
    cl_ulong lNum;

 clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(buffer), &buffer, NULL);
 //fprintf(logfp, "CL_DEVICE_NAME = %s\n", buffer);
 printf( "  CL_DEVICE_NAME = %s\n", buffer);

 clGetDeviceInfo(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &uNum, NULL);
 printf( "  CL_DEVICE_MAX_CLOCK_FREQUENCY = %d MHz\n", uNum);

 clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &uNum, NULL);
 printf( "  CL_DEVICE_MAX_COMPUTE_UNITS = %d\n", uNum);

 clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &size, NULL);
 printf( "  CL_DEVICE_MAX_WORK_GROUP_SIZE = %d\n", size);

 clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t), &size, NULL);
 printf( "  CL_DEVICE_MAX_WORK_ITEM_SIZES = %d\n", size);

 clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, sizeof(cl_ulong), &lNum, NULL);
 printf("  CL_DEVICE_GLOBAL_MEM_CACHE_SIZE = %llu bytes\n", lNum);

 clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(workitem_size), &workitem_size, NULL);
 printf("  CL_DEVICE_MAX_WORK_ITEM_SIZES:\t%u / %u / %u \n", workitem_size[0], workitem_size[1], workitem_size[2]);

 clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &lNum, NULL);
 printf("  CL_DEVICE_LOCAL_MEM_SIZE = %llu bytes\n", lNum);


 printf("\n\n");

     //==============================================================================


    // Create a context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);

    // Create a command queue
    queue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);

    // Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1,
                            (const char **) & kernelSource, NULL, &err);

    // Build the program executable
    clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    // Create the compute kernel in the program we wish to run
    kernel = clCreateKernel(program, "vecAdd", &err);

    // Create the input and output arrays in device memory for our calculation
    d_a = clCreateBuffer(context, CL_MEM_READ_ONLY, im_bytes, NULL, NULL);
    d_b = clCreateBuffer(context, CL_MEM_READ_ONLY, bl_bytes, NULL, NULL);
    d_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, bl_bytes, NULL, NULL);

    // Write our data set into the input array in device memory
    err = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, 0,
                                   im_bytes, h_a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue, d_b, CL_TRUE, 0,
                                   bl_bytes, h_b, 0, NULL, NULL);


unsigned int i,j,k,b,number,bl_width,im_width,min,sum;
bl_width = 100;
im_width = 1280;
number =100;
min =100000000 ;
sum =0 ;
for(i = 0 ; i < IMAGE_SIZE/im_width ; i++){
	for (j = 0 ; j < (im_width) ; j++){
		for(k= 0 ; k < 100 ; k++){
    // Set the arguments to our compute kernel
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), &number);
    err |= clSetKernelArg(kernel, 4, sizeof(unsigned int), &k);
    err |= clSetKernelArg(kernel, 5, sizeof(unsigned int), &i);
    err |= clSetKernelArg(kernel, 6, sizeof(unsigned int), &j);
    err |= clSetKernelArg(kernel, 7, sizeof(unsigned int), &bl_width);
    err |= clSetKernelArg(kernel, 8, sizeof(unsigned int), &im_width);

    // Execute the kernel over the entire range of the data set
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, &event);
   // Read the results from the device
    clEnqueueReadBuffer(queue, d_c, CL_TRUE, 0,
                                bl_bytes, h_c, 0, NULL, NULL );

    //Sum up vector c and print result divided by n, this should equal 1 within error
sum =0;
for (b = 0 ; b < 10000 ; b++){
sum = sum + h_c[b];}
    printf("%d\t",sum);//h_c[k*100]);

}

sum /= BLOCK_SIZE;

			if (min >= sum) {
				if (min > sum) {//new MSE found!					
					b = 0;
					//add the coordination of MSE
					Coordinates[b][0] = i;
					Coordinates[b++][1] = j;
				}
				else {// same MSE found! just add the coordination
					Coordinates[b][0] = i;
					Coordinates[b++][1] = j;
				}
				min = sum; // set the new minimmum MSE 
			}

}}



    // Wait for the command queue to get serviced before reading back results
    clFinish(queue);

 err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
 if (err != CL_SUCCESS) { 
  printf("Error start time. Error Code=%d\n",err);
  exit(1);
 }

 err = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
 if (err != CL_SUCCESS) { 
  printf("Error start time. Error Code=%d\n",err);
  exit(1);
 }

 double total = (double)(end-start)/1e6;
 printf(" time = %e msec\n", total);

    // release OpenCL resources
    clReleaseMemObject(d_a);
    clReleaseMemObject(d_b);
    clReleaseMemObject(d_c);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);



    return 0;
}


