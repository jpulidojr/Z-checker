/**
 *  @file zc.c
 *  @author Sheng Di and Dingwen Tao
 *  @date Aug, 2016
 *  @brief ZC_Init, Compression and Decompression functions
 *  (C) 2016 by Mathematics and Computer Science (MCS), Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "zc.h"
#include "rw.h"
#include "ZC_Hashtable.h"

int sysEndianType; //endian type of the system
int dataEndianType; //endian type of the data

int checkingStatus; //0 refers to probe_compressor, 1 refers to analyze_data, and 2 indicates compare_compressor
//char *ZC_workspaceDir;

int errorBoundMode; //ABS, REL, ABS_AND_REL, or ABS_OR_REL

char *zc_cfgFile;

double absErrBound;
double relBoundRatio;

int minValueFlag = 1;
int maxValueFlag = 1;
int valueRangeFlag = 1;
int avgValueFlag = 1;
int entropyFlag = 1;
int autocorrFlag = 1;
int fftFlag = 1;
int lapFlag = 1;

int compressTimeFlag = 1;
int decompressTimeFlag = 1;
int compressSizeFlag = 1;

int minAbsErrFlag = 1;
int avgAbsErrFlag = 1;
int maxAbsErrFlag = 1;
int autoCorrAbsErrFlag = 1;
int absErrPDFFlag = 1;

int minRelErrFlag = 1;
int avgRelErrFlag = 1;
int maxRelErrFlag = 1;

int rmseFlag = 1;
int nrmseFlag = 1;
int snrFlag = 1;
int psnrFlag = 1;
int pearsonCorrFlag = 1;

int plotAbsErrPDFFlag = 1;
int plotAutoCorrFlag = 1;
int plotFFTAmpFlag = 1;
int plotEntropyFlag = 1;

int checkCompressorsFlag = 1; //corresponding to plotCompressionResult flag in ec.config

int ZC_versionNumber[3];

struct timeval startCmprTime;
struct timeval endCmprTime;  /* Start and end times */
struct timeval startDecTime;
struct timeval endDecTime;
double totalCmprCost = 0;
double totalDecCost = 0;

int compressors_count = 0;
char* compressors[20];
char* compressors_dir[20];
char* compareData_dir[20];
char* properties_dir[20];

char* comparisonCases;

void cost_startCmpr()
{
	gettimeofday(&startCmprTime, NULL);
}

double cost_endCmpr()
{
	struct timeval endCmprTime;
	gettimeofday(&endCmprTime, NULL);
	double elapsed = ((endCmprTime.tv_sec*1000000+endCmprTime.tv_usec)-(startCmprTime.tv_sec*1000000+startCmprTime.tv_usec))/1000000.0;
	return elapsed;
}

void cost_startDec()
{
	gettimeofday(&startDecTime, NULL);
}

double cost_endDec()
{
	struct timeval endDecTime;
	gettimeofday(&endDecTime, NULL);
	double elapsed = ((endDecTime.tv_sec*1000000+endDecTime.tv_usec)-(startDecTime.tv_sec*1000000+startDecTime.tv_usec))/1000000.0;
	return elapsed;
}

int ZC_Init(char *configFilePath)
{
	char str[512]="", str2[512]="", str3[512]="";
	zc_cfgFile = configFilePath;
	int loadFileResult = ZC_LoadConf();
	if(loadFileResult==0)
		exit(0);
	
	return 0;
}

int ZC_computeDataLength(int r5, int r4, int r3, int r2, int r1)
{
	int dataLength;
	if(r1==0) 
	{
		dataLength = 0;
	}
	else if(r2==0) 
	{
		dataLength = r1;
	}
	else if(r3==0) 
	{
		dataLength = r1*r2;
	}
	else if(r4==0) 
	{
		dataLength = r1*r2*r3;
	}
	else if(r5==0) 
	{
		dataLength = r1*r2*r3*r4;
	}
	else 
	{
		dataLength = r1*r2*r3*r4*r5;
	}
	return dataLength;
}

