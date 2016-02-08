/*
BCM2835 "GPU_FFT" release 3.0
Copyright (c) 2015, Andrew Holme.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <getopt.h>

#include "mailbox.h"
#include "gpu_fft.h"

static const char Usage[] =
	"Usage: hello_fft.bin -n log2_N [-j jobs] [-l loops] [-f freq] [-i inc] [-d] [-p]\n"
	"  log2_N = log2(FFT_length),       log2_N = 8...22, <0 = forward transform\n"
	"  jobs   = transforms per batch,   jobs>0,        default 1\n"
	"  loops  = number of test repeats, loops>0,       default 1\n"
	"  freq   = frequency in units of FFT length,      default 1\n"
	"  inc    = frequency increment in case jobs>1,    default 1\n"
	"  -d     = dump result\n"
	"  -p     = print performance counters\n";

static const char Options[] = "n:j:l:f:i:dp";

unsigned Microseconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

static double chop(double value)
{
	return fabs(value) < 3E-5 ? 0 : value;
}

int main(int argc, char *argv[])
{
	int i, j, k, ret, loops, fbias, finc, freq, log2_N, jobs,
		debug, perf, N, dir, bad, mb = mbox_open();
	unsigned t[3];
	double tsq[4];

	struct GPU_FFT_COMPLEX *base;
	struct GPU_FFT *fft;

	log2_N = 0;
	jobs = 1;
	loops = 1;
	fbias = 1;
	finc = 1;
	debug = 0;
	perf = 0;

	if (argc < 2)
	{usage:
		printf(Usage);
		return -1;
	}

	j = 0;
 next:
	if (j)
		i = -1;
	else
		i = getopt(argc, argv, Options);
 oldopt:
	//printf("i=%c, optarg=%s\n", i, optarg);
	switch (i)
	{default:
		goto usage;
	 case 'n':
		log2_N = atoi(optarg);
		goto next;
	 case 'l':
		loops = atoi(optarg);
		goto next;
	 case 'j':
		jobs = atoi(optarg);
		goto next;
	 case 'f':
		fbias = atoi(optarg);
		goto next;
	 case 'i':
		finc = atoi(optarg);
		goto next;
	 case 'd':
		debug = 1;
		goto next;
	 case 'p':
		perf = 1;
		goto next;
	 case -1: // end: old style syntax?
		if (optind >= argc)
			break;
		if (j >= 10)
			goto usage;
		i = Options[j];
		optarg = argv[optind++];
		j += 2;
		goto oldopt;
	}

	dir = GPU_FFT_REV;
	if (log2_N < 0)
	{
		dir = GPU_FFT_FWD;
		log2_N = -log2_N;
	}
	if (jobs < 1 || loops < 1)
	{
		printf(Usage);
		return -1;
	}

	N = 1 << log2_N; // FFT length
	ret = gpu_fft_prepare(mb, log2_N, dir, jobs, &fft); // call once

	switch (ret)
	{
	case -1:
		fprintf(stderr, "Unable to enable V3D. Please check your firmware is up to date.\n");
		return -1;
	case -2:
		fprintf(stderr, "log2_N=%d not supported.  Try between 8 and 22.\n", log2_N);
		return -1;
	case -3:
		fprintf(stderr, "Out of memory.  Try a smaller batch or increase GPU memory.\n");
		return -1;
	case -4:
		fprintf(stderr, "Unable to map Videocore peripherals into ARM memory space.\n");
		return -1;
	case -5:
		fprintf(stderr, "Can't open libbcm_host.\n");
		return -1;
	case -6:
		fprintf(stderr, "Can't open obj/twiddles.bin.\n");
		return -1;
	}

	t[2] = 0;
	for (k = 0; k < loops; k++)
	{
		for (j = 0; j < jobs; j++)
			memset(fft->out + j * fft->step, -1, N * sizeof(struct GPU_FFT_COMPLEX));

		for (j = 0; j < jobs; j++)
		{
			base = fft->in + j * fft->step; // input buffer
			if (dir == GPU_FFT_REV)
			{
				freq = j * finc + fbias & (N - 1);
				memset(base, 0, N * sizeof(struct GPU_FFT_COMPLEX));
				base[freq].re = 0.5;
				base[N - freq & N - 1].re += 0.5;
			} else
			{
				freq = j * finc + fbias & (N - 1);
				for (i = 0; i < N; i++)
				{
					base[i].re = cos((int64_t)freq * i % N / (double)N * M_2PI);
					base[i].im = sin((int64_t)freq * i % N / (double)N * M_2PI);
				}
			}
		}

		usleep(1); // Yield to OS
		t[0] = Microseconds();
		gpu_fft_execute(fft); // call one or many times
		t[1] = Microseconds();
		if (k)
			t[2] += t[1] - t[0];

		if (perf)
		{	unsigned counters[16][2];
			i = gpu_fft_pct_read(&fft->base, counters);
			//printf("PCTE = %x\n", i);
			//for (i = 0; i < 16; ++i)
			//	printf("PCT %u = %u\n", counters[i][1], counters[i][0]);
			printf(
				"GPU load\t%5.2f %%\n"
				"GPU exec instr\t%5.2f %%\n"
				"TMU stall\t%5.2f %%\n"
				"Inst cache miss\t%5.2f %%\t%7u\n"
				"Unif cache miss\t%5.2f %%\t%7u\n"
				"TMU cache miss\t%5.2f %%\t%7u\n"
				"L2C cache miss\t%5.2f %%\t%7u\n",
				100.*counters[1][0]/(counters[0][0]+counters[1][0]),
				100.*counters[2][0]/counters[1][0],
				100.*counters[3][0]/counters[1][0],
				100.*counters[7][0]/(counters[6][0]+counters[7][0]), counters[7][0],
				100.*counters[9][0]/(counters[8][0]+counters[9][0]), counters[9][0],
				100.*counters[11][0]/counters[10][0], counters[11][0],
				100.*counters[15][0]/(counters[14][0]+counters[15][0]), counters[15][0] );
		}

		tsq[2] = tsq[3] = 0;
		bad = 0;
		for (j = 0; j < jobs; j++)
		{
			tsq[0] = tsq[1] = 0;
			base = fft->out + j * fft->step; // output buffer
			if (dir == GPU_FFT_REV)
			{
				freq = j * finc + fbias & (N - 1);
				//fprintf(stderr, "%i %i\n", fft->in - fft->out, fft->step);
				for (i = 0; i < N; i++)
				{
					double amp = cos((int64_t)freq * i % N / (double)N * M_2PI);
					tsq[0] += pow(amp, 2);
					tsq[1] += amp = pow(amp - base[i].re, 2) + pow(base[i].im, 2);
					if (debug)
						//fprintf(stderr, "%10g\t%10g\t%10g\t%10g\t%10g\t%10g\n", base[i].re, base[i].im, fft->out[i-fft->step].re, fft->out[i-fft->step].im, fft->out[i-2*fft->step].re, fft->out[i-2*fft->step].im);
						printf("%10g\t%10g\t%10g\t%10g\t%10g\n", chop(base[i].re), chop(base[i].im), chop(fft->in[i].re), chop(fft->in[i].im), chop(amp));
				}
			} else
			{
				freq = j * finc + fbias & (N - 1);
				tsq[0] += 2 * N;
				for (i = 0; i < N; i++)
				{
					double amp = (i == freq) * N;
					amp = pow(amp - base[i].re, 2) + pow(base[i].im, 2);
					tsq[1] += amp;
					if (debug)
						printf("%10g\t%10g\t%10g\t%10g\t%10g\n", chop(base[i].re), chop(base[i].im), chop(fft->in[i].re), chop(fft->in[i].im), chop(amp));
						//fprintf(stderr, "%10g\t%10g\t%10g\t%10g\t%10g\t%10g\t%10g\n", base[i].re, base[i].im, fft->out[i-fft->step].re, fft->out[i-fft->step].im, fft->out[i-2*fft->step].re, fft->out[i-2*fft->step].im, amp);
				}
			}
			//fprintf(stderr, "step_rms_err = %.5e, j = %d\n", sqrt(tsq[1] / tsq[0]), j);
			tsq[2] += tsq[0];
			tsq[3] += tsq[1];
			if (tsq[1] / tsq[0] > 2E-6)
				++bad;
		}

		printf("rel_rms_err = %.5e, usecs = %f, k = %d\n", sqrt(tsq[3] / tsq[2]), (double)(t[1] - t[0]) / jobs, k);
		if (bad)
			printf("failed: %i = %f %%\n", bad, 100. * bad / jobs);
	}

	gpu_fft_release(fft); // Videocore memory lost if not freed !

	if (loops > 1)
		printf("average: usecs = %f\n", (double)t[2] / jobs / (loops - 1));
	return 0;
}
