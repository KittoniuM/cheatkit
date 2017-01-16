#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <sys/uio.h>

struct options
{
	int pid;
	char* function;
	char* fileName;
};

struct module
{
	long start;
	long end;
	long size;
	unsigned read;
	unsigned write;
};

void RunEqualSearch(FILE *mapsFile);

void parse(const struct options *opt)
{

	if(!opt->pid || !opt->fileName || !opt->function)	{
		printf("One of option arguments wrong.\n");
		abort();
	}

	char mapLocation[255];

	sprintf(mapLocation, "/proc/%i/maps", opt->pid);

	FILE *mapsFile = fopen(mapLocation, "rb");

	if(!mapsFile) {
		printf("Make sure that pid is right. (Can't find map location)\n");
		abort();
	}

	FILE *outputFile = fopen(opt->fileName, "rb");

	if(!outputFile) {
		printf("can't find previous file. Creating new one.\n");
		outputFile = fopen(opt->fileName, "ab+");
		fclose(outputFile);
		outputFile = fopen(opt->fileName, "rb");
	} 
	else
	{
		printf("Found previous file. Procceding with scanning memory\n");
	}

	printf("You provided %s function.\n", opt->function);

	switch(*opt->function)
	{
		case 'e' :
		{
			RunEqualSearch(mapsFile);
			break;
		}

		default :
		{
			printf("Function argument is wrong.");
			abort();
			break;
		}
	}
}

void RunEqualSearch(FILE *mapsFile)
{
	unsigned long start, end;
	char permisions[4];

	while(fscanf(mapsFile, "%x-%x %s",&start, &end, permisions) != EOF)
	{
		printf("%x-%x %s\n", start, end, permisions);

		fgetc(mapsFile);
	}

	printf("Scanning done.\n");
}

int main(int argc, char *argv[])
{

	struct options opt = {0};

	int op=0;

	while (( op = getopt(argc,argv,"p:o:f:")) != -1)
	{
		switch (op)
		{
			case 'p' :
			{
					opt.pid = atoi(optarg);
					break;
			}

			case 'o' :
			{
					opt.fileName = optarg;
					break;
			}

			case 'f' :
			{
					opt.function = optarg;
					break;
			}
		}
	}

	parse(&opt);

	return 0;
}