ZC_DataProperty* ZC_startCmpr(char* varName, int dataType, void *oriData, int r5, int r4, int r3, int r2, int r1)
{
	int i;
	double min,max,valueRange,sum = 0,avg;
	ZC_DataProperty* property = (ZC_DataProperty*)malloc(sizeof(ZC_DataProperty));
	memset(property, 0, sizeof(ZC_DataProperty));
	
	property->varName = varName;
	property->numOfElem = ZC_computeDataLength(r5,r4,r3,r2,r1);
	property->dataType = dataType;
	property->data = oriData;
	property->r5 = r5;
	property->r4 = r4;
	property->r3 = r3;
	property->r2 = r2;
	property->r1 = r1;
	
	if(dataType == ZC_FLOAT)
	{
		float* data = (float*)oriData;
		min = data[0];
		max = data[0];
		for(i=0;i<property->numOfElem;i++)
		{
			if(min>data[i]) min = data[i];
			if(max<data[i]) max = data[i];
			sum += data[i];
		}
		double med = min+(max-min)/2;
		double sum_of_square = 0;
		for(i=0;i<property->numOfElem;i++)
			sum_of_square += (data[i] - med)*(data[i] - med);
		property->zeromean_variance = sum_of_square/property->numOfElem;		
	}
	else
	{
		double* data = (double*)oriData;
		min = data[0];
		max = data[0];
		for(i=0;i<property->numOfElem;i++)
		{
			if(min>data[i]) min = data[i];
			if(max<data[i]) max = data[i];
			sum += data[i];
		}			
		double med = min+(max-min)/2;
		double sum_of_square = 0;
		for(i=0;i<property->numOfElem;i++)
			sum_of_square += (data[i] - med)*(data[i] - med);
		property->zeromean_variance = sum_of_square/property->numOfElem;			
	}
	property->minValue = min;
	property->maxValue = max;
	property->valueRange = max - min;
	property->avgValue = sum/property->numOfElem;
	
	if(compressTimeFlag)
		cost_startCmpr();
	return property;
}

ZC_DataProperty* ZC_startCmpr_withDataAnalysis(char* varName, int dataType, void *oriData, int r5, int r4, int r3, int r2, int r1)
{
	ZC_DataProperty* property = ZC_genProperties(varName, dataType, oriData, r5, r4, r3, r2, r1);
	char tgtWorkspaceDir[ZC_BUFS];
	sprintf(tgtWorkspaceDir, "dataProperties");
	ZC_writeDataProperty(property, tgtWorkspaceDir);
	if(compressTimeFlag)
		cost_startCmpr();
	return property;
}

ZC_CompareData* ZC_endCmpr(ZC_DataProperty* dataProperty, int cmprSize)
{
	double cmprTime;
	if(compressTimeFlag)
		cmprTime = cost_endCmpr();

	ZC_CompareData* compareResult = (ZC_CompareData*)malloc(sizeof(ZC_CompareData));
	memset(compareResult, 0, sizeof(ZC_CompareData));	
	int elemSize = dataProperty->dataType==ZC_FLOAT? 4: 8;	
	
	if(compressTimeFlag)
	{
		compareResult->compressTime = cmprTime;
		compareResult->compressRate = dataProperty->numOfElem*elemSize/cmprTime; //in MB/s		
	}

	if(compressSizeFlag)
	{
		compareResult->compressSize = cmprSize;
		compareResult->compressRatio = (double)(dataProperty->numOfElem*elemSize)/cmprSize;
		if(dataProperty->dataType == ZC_DOUBLE)
			compareResult->rate = 64.0/compareResult->compressRatio;
		else
			compareResult->rate = 32.0/compareResult->compressRatio;
	}
	compareResult->property = dataProperty;
	return compareResult;
}

void ZC_startDec()
{
	if(decompressTimeFlag)
		cost_startDec();
}

void ZC_endDec(ZC_CompareData* compareResult, char* solution, void *decData)
{
	int elemSize = compareResult->property->dataType==ZC_FLOAT? 4: 8;	
	
	if(decompressTimeFlag)
	{
		double decTime = cost_endDec();
		compareResult->decompressTime = decTime;  //in seconds
		compareResult->decompressRate = compareResult->property->numOfElem*elemSize/decTime; // in MB/s		
	}
	if(compareResult==NULL)
	{
		printf("Error: compressionResults==NULL. \nPlease construct ZC_CompareData* compareResult using ZC_compareData() or ZC_endCmpr().\n");
		exit(0);
	}
	ZC_compareData_dec(compareResult, decData);
	ZC_writeCompressionResult(compareResult, solution, compareResult->property->varName, "compressionResults");
}

