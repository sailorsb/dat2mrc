#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "mrc.h"

using namespace std;
	int startnum=1;
	int endnum=1;
	char *prefix = "stack";
	char *inputprefix = "-";
	int width = 7676;
	int height = 7420;
	//int width = 7420;
	//int height = 7676;
	int frame = 32;
	int prefixstart = 1;
	bool delraw = false;
	char * buf=NULL;
	const size_t bufsize = 4*1048576;
	int waittime=120;
	int type=2;//float

void usage()
{
	fprintf(stdout,"Convert gatan dat to mrc stack.\n");
	fprintf(stdout,"Update:20190817 support K3.\n");
	fprintf(stdout,"Usage: ./dat2mrc \n");
	fprintf(stdout,"\t -s num\t--start \tinput start num\n");
	fprintf(stdout,"\t -e num\t--end \tinput end num,if not used, process *.dat\n");
	fprintf(stdout,"\t [-c num]\t--column \tcolumn,default is 7676\n");//7420
	fprintf(stdout,"\t [-r num]\t--row \trow,default is 7420\n");//7676
	fprintf(stdout,"\t [-f num]\t--frame \tframes,default is 32\n");
	fprintf(stdout,"\t [-w num]\t--wait \twait time ,default is 120 seconds\n");
	fprintf(stdout,"\t [-i name]\t--inputprefix \tinput prefix,default is \"-\"\n");
	fprintf(stdout,"\t [-p name]\t--prefix \toutput prefix,default is stack\n");
	fprintf(stdout,"\t [-b num]\t--prefixbegin \toutput prefix begin number,default is 1\n");
	fprintf(stdout,"\t [-t num]\t--type \tmrc file type. 0--char(1 byte/pixel,-128~127) 2--float(4 byte/pixel) 5--unsigned char(1 byte/pixel,0~255)\n");
	fprintf(stdout,"\t [-d]\t--delraw \tif use this flag, will delete .dat file when create stack file\n");
	fprintf(stdout,"\t [-3]\t--k3 \tif use this flag and unset column,row,type,them will be set colum=11520 row=8184 byte=0 .\n");
	
	fprintf(stdout,"\t [-h]\t--help \tshow this message\n");
}
void mergemrc(int datnum,int prefixnum)
{
	char inputfilename[256]={0};
	char outputfilename[256]={0};
	FILE *filein,*fileout;
	sprintf(outputfilename,"%s_%04d.mrcs",prefix,prefixnum);	
	fileout = fopen(outputfilename,"wb");
	MRCHeader header;
	memset(&header,0,sizeof(header));
	header.nx = width;
	header.ny = height;
	header.nz = frame;
	header.mode = type;
	header.map[0]='M';
	header.map[1]='A';
	header.map[2]='P';
	header.map[3]=' ';
	header.nlabels=1;
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(header.label[0],"Create by eTas at %4d-%02d-%02d %02d:%02d:%02d",
		timenow->tm_year+1900,timenow->tm_mon+1,timenow->tm_mday,timenow->tm_hour,timenow->tm_min,timenow->tm_sec);
	int realFrame=0;
	fwrite(&header,sizeof(header),1,fileout);
	size_t filelen;
	bool isNull=false;
	int sumwaittime = waittime;
	
	sprintf(inputfilename,"%s%04d-%04d.dat",inputprefix,datnum,frame);
	filein = fopen(inputfilename,"rb");
	if(filein==NULL)
	{
		printf("Frame %d not found. Wait %ds\n",frame,waittime);
	}
	else
	{
		sumwaittime=0;
		fclose(filein);
	}
	while(sumwaittime>0)
	{
		sprintf(inputfilename,"%s%04d-%04d.dat",inputprefix,datnum,frame);
		filein = fopen(inputfilename,"rb");
		if(filein==NULL)
		{
			sleep(1);
			sumwaittime--;
		}
		else
		{
			fclose(filein);
			break;
		}
	}
	for(int i=1;i<=frame;++i)
	{
		sprintf(inputfilename,"%s%04d-%04d.dat",inputprefix,datnum,i);
		filein = fopen(inputfilename,"rb");
		if(filein==NULL)
		{
			printf("Can not found this file %s\n",inputfilename);
			if(i==1)isNull=true;
			break;
		}
		fseek(filein,0,SEEK_END);
		filelen=ftell(filein);
		rewind(filein);
		//printf("%d\n",filelen);
		size_t realread=0;
		do
		{

			if(filelen>=bufsize)
			{
				realread=bufsize;
				filelen-=bufsize;
			}else
			{
				realread=filelen;
			}
			fread(buf,realread,1,filein);
			fwrite(buf,realread,1,fileout);
		}while(realread==bufsize);
		++realFrame;
		fclose(filein);
		//printf("merge %s\n",inputfilename);
		if(delraw)
		{
			remove(inputfilename);
			//printf("delete %s\n",inputfilename);
		}
	}
	if(realFrame!=frame)
	{
		fseek(fileout,0,SEEK_SET);
		header.nz = realFrame;
		fwrite(&header,sizeof(header),1,fileout);
	}
	fclose(fileout);
	if(isNull)
		remove(outputfilename);
}
int main(int argc,char* argv[])
{
	int opt;
	static const char *shortopts = "s:e:i:p:c:r:f:b:w:t:3dh";
	struct option longopts[] = {
		{ "start", required_argument, NULL, 's' },
		{ "end", required_argument, NULL, 'e' },
		{ "prefix", required_argument, NULL, 'p' },
		{ "inputprefix", required_argument, NULL, 'i' },
		{ "column", required_argument, NULL, 'c' },
		{ "row", required_argument, NULL, 'r' },
		{ "frame", required_argument, NULL, 'f' },
		{ "wait", required_argument, NULL, 'w' },
		{ "prefixstart", required_argument, NULL, 'b' },
		{ "type", required_argument, NULL, 't' },
		{ "delraw", no_argument, NULL, 'd' },
		{ "k3", no_argument, NULL, '3' },
		{ "help", no_argument, NULL, 'h' },
		//{ 0, 0, 0, 0, 0, 0, 0, 0 },
	};
	if(argc<3)
	{
		usage();
		return -1;
	}
	while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch (opt) {
		case 's':
			startnum = atoi(optarg);
			break;
		case 'e':
			endnum = atoi(optarg);
			break;
		case 'p':
			prefix = optarg;
			break;
		case 'i':
			inputprefix = optarg;
			break;
		case 'c':
			width = atoi(optarg);
			break;
		case 'r':
			height = atoi(optarg);
			break;
		case 'f':
			frame = atoi(optarg);
			break;
		case 'w':
			waittime = atoi(optarg);
			if(waittime<0)waittime=1;
			break;
		case 'b':
			prefixstart = atoi(optarg);
			break;
		case 't':
			type = atoi(optarg);
			break;
		case 'd':
			delraw=true;
			break;
		case '3':
			width = 11520;
			height = 8184;
			type = 0;
			break;
		case 'h':
			usage();
			return 0;
			break;
		default:
			fprintf(stdout, "The opt -%c is undefined\n", opt);
			usage();
			return -1;
			break;
		}

	}
	int stacknum = prefixstart;
	buf = (char*)malloc(bufsize);
	for(int i=startnum;i<=endnum;++i)
	{
		mergemrc(i,stacknum++);
		printf("process %04d --> %04d finished.\n",i,stacknum-1);
	}	
	free(buf);
	return 0;
}
