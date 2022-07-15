// libraries
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "CL/opencl.h"


// symbolic variables
#define MEM_SIZE				(1024 * 1024)
#define SUCCESS					0
#define FAILURE 				1
#define SET						1
#define UNSET					0
#define NUM_ENTRIES_DEVICE 		1
#define NUM_DEVICES				1
#define TRUE					1
#define FALSE					0


// global variables
char bitstream_name[128] = "kernel";
char kernel_name[128] = "TestKernel";
char bitstream_path[256] = "./";
char verbose;


// main function
int main(int argc, char** argv)
{
	// variables
	cl_platform_id hw_platform;
	cl_device_id fpga_device;
	cl_context hw_context;
	cl_command_queue cmd_queue;
	size_t max_kernel_size;
	unsigned char* bitstream;
	FILE *fp_bitstream;
	cl_program program;
	cl_kernel hw_kernel;
	cl_mem dev_din;
	cl_mem dev_dout;
	cl_event kernel_event;
	int* h_din;
	int* h_dout;
	int i;


	// body
	verbose = TRUE;
	fprintf(stdout, "\nFPGA Acceleration -- Test Program\n");
	fprintf(stdout, "LINKS Foundation -- Alberto Scionti (2022)\n\n");
	fprintf(stdout, "**************************\n");
	//
	// initialize HW platform variables
	hw_platform = NULL;	
	fpga_device = NULL;
	// searching for an OpenCL platform and device
	if (verbose)
		fprintf(stdout, "[inf] searching for an OpenCL enabled platform\n");
	clGetPlatformIDs(SET, &hw_platform, NULL);
	if (verbose)
		fprintf(stdout, "[inf] searching for enabled devices\n");
	clGetDeviceIDs(hw_platform, CL_DEVICE_TYPE_ALL, NUM_ENTRIES_DEVICE, &fpga_device, NULL);
	// cretae a context
	if (verbose)
		fprintf(stdout, "[inf] creating the context for kernel execution\n");
	hw_context = clCreateContext(NULL, NUM_DEVICES, &fpga_device, NULL, NULL, NULL);
	// create the command queue
	if (verbose)
		fprintf(stdout, "[inf] creating the device command queue\n");
	cmd_queue = clCreateCommandQueue(hw_context, fpga_device, UNSET, NULL);
	// reading the kernel binary
	if (verbose)
		fprintf(stdout, "[inf] allocating memory area for loading the kernel bitstream\n");
	max_kernel_size = 0x10000000;
	bitstream = (unsigned char*) malloc(max_kernel_size);
	if (bitstream == NULL)
	{
		fprintf(stderr, "[err] unable to allocate memory for the kernel bitstream\n");
		fprintf(stderr, "      program aborted\n");
		exit(EXIT_FAILURE);
	}
	if (argc == 2)
	{
		strcpy(bitstream_name, argv[1]);
		fprintf(stdout, "[inf] loading %s.cl\n", bitstream_name);
	}
	else
	{
		fprintf(stdout, "[inf] trying to load default kernel (%s.cl)\n", bitstream_name);
	}
	// opening the bitstream file in binary mode and reading content
	strcat(bitstream_path, bitstream_name);
	strcat(bitstream_path, ".aocx");
	fp_bitstream = fopen(bitstream_path, "rb");
	if (fp_bitstream == NULL)
	{
		fprintf(stderr, "[err] unable to open and load the kernel bitstream (%s)\n", bitstream_path);
		fprintf(stderr, "      program aborted\n");
		exit(EXIT_FAILURE);
	}
	fread(bitstream, max_kernel_size, 1, fp_bitstream);
	fclose(fp_bitstream);
	// create program
	if (verbose)
		fprintf(stdout, "[inf] creating the program structure to embed the kernel\n");
	program = clCreateProgramWithBinary(hw_context, 
										NUM_DEVICES, 
										&fpga_device, 
										&max_kernel_size, 
										(const unsigned char**)&bitstream,
										NULL, 
										NULL);
	// create the kernel
	if (verbose)
		fprintf(stdout, "[inf] create the hw-kernel\n");
	hw_kernel = clCreateKernel(program, kernel_name, NULL);
	// allocating the host data structures for loading input and outdata
	// for communicating with the FPGA board, initialize the data structures
	if (verbose)
		fprintf(stdout, "[inf] allocate input/output memory data structures\n");
	posix_memalign ((void*) &h_din, 64, sizeof(int) * MEM_SIZE);
	posix_memalign ((void*) &h_dout, 64, sizeof(int) * MEM_SIZE);
	if ((h_din == NULL) || (h_dout == NULL))
	{
		fprintf(stderr, "[err] unable to allocate memory data structures\n");
		fprintf(stderr, "      program aborted\n");
		free(h_din);
		free(h_dout);
		free(bitstream);
		exit(EXIT_FAILURE);
	}
	if (verbose)
		fprintf(stdout, "[inf] initializing the memory arrays\n");
	for (i = 0; i < MEM_SIZE; i++)
	{
		h_din[i] = i;
		h_dout[i] = 0;
		if (verbose && (i < 5))
			fprintf(stdout, "       h_din[%d] = %d / h_dout[%d] = %d\n", i, h_din[i], i, h_dout[i]);
	}	
	// creating memory buffers for interacting with the FPGA board (data in/out movement)
	if (verbose)
		fprintf(stdout, "[inf] allocating memory buffers for data movement to/from the FPGA device\n");
	dev_din = clCreateBuffer(hw_context, CL_MEM_READ_ONLY, sizeof(int) * MEM_SIZE, NULL, NULL);
	dev_dout = clCreateBuffer(hw_context, CL_MEM_WRITE_ONLY, sizeof(int) * MEM_SIZE, NULL, NULL);
	// this function creates and enqueues commands to allow copying input data from the host memory (h_din)
	// to the buffer that will be read by the FPGA board => Write/Read operation sees from the host perspective
	if (verbose)
		fprintf(stdout, "[inf] enqueuing commands requireds to write a data buffers\n");
	clEnqueueWriteBuffer(cmd_queue, dev_din, CL_TRUE, 0, sizeof(int) * MEM_SIZE, h_din, 0, NULL, NULL);
	// 
	// LAUNCH THE KERNEL
	if (verbose)
		fprintf(stdout, "[inf] setting the kernel arguments\n");
	clSetKernelArg(hw_kernel, 0, sizeof(cl_mem), &dev_din);
	clSetKernelArg(hw_kernel, 1, sizeof(cl_mem), &dev_dout);
	// enqueuing commands to write on the device memeory the kernel code and execute it
	// there is an event associated to that related to the completion of the task
	fprintf(stdout, "[inf] launching kernel execution...\n");
	clEnqueueTask(cmd_queue, hw_kernel, 0, NULL, &kernel_event);
	// waiting the kernel event that define the end of the execution
	// the first argument is the number of events to wait for
	clWaitForEvents(1, &kernel_event);
	// relaease the event object
	clReleaseEvent(kernel_event);
	// reading back data from the device memory (buffer dev_dout) to the host memory array (h_dout)
	// to this end, the code enqueue the commands that allows to perform this copy
	if (verbose)
		fprintf(stdout, "[inf] reading back data output (copying data from dev_dout to h_dout)\n");
	clEnqueueReadBuffer(cmd_queue, dev_dout, CL_TRUE, 0, sizeof(int) * MEM_SIZE, h_dout, 0, NULL, NULL); 	
	fprintf(stdout, "[inf] done\n");
	fprintf(stdout, "[inf] reading back data:\n");
	for (i = 0; i < 5; i++)
	{
		fprintf(stdout, "       h_din[%d] = %d / h_dout[%d] = %d\n", i, h_din[i], i, h_dout[i]);
	}
	// flushing the command queue before realeasing it	
	clFlush(cmd_queue);
	clFinish(cmd_queue);
	//
	// cleanup environment
	if (verbose)
		fprintf(stdout, "[inf] clean up all allocated memory structures\n");
	clReleaseMemObject(dev_din);
	clReleaseMemObject(dev_dout);
	clReleaseKernel(hw_kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmd_queue);
	clReleaseContext(hw_context);
	free(bitstream);
	free(h_din);
	free(h_dout);
	// complete the execution
	fprintf(stdout, "**************************\n");
	fprintf(stdout, "\nTest Program completed\n\n");
	return (SUCCESS);
}