// symbolic variables
#define SIZE 	(1024 * 1024) 
#define DELTA	10

// kernel definition
__kernel void TestKernel(__global const int* restrict k_di,
			 __global int* restrict k_do)
{
	// kernel variables
	unsigned int i;

	// kernel body
	for (i = 0; i < SIZE; i++)
	{
		k_do[i] = k_di[i] + DELTA;
	}
}