void ZC_plotHistogramResults(int cmpCount, char** compressorCases)
{
	if(cmpCount==0)
	{
		printf("Error: compressors_count==0.\n Please run ZC_Init(config) first..\n");
		printf("If you already have ZC_Init(config) in your code, please make sure checkCompressors=1.\n");
		exit(0);
	}
	int i, j, count = ecPropertyTable->count;
	
	char** keys = ht_getAllKeys(ecPropertyTable);
	char* cmprRatioLines[count+1];
	char* cmprRateLines[count+1];
	char* dcmprRateLines[count+1];
	char* psnrLines[cmpCount*count+1];
	cmprRatioLines[0] = (char*)malloc(2*ZC_BUFS);
	cmprRateLines[0] = (char*)malloc(2*ZC_BUFS);
	dcmprRateLines[0] = (char*)malloc(2*ZC_BUFS);	
	psnrLines[0] = (char*)malloc(2*ZC_BUFS);
	
	//constructing the field line
	sprintf(cmprRatioLines[0], "%s", "cmprRatio");
	sprintf(cmprRateLines[0], "%s", "cmprRate");
	sprintf(dcmprRateLines[0], "%s", "dcmprRate");	
	sprintf(psnrLines[0], "%s", "psnr");
	for(i=0;i<cmpCount;i++)
	{
		sprintf(cmprRatioLines[0], "%s %s", cmprRatioLines[0],compressorCases[i]);
		sprintf(cmprRateLines[0], "%s %s", cmprRateLines[0], compressorCases[i]);
		sprintf(dcmprRateLines[0], "%s %s", dcmprRateLines[0], compressorCases[i]);		
		sprintf(psnrLines[0], "%s %s", psnrLines[0], compressorCases[i]);
	}
	sprintf(cmprRatioLines[0], "%s\n", cmprRatioLines[0]);
	sprintf(cmprRateLines[0], "%s\n", cmprRateLines[0]);
	sprintf(dcmprRateLines[0], "%s\n", dcmprRateLines[0]);		
	sprintf(psnrLines[0], "%s\n", psnrLines[0]);	

	double maxCR = 0, maxCRT = 0, maxDCRT = 0, maxPSNR = 0;

	//constructing the data
	for(i=0;i<count;i++)
	{
		char* key = keys[i];
		char* line1 = (char*)malloc(10*ZC_BUFS); //suppose there are 10 compressors
		char* line2 = (char*)malloc(10*ZC_BUFS); //suppose there are 10 compressors
		char* line3 = (char*)malloc(10*ZC_BUFS); //suppose there are 10 compressors
		char* line4 = (char*)malloc(10*ZC_BUFS); //suppose there are 10 compressors		
		char* compVarCase = (char*)malloc(ZC_BUFS);
		sprintf(compVarCase, "%s", key);
		sprintf(line1, "%s", key);
		sprintf(line2, "%s", key);
		sprintf(line3, "%s", key);
		sprintf(line4, "%s", key);		
		
		for(j=0;j<cmpCount;j++)
		{
			char* compressorName = compressorCases[j];
			sprintf(compVarCase, "%s:%s", compressorName, key);
			ZC_CompareData* compressResult = ht_get(ecCompareDataTable, compVarCase);
			if(compressResult==NULL)
			{
				printf("Error: compressResult==NULL. %s cannot be found in compression result\n", compVarCase);
				sprintf(line1, "%s -", line1);
				sprintf(line2, "%s -", line2);
				sprintf(line3, "%s -", line3);
				sprintf(line4, "%s -", line4);				
			}
			else
			{
				double cr = compressResult->compressRatio;
				if(maxCR<cr)
					maxCR = cr;
				sprintf(line1, "%s %f", line1, cr);
				double crt = compressResult->compressRate;
				if(maxCRT<crt)
					maxCRT = crt;
				sprintf(line2, "%s %f", line2, crt);
				double dcrt = compressResult->decompressRate;
				if(maxDCRT<dcrt)
					maxDCRT = dcrt;
				sprintf(line3, "%s %f", line3, dcrt);				
				double psnr = compressResult->psnr;
				if(maxPSNR<psnr)
					maxPSNR = psnr;
				sprintf(line4, "%s %f", line4, psnr);
			}			
		}
		sprintf(line1, "%s\n", line1);
		cmprRatioLines[i+1] = line1;
		sprintf(line2, "%s\n", line2);
		cmprRateLines[i+1] = line2;
		sprintf(line3, "%s\n", line3);
		dcmprRateLines[i+1] = line3;
		sprintf(line4, "%s\n", line4);
		psnrLines[i+1] = line4;				
		free(compVarCase);
	}
	
	char compreStringKey[ZC_BUFS_LONG];
	strcpy(compreStringKey, compressorCases[0]);
	for(i=1;i<cmpCount;i++)
	{
		strcat(compreStringKey, "_");
		strcat(compreStringKey, compressorCases[i]);
	} 
	
	char cmpRatioKey[ZC_BUFS_LONG], cmpRateKey[ZC_BUFS_LONG], dcmpRateKey[ZC_BUFS_LONG], psnrKey[ZC_BUFS_LONG];
	char cmpRatioDataFile[ZC_BUFS_LONG], cmpRateDataFile[ZC_BUFS_LONG], dcmpRateDataFile[ZC_BUFS_LONG], psnrDataFile[ZC_BUFS_LONG];
	char cmpRatioPlotFile[ZC_BUFS_LONG], cmpRatePlotFile[ZC_BUFS_LONG], dcmpRatePlotFile[ZC_BUFS_LONG], psnrPlotFile[ZC_BUFS_LONG];
	char cmpRatioCmd[ZC_BUFS_LONG], cmpRateCmd[ZC_BUFS_LONG], dcmpRateCmd[ZC_BUFS_LONG], psnrCmd[ZC_BUFS_LONG];
	sprintf(cmpRatioKey, "cratio_%s", compreStringKey);
	sprintf(cmpRateKey, "crate_%s", compreStringKey);
	sprintf(dcmpRateKey, "drate_%s", compreStringKey);
	sprintf(psnrKey, "psnr_%s", compreStringKey);
	sprintf(cmpRatioDataFile, "%s.txt", cmpRatioKey);
	sprintf(cmpRateDataFile, "%s.txt", cmpRateKey);
	sprintf(dcmpRateDataFile, "%s.txt", dcmpRateKey);
	sprintf(psnrDataFile, "%s.txt", psnrKey);
	sprintf(cmpRatioPlotFile, "%s.p", cmpRatioKey);
	sprintf(cmpRatePlotFile, "%s.p", cmpRateKey);
	sprintf(dcmpRatePlotFile, "%s.p", dcmpRateKey);	
	sprintf(psnrPlotFile, "%s.p", psnrKey);
	
	sprintf(cmpRatioCmd, "gnuplot \"%s\"", cmpRatioPlotFile);
	sprintf(cmpRateCmd, "gnuplot \"%s\"", cmpRatePlotFile);
	sprintf(dcmpRateCmd, "gnuplot \"%s\"", dcmpRatePlotFile);	
	sprintf(psnrCmd, "gnuplot \"%s\"", psnrPlotFile);
	//writing the data to file
	ZC_writeStrings(count+1, cmprRatioLines, cmpRatioDataFile);
	ZC_writeStrings(count+1, cmprRateLines, cmpRateDataFile);
	ZC_writeStrings(count+1, dcmprRateLines, dcmpRateDataFile);	
	ZC_writeStrings(count+1, psnrLines, psnrDataFile);
		
	//generate GNUPLOT scripts, and plot the data by running the scripts
	char** scriptLines = genGnuplotScript_histogram(cmpRatioKey, "txt", 26, 1+cmpCount, "Variables", "Compression Ratio", (long)(maxCR*1.3));
	ZC_writeStrings(18, scriptLines, cmpRatioPlotFile);
	system(cmpRatioCmd);
	
	scriptLines = genGnuplotScript_histogram(cmpRateKey, "txt", 26, 1+cmpCount, "Variables", "Compression Rate", (long)(maxCRT*1.3));
	ZC_writeStrings(18, scriptLines, cmpRatePlotFile);
	system(cmpRateCmd);

	scriptLines = genGnuplotScript_histogram(dcmpRateKey, "txt", 26, 1+cmpCount, "Variables", "Decompression Rate", (long)(maxDCRT*1.3));
	ZC_writeStrings(18, scriptLines, dcmpRatePlotFile);
	system(dcmpRateCmd);
	
	scriptLines = genGnuplotScript_histogram(psnrKey, "txt", 26, 1+cmpCount, "Variables", "PSNR", (long)(maxPSNR*1.3));
	ZC_writeStrings(18, scriptLines, psnrPlotFile);
	system(psnrCmd);	
	
	free(scriptLines);
	for(i=0;i<count+1;i++)
	{
		free(cmprRatioLines[i]);
		free(cmprRateLines[i]);
		free(dcmprRateLines[i]);
		free(psnrLines[i]);
	}
}

