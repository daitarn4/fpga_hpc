// libraries
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "CL/opencl.h"


// symbolic variables
#define SIZE	(1024 * 1024)
#define SUCCESS	0
#define FAILURE 1
#define SET	1
#define UNSET	0


// global variables
char bitstream_name[128] = "kernel";


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

	// body
	fprintf(stdout, "\nFPGA Acceleration -- Test Program\n");
	fprintf(stdout, "LINKS Foundation -- Alberto Scionti (2022)\n");
	//
	// initialize HW platform variables
	hw_platform = NULL;	
	fpga_device = NULL;
	// searching for an OpenCL platform and device
	clGetPlatformIDs(SET, &hw_platform, NULL);
	clGetDeviceIDs(hw_platform, CL_DEVICE_TYPE_ALL, SET, &fpga_device, NULL);
	// cretae a context
	hw_context = clCreateContext(NULL, SET, &fpga_device, NULL, NULL, NULL);
	// create the command queue
	cmd_queue = clCreateCommandQueue(hw_context, &fpga_device, UNSET, NULL);
	// reading the kernel binary
	max_kernel_size = 0x10000000;
	bitstream = (unsigned char*) malloc(max_kernel_size);
	if (argc == 2)
	{
		strcpy(bitstream_name, argv[1]);
		fprintf(stdout, "[inf] loading %s.cl\n", bitstream_name);
	}
	else
	{
		fprintf(stdout, "[inf] trying to load default kernel (%s.cl)\n", bitstream_name);
	}
	//
	// cleanup environment
	free(bitstream);
	// complete the execution
	fprintf(stdout, "Test Program completed\n\n");
	return (SUCCESS);
}
