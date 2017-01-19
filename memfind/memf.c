#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/uio.h>

struct options
{
	int pid;
	char* function;
	char* fileName;
};

struct region
{
	long start;
	long end;
	long size;
	unsigned read;
	unsigned write;
};

void runEqualSearch();

bool getRegions(struct region *regions, const struct options *opt)
{
	char mapLocation[255];

	sprintf(mapLocation, "/proc/%i/maps", opt->pid);

	FILE *maps_file = fopen(mapLocation, "rb");

	if(!maps_file) {
		printf("Make sure that pid is right. (Can't find map location)\n");
		return false;
	}

	unsigned long start, end;
	char permisions[4];

	while(fscanf(maps_file, "%x-%x %s",&start, &end, permisions) != EOF)
	{
		printf("%x-%x %s\n", start, end, permisions);

		fgetc(maps_file);
	}

	printf("Scanning done.\n");

	return true;
}

void parse(const struct options *opt)
{

	if(!opt->pid || !opt->fileName || !opt->function)	{
		printf("One of option arguments wrong.\n");
		return;
	}

	FILE *output_file = fopen(opt->fileName, "rb");

	if(!output_file) {
		printf("can't find previous file. Creating new one.\n");
		output_file = fopen(opt->fileName, "ab+");
		fclose(output_file);
		output_file = fopen(opt->fileName, "rb");
	} 
	else
	{
		printf("Found previous file. Procceding with scanning memory\n");
	}

	printf("You provided %s function.\n", opt->function);

	struct region *to_scan;

	if(!getRegions(to_scan, opt)) {
		printf("Can't get regions of maps file");
		return;
	}

	switch(*opt->function)
	{
		case 'e' :
		{
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

void runEqualSearch()
{

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