void ZC_plotComparisonCases()
{
	int i=0,n,j;
	char* cases[ZC_BUFS]; //100 cases at most
	char* ccase = strtok(comparisonCases, " ");
	for(i=0;ccase!=NULL;i++)
	{
		if(i>ZC_BUFS) 
		{
			printf("Error: # comparison cases cannot be greater than %d\n", ZC_BUFS);
			exit(0);
		}
		cases[i] = ccase;
		ccase = strtok(NULL, " ");
	}
	n = i;
	for(i=0;i<n;i++)
	{
		char* cmprssors[ZC_BUFS]; 
		char* cmprsor = strtok(cases[i], ",");
		for(j=0;cmprsor!=NULL;j++)
		{
			cmprssors[j] = cmprsor; 
			cmprsor = strtok(NULL, ",");
		}
		int m = j;
		ZC_plotHistogramResults(m, cmprssors);
	}
}

/**
 * @deprecated 
 * */
void ZC_plotCompressionRatio()
{
	if(compressors_count==0)
	{
		printf("Error: compressors_count==0.\n Please run ZC_Init(config) first.\n");
		exit(0);
	}
	int i, j, count = ecPropertyTable->count;
	
	//TODO: Construct the cases by using compressor name and the variable name
	//TODO: traverse all the property-variables through the propertyTable
	char** keys = ht_getAllKeys(ecPropertyTable);
	char* dataLines[count+1];
	dataLines[0] = "data";
	for(i=0;i<compressors_count;i++)
		sprintf(dataLines[0], "%s %s", dataLines[0], compressors[i]);
	sprintf(dataLines[0], "%s\n", dataLines[0]);
	
	double maxCR = 0;
	
	for(i=0;i<count;i++)
	{
		char* key = keys[i];
		char* line = (char*)malloc(10*ZC_BUFS);
		char* compVarCase = (char*)malloc(ZC_BUFS);
		sprintf(compVarCase, "%s", key);
		sprintf(line, "%s", key);
		for(j=0;j<compressors_count;j++)
		{
			char* compressorName = compressors[j];
			sprintf(compVarCase, "%s:%s", compressorName, key);
			ZC_CompareData* compressResult = ht_get(ecCompareDataTable, compVarCase);
			if(compressResult==NULL)
			{
				printf("Error: compressResult==NULL. %s cannot be found in compression result\n", compVarCase);
				sprintf(line, "%s -", line);
			}
			else
			{
				double cr = compressResult->compressRatio;
				if(maxCR<cr)
					maxCR = cr;
				sprintf(line, "%s %f", line, cr);				
			}
		}		
		sprintf(line, "%s\n", line);
		dataLines[i+1] = line;
		free(compVarCase);
	}
	
	ZC_writeStrings(count+1, dataLines, "compressionRatio.txt");
	
	//TODO: generate the GNUPLOT script.
	char** scriptLines = genGnuplotScript_histogram("compressionRatio", "txt", 26, 1+compressors_count, "Variables", "Compression Ratio", (long)(maxCR*1.3));
	ZC_writeStrings(18, scriptLines, "compressionRatio.p");
	system("gnuplot compressionRatio.p");
}

