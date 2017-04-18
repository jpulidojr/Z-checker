#include <stdio.h>
#include <stdlib.h>
#include "zc.h"
void main()
{
	int count, i , j;
	char* cmd = "cat R2.txt"; //this line is to simulate the R command with vector as output
	float* data;
	int status = ZC_executeCmd_RfloatVector(cmd, &count, &data);
	if(status!=ZC_SCES)
	{
		printf("Error 1: unsuccessful getting results by running Rscript.");
		exit(0);
	}
	
	int m, n;
	char* cmd2 = "cat R3.txt"; //this line is to simulate the R command with matrix as output
	float* data2;
	status = ZC_executeCmd_RfloatMatrix(cmd2, &m, &n, &data2);
	if(status!=ZC_SCES)
	{
		printf("Error 2: unsuccessful getting results by running Rscript.");
		exit(0);		
	}

	printf("=============data1============\n");
	for(i=0;i<count;i++)
		printf("%f ",data[i]);
	printf("\n");
	printf("=============data2============\n");
	for(i=0;i<m;i++)
	{
		for(j=0;j<n;j++)
			printf("%f ", data2[i*n+j]);
		printf("\n");
	}
}