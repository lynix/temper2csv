/* Copyright 2014 Alexander Koch <lynix47@gmail.com>
 *
 * This file is part of 'temper2csv'.
 *
 * 'temper2csv' is distributed under the MIT License, see LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "libtemper.h"

#define DEF_OUTF "temperatures.csv"
#define DEF_INTR 300
#define DEF_INTS 2
#define DEF_NUMS 5
#define DEF_VERB false
#ifndef PROGN
	#define PROGN "temper2csv"
#endif

typedef struct {
	char		*out_file;
	uint16_t	int_record;
	uint16_t	int_sample;
	uint16_t	num_sample;
	bool		verbose;
} opts_t;

sem_t	signal;
FILE	*outf;
opts_t	opts;


void *record_loop(void *arg);
void parse_cmdline(int argc, char *argv[]);
void err_exit(const char *format, ...);
void print_help();
double eval_samples(double *samples);


int main(int argc, char *argv[])
{
	parse_cmdline(argc, argv);

	// create or open output file
	char *mode = access(opts.out_file, F_OK) == 0 ? "a" : "w";
	outf = fopen(opts.out_file, mode);
	if (outf == NULL)
		err_exit("failed to open '%s' for writing: %s", opts.out_file,
				strerror(errno));
	// write header
	if (*mode == 'w')
		fputs("timestamp,temperature\n", outf);

	if (sem_init(&signal, 0, 0) < 0)
		err_exit("failed to initialize semaphore: %s", strerror(errno));

	pthread_t worker_thread;
	if (pthread_create(&worker_thread, NULL, record_loop, NULL) < 0)
		err_exit("failed to spawn worker thread, terminating.");

	while (true) {
		sem_post(&signal);
		sleep(opts.int_record);
	}

	return EXIT_SUCCESS;
}


void *record_loop(void *arg)
{
	double *samples = (double *)calloc(opts.num_sample, sizeof(double));
	char *err = NULL;

	while (true) {
		sem_wait(&signal);

		time_t t = time(NULL);
		for (uint16_t i=0; i<opts.num_sample; i++) {
			samples[i] = temper_read(&err);
			if (err != NULL)
				err_exit("failed to retrieve temperature: %s", err);
			sleep(opts.int_sample);
		}

		double value = eval_samples(samples);

		if (opts.verbose) {
			char strbuf[20];
			struct tm *tmp = localtime(&t);
			strftime(strbuf, 20, "%F %T", tmp);
			printf("%s: %.2f°C\n", strbuf, value);
		}

		if (fprintf(outf, "%ld,%2.2f\n", t, value) < 0)
			err_exit("failed to write to output file: %s", strerror(errno));
		fflush(outf);
	}

	return NULL;
}


double eval_samples(double *samples)
{
	// bubble sort
	double tmp;
	for (uint16_t x=0; x<opts.num_sample; x++) {
		for (uint16_t y=0; y<opts.num_sample-1; y++) {
			if (samples[y] > samples[y+1]){
				tmp = samples[y+1];
				samples[y+1] = samples[y];
				samples[y] = tmp;
			}
		}
	}

	// return median
	return samples[(opts.num_sample >> 1)];
}


void parse_cmdline(int argc, char *argv[])
{
	// initialize defaults
	opts.int_record = DEF_INTR;
	opts.int_sample = DEF_INTS;
	opts.num_sample = DEF_NUMS;
	opts.out_file   = DEF_OUTF;
	opts.verbose    = DEF_VERB;

	// parse cmdline arguments
	char c;
	extern int optind, optopt, opterr;
	long val;
	while ((c = getopt(argc, argv, "o:i:n:t:vh")) != -1) {
		switch (c) {
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
				break;
			case 'o':
				opts.out_file = optarg;
				break;
			case 'i':
				val = strtol(optarg, NULL, 10);
				if (val < 1 || val > UINT16_MAX)
					err_exit("invalid value for -%c: %ld", c, val);
				opts.int_record = (uint16_t)val;
				break;
			case 'n':
				val = strtol(optarg, NULL, 10);
				if (val < 1 || val > UINT16_MAX)
					err_exit("invalid value for -%c: %ld", c, val);
				opts.num_sample = (uint16_t)val;
				break;
			case 't':
				val = strtol(optarg, NULL, 10);
				if (val < 1 || val > UINT16_MAX)
					err_exit("invalid value for -%c: %ld", c, val);
				opts.int_sample = (uint16_t)val;
				break;
			case 'v':
				opts.verbose = true;
				break;
			case '?':
				if (optopt == 'o' || optopt == 'i' || optopt == 'n' ||
						optopt == 't')
					fprintf(stderr, "option -%c requires an argument\n",
							optopt);
				exit(EXIT_FAILURE);
				break;
			default:
				fprintf(stderr, "invalid arguments, try -h for help.");
				exit(EXIT_FAILURE);
		}
	}
}


void err_exit(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	fflush(stderr);
	va_end(args);

	fclose(outf);

	exit(EXIT_FAILURE);
}


void print_help()
{
	printf("Usage: %s [OPTIONS]\n\n", PROGN);
	puts  ("OPTIONS:");
	printf("  -o FILE   output file (default: '%s')\n", DEF_OUTF);
	printf("  -i NUM    record period in seconds (default: %d)\n", DEF_INTR);
	printf("  -n NUM    number of samples per record (default: %d)\n",
			DEF_NUMS);
	printf("  -t NUM    sampling delay in seconds (default: %d)\n",
			DEF_INTS);
	puts  ("  -v        verbose output\n");
	puts  ("  -h        display this usage information\n");
	printf("This version of mi3stat was built on %s %s.\n", __DATE__, __TIME__);
}