void ZC_plotRateDistortion()
{
	if(compressors_count==0)
	{
		printf("Error: compressors_count==0.\n Please run ZC_Init(config) first.\n");
		exit(0);
	}
	int i, j;
	
	//select variables
	char** variables = ht_getAllKeys(ecPropertyTable);
	for(i=0;i<ecPropertyTable->count;i++)
	{
		int count, validLineNum; //count: the number of test-cases
		char** cmpResList = getCompResKeyList(variables[i], &count);
		char** dataLines_psnr = extractRateDistortion_psnr(count, cmpResList, &validLineNum);
		if(dataLines_psnr!=NULL&&validLineNum>0)
		{
			char fileName[ZC_BUFS];
			sprintf(fileName, "rate-distortion_psnr_%s.txt", variables[i]);
			ZC_writeStrings(count+1, dataLines_psnr, fileName);		
			
			//TODO: generate the GNUPLOT script.
			sprintf(fileName, "rate-distortion_psnr_%s", variables[i]);
			char** scriptLines = genGnuplotScript_linespoints(fileName, "txt", 26, 1+compressors_count, "Rate", "Distortion");
			sprintf(fileName, "rate-distortion_psnr_%s.p", variables[i]);
			ZC_writeStrings(24, scriptLines, fileName);
			char cmd[ZC_BUFS];
			sprintf(cmd, "gnuplot %s", fileName);
			system(cmd);
			
			for(j=0;j<count+1;j++)
				if(dataLines_psnr[j]!=NULL) free(dataLines_psnr[j]);						
		}
		
		char** dataLines_snr = extractRateDistortion_snr(count, cmpResList, &validLineNum);
		if(dataLines_snr!=NULL&&validLineNum>0)
		{
			char fileName[ZC_BUFS];
			sprintf(fileName, "rate-distortion_snr_%s.txt", variables[i]);
			ZC_writeStrings(count+1, dataLines_snr, fileName);		
			
			//TODO: generate the GNUPLOT script.
			sprintf(fileName, "rate-distortion_snr_%s", variables[i]);
			char** scriptLines = genGnuplotScript_linespoints(fileName, "txt", 26, 1+compressors_count, "Rate", "Distortion");
			sprintf(fileName, "rate-distortion_snr_%s.p", variables[i]);
			ZC_writeStrings(24, scriptLines, fileName);
			char cmd[ZC_BUFS];
			sprintf(cmd, "gnuplot %s", fileName);
			system(cmd);
			
			for(j=0;j<count+1;j++)
				if(dataLines_snr[j]!=NULL) free(dataLines_snr[j]);						
		}		
		
		free(cmpResList);
	}
}

