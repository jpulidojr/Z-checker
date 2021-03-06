#include <stdio.h>
#include <string.h>
#include "ZC_rw.h"
#include "zc.h"
#include <mpi.h> 

int main(int argc, char * argv[])
{	
    size_t r5=0,r4=0,r3=0,r2=0,r1=0;
    char dataFile[640], outputFilePath[640], oriFilePath[640];
    char *datatype, *cfgFile, *dataDir;

    if(argc < 5)
    {
        printf("Usage: analyzeDataProperty [datatype (-f or -d)] [config_file] [srcDataDir] [dataFileName] [dimension sizes...]\n");
        printf("Example: analyzeDataProperty -f zc.config testdata/x86 testfloat_8_8_128.dat 8 8 128\n");
        exit(0);
    }

    datatype=argv[1];
    cfgFile=argv[2];
    dataDir=argv[3];
    sprintf(dataFile, "%s", argv[4]);
    sprintf(oriFilePath, "%s/%s", dataDir, dataFile);
    if(argc>=6)
        r1 = atoi(argv[5]); //8
    if(argc>=7)
        r2 = atoi(argv[6]); //8
    if(argc>=8)
        r3 = atoi(argv[7]); //128
    if(argc>=9)
        r4 = atoi(argv[8]);
    if(argc>=10)
        r5 = atoi(argv[9]);

    MPI_Init(&argc,&argv);

    int numprocs, myrank;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);/* get the rank */ 

    printf("myrank=%d, cfgFile=%s\n", myrank, cfgFile);
    ZC_Init(cfgFile);

	int dim = ZC_computeDimension(r5, r4, r3, r2, r1);
	size_t offset;
	switch(dim)
	{
	case 1:
		r1 = r1/numprocs;
		offset = r1*myrank;
		break;
	case 2:
		r2 = r2/numprocs;
		offset = r1*r2*myrank;
		break;
	case 3:
		r3 = r3/numprocs;
		offset = r1*r2*r3*myrank;
		break;
	case 4:
		r4 = r4/numprocs;	
		offset = r1*r2*r3*r4*myrank;
		break;
	case 5:
		r5 = r5/numprocs;
		offset = r1*r2*r3*r4*r5*myrank;	
		break;
	}
	
    long globalLength = ZC_computeDataLength_online(r5,r4,r3,r2,r1);
    if(myrank==0)
	    printf("globalLength = %d, %d\n", globalLength, globalDataLength);
    size_t nbEle;
    ZC_DataProperty* property = NULL;
    if(strcmp(datatype, "-f")==0)
    {
		float *data = ZC_readFloatData(oriFilePath, &nbEle);
		property = ZC_genProperties(dataFile, ZC_FLOAT, data+offset, r5, r4, r3, r2, r1);
    }
    else if(strcmp(datatype, "-d")==0)
    {
		double *data = ZC_readDoubleData(oriFilePath, &nbEle);
		property = ZC_genProperties(dataFile, ZC_DOUBLE, data+offset, r5, r4, r3, r2, r1);
    }
    if(myRank==0)
    {
		ZC_printDataProperty(property);
		ZC_writeDataProperty(property, "dataProperties");
		printf("Done\n");		
    }
    MPI_Finalize();
    return 0;
}