char** getCompResKeyList(char* var, int* count)
{
	int i, j = 0;
	char** selected = (char**)malloc(ecCompareDataTable->count*sizeof(char*));
	char** cmpResKeys = ht_getAllKeys(ecCompareDataTable);
	for(i=0;i < ecCompareDataTable->count;i++)
	{
		char* tmp = (char*)malloc(strlen(cmpResKeys[i])+1);
		sprintf(tmp, "%s",cmpResKeys[i]); 
		char* cmpssor = strtok(tmp, ":");
		char* varCase = strtok(NULL, ":");
		if(strcmp(var, varCase)==0)
			selected[j++] = cmpResKeys[i];
		free(tmp);
	}
	*count = j;
	return selected;
}

char** extractRateDistortion_psnr(int totalCount, char** cmpResList, int* validLineNum)
{
	int i, j, p, k, q = 1, t = 0;
	
	if(totalCount==0)
		return NULL;
	//construct the dataLines and the field line
	char** dataLines = (char**)malloc((totalCount+1)*sizeof(char*)); //including field file
	memset(dataLines, 0, (totalCount+1)*sizeof(char*));
	
	dataLines[0] = (char*)malloc(ZC_BUFS);
	
	sprintf(dataLines[0], "ratedistortion");
	for(i=0;i<compressors_count;i++)
		sprintf(dataLines[0], "%s %s", dataLines[0], compressors[i]);
	sprintf(dataLines[0], "%s\n", dataLines[0]);
	
	RateDistElem* rdList = (RateDistElem*)malloc(totalCount*sizeof(RateDistElem));
	
	//start checking compressors one by one, constructing the rate distortion curves.	
	for(i=0;i<compressors_count;i++)
	{
		memset(rdList, 0, totalCount*sizeof(RateDistElem));
		p = 0;
		char* compressorName = compressors[i];
		//TODO: scan ecCompareDataTable and select matched records
		for(j=0;j<totalCount;j++)
		{
			char* key = cmpResList[j];
			int ck = checkStartsWith(key, compressorName);
			if(ck)
			{
				ZC_CompareData* compareResult = ht_get(ecCompareDataTable, key);
				RateDistElem e = (RateDistElem)malloc(sizeof(struct RateDistElem_t));
				e->rate = compareResult->rate;
				e->psnr = compareResult->psnr;
				e->maxAbsErr = compareResult->maxAbsErr;
				e->compressRate = compareResult->compressRate;
				rdList[p++] = e;
			}
		}
		
		t += p;
		ZC_quick_sort(rdList, 0, p-1);
		
		for(j=0;j<p;j++)
		{
			RateDistElem e = rdList[j];
			char* l = (char*)malloc(ZC_BUFS);
			sprintf(l, "%f", e->rate);
			for(k=0;k<compressors_count;k++)
			{
				if(k==i && e->maxAbsErr!=0)
				{
					sprintf(l, "%s %f", l, e->psnr);
				}
				else
					sprintf(l, "%s -", l); 
			}
			sprintf(l, "%s\n", l);
			dataLines[q++] = l;
		}
		
		for(j=0;j<totalCount;j++)
			if(rdList[j]!=NULL) free(rdList[j]);
	}

	*validLineNum = t;

	free(rdList);
	return dataLines;
}

char** extractRateDistortion_snr(int totalCount, char** cmpResList, int* validLineNum)
{
	int i, j, p, k, q = 1, t = 0;
	
	if(totalCount==0)
		return NULL;
	//construct the dataLines and the field line
	char** dataLines = (char**)malloc((totalCount+1)*sizeof(char*)); //including field file
	memset(dataLines, 0, (totalCount+1)*sizeof(char*));
	
	dataLines[0] = (char*)malloc(ZC_BUFS);
	
	sprintf(dataLines[0], "ratedistortion");
	for(i=0;i<compressors_count;i++)
		sprintf(dataLines[0], "%s %s", dataLines[0], compressors[i]);
	sprintf(dataLines[0], "%s\n", dataLines[0]);
	
	RateDistElem* rdList = (RateDistElem*)malloc(totalCount*sizeof(RateDistElem));
	
	//start checking compressors one by one, constructing the rate distortion curves.	
	for(i=0;i<compressors_count;i++)
	{
		memset(rdList, 0, totalCount*sizeof(RateDistElem));
		p = 0;
		char* compressorName = compressors[i];
		//TODO: scan ecCompareDataTable and select matched records
		for(j=0;j<totalCount;j++)
		{
			char* key = cmpResList[j];
			int ck = checkStartsWith(key, compressorName);
			if(ck)
			{
				ZC_CompareData* compareResult = ht_get(ecCompareDataTable, key);
				RateDistElem e = (RateDistElem)malloc(sizeof(struct RateDistElem_t));
				e->rate = compareResult->rate;
				e->psnr = compareResult->snr;
				e->maxAbsErr = compareResult->maxAbsErr;
				e->compressRate = compareResult->compressRate;
				rdList[p++] = e;
			}
		}
		
		t += p;
		ZC_quick_sort(rdList, 0, p-1);
		
		for(j=0;j<p;j++)
		{
			RateDistElem e = rdList[j];
			char* l = (char*)malloc(ZC_BUFS);
			sprintf(l, "%f", e->rate);
			for(k=0;k<compressors_count;k++)
			{
				if(k==i && e->maxAbsErr!=0)
				{
					sprintf(l, "%s %f", l, e->psnr);
				}
				else
					sprintf(l, "%s -", l); 
			}
			sprintf(l, "%s\n", l);
			dataLines[q++] = l;
		}
		
		for(j=0;j<totalCount;j++)
			if(rdList[j]!=NULL) free(rdList[j]);
	}

	*validLineNum = t;

	free(rdList);
	return dataLines;
}

void ZC_plotAutoCorr_CompressError()
{
	int i,j;
	char acFileName[ZC_BUFS], acPlotFile[ZC_BUFS], acCmd[ZC_BUFS_LONG];
	char** allCampCaseNames = ht_getAllKeys(ecCompareDataTable);
	for(i=0;i<ecCompareDataTable->count;i++)
	{
		sprintf(acFileName, "%s", allCampCaseNames[i]);
		char** scriptLines = genGnuplotScript_fillsteps(acFileName, "autocorr", 26, 2, "Lags", "ACF");
		sprintf(acPlotFile, "compressionResults/%s-autocorr.p", acFileName);
		ZC_writeStrings(19, scriptLines, acPlotFile);
		//execute .p files using system().
		sprintf(acCmd, "cd compressionResults;gnuplot \"%s-autocorr.p\";mv \"%s.autocorr.eps\" \"%s-autocorr.eps\"", acFileName, acFileName, acFileName);
		system(acCmd);	
		for(j=0;j<19;j++)
			free(scriptLines[j]);
		free(scriptLines);	
	}	
}

void ZC_plotAutoCorr_DataProperty()
{
	int i,j;
	char acFileName[ZC_BUFS], acPlotFile[ZC_BUFS], acCmd[ZC_BUFS_LONG];
	char** allVarNames = ht_getAllKeys(ecPropertyTable);
	for(i=0;i<ecPropertyTable->count;i++)
	{
		sprintf(acFileName, "%s", allVarNames[i]);
		char** scriptLines = genGnuplotScript_fillsteps(acFileName, "autocorr", 26, 2, "Lags", "ACF");
		sprintf(acPlotFile, "dataProperties/%s-autocorr.p", acFileName);		
		ZC_writeStrings(19, scriptLines, acPlotFile);
		//execute .p files using system().
		sprintf(acCmd, "cd dataProperties;gnuplot %s-autocorr.p;mv %s.autocorr.eps %s-autocorr.eps", acFileName, acFileName, acFileName);
		system(acCmd);
		for(j=0;j<19;j++)
			free(scriptLines[j]);
		free(scriptLines);		
	}
}

void ZC_plotFFTAmplitude_OriginalData()
{
	int i,j;
	char ampFileName[ZC_BUFS], ampPlotFile[ZC_BUFS], ampCmd[ZC_BUFS_LONG];
	char** allVarNames = ht_getAllKeys(ecPropertyTable);
	for(i=0;i<ecPropertyTable->count;i++)
	{
		sprintf(ampFileName, "%s.fft", allVarNames[i]);
		sprintf(ampPlotFile, "dataProperties/%s-fft-amp.p", allVarNames[i]);
		char** scriptLines = genGnuplotScript_fillsteps(ampFileName, "amp", 26, 2, "frequency", "amplitude");
		ZC_writeStrings(19, scriptLines, ampPlotFile);
		//execute .p files using system().
		sprintf(ampCmd, "cd dataProperties;gnuplot \"%s-fft-amp.p\";mv \"%s.fft.amp.eps\" \"%s-fft-amp.eps\"", allVarNames[i],allVarNames[i], allVarNames[i]);
		system(ampCmd);
		for(j=0;j<19;j++)
			free(scriptLines[j]);
		free(scriptLines);
	}
}

void ZC_plotFFTAmplitude_DecompressData()
{
	int i,j;
	char ampFileName[ZC_BUFS], ampPlotFile[ZC_BUFS], ampCmd[ZC_BUFS_LONG];
	char** allVarNames = ht_getAllKeys(ecCompareDataTable);
	for(i=0;i<ecCompareDataTable->count;i++)
	{
		sprintf(ampFileName, "%s.fft", allVarNames[i]);
		sprintf(ampPlotFile, "compressionResults/%s-fft-amp.p", allVarNames[i]);
		char** scriptLines = genGnuplotScript_fillsteps(ampFileName, "amp", 26, 2, "frequency", "amplitude");
		ZC_writeStrings(19, scriptLines, ampPlotFile);
		//execute .p files using system().
		sprintf(ampCmd, "cd compressionResults;gnuplot \"%s-fft-amp.p\";mv \"%s.fft.amp.eps\" \"%s-fft-amp.eps\"", allVarNames[i],allVarNames[i], allVarNames[i]);
		system(ampCmd);
		for(j=0;j<19;j++)
			free(scriptLines[j]);
		free(scriptLines);
	}
}

void ZC_plotErrDistribtion()
{
	int i,j;
	char disFileName[ZC_BUFS], disPlotFile[ZC_BUFS], disCmd[ZC_BUFS_LONG];
	char** allVarNames = ht_getAllKeys(ecCompareDataTable);
	for(i=0;i<ecCompareDataTable->count;i++)
	{
		sprintf(disFileName, "%s", allVarNames[i]);
		sprintf(disPlotFile, "compressionResults/%s-dis.p", allVarNames[i]);
		char** scriptLines = genGnuplotScript_fillsteps(disFileName, "dis", 26, 2, "Error", "PDF");
		ZC_writeStrings(19, scriptLines, disPlotFile);
		//execute .p files using system().
		sprintf(disCmd, "cd compressionResults;gnuplot \"%s-dis.p\";mv \"%s.dis.eps\" \"%s-dis.eps\"", disFileName,disFileName, disFileName);
		system(disCmd);
		for(j=0;j<19;j++)
			free(scriptLines[j]);
		free(scriptLines);	
	}
}


void ZC_Finalize()
{
	//TODO: free hashtable memory
	if(ecPropertyTable!=NULL)
		ht_freeTable(ecPropertyTable);
	if(ecCompareDataTable!=NULL)
		ht_freeTable(ecCompareDataTable);
    return ;
}
