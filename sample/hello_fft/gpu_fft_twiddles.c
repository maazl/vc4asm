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

#include <math.h>

#include "gpu_fft.h"

#define ALPHA(dx) (2*pow(sin((dx)/2),2))
#define  BETA(dx) (sin(dx))

static const double k[16] = {0,8,4,4,2,2,2,2,1,1,1,1,1,1,1,1};
/* (MM) Optimized: multiply m[] by k[] */
static const double m[16] = {0,0,0,4,0,2,4,6,0,1,2,3,4,5,6,7};

/****************************************************************************/

static float *twiddles_base_16(double two_pi, float *out, double theta) {
    int i;
    const double f = two_pi / 16;
    for (i=0; i<16; i++) {
        *out++ = cos(f*m[i] + theta*k[i]);
        *out++ = sin(f*m[i] + theta*k[i]);
    }
    return out;
}

static float *twiddles_base_32(double two_pi, float *out, double theta) {
    int i;
    const double f = two_pi / 32;
    for (i=0; i<16; i++) {
        *out++ = cos(f*i + theta);
        *out++ = sin(f*i + theta);
    }
    return twiddles_base_16(two_pi, out, 2*theta);
}

static float *twiddles_base_64(double two_pi, float *out, double theta) {
    int i;
    const double f = two_pi / 64;
    for (i=0; i<32; i++) {
        *out++ = cos(f*i);
        *out++ = sin(f*i);
    }
    return twiddles_base_32(two_pi, out, 0);
}

/****************************************************************************/

static float *twiddles_step_16(double two_pi, float *out, double theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = ALPHA(theta*k[i]);
        *out++ =  BETA(theta*k[i]);
    }
    return out;
}

static float *twiddles_step_32(double two_pi, float *out, double theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = ALPHA(theta);
        *out++ =  BETA(theta);
    }
    return twiddles_step_16(two_pi, out, 2*theta);
}

static float *twiddles_step_64(double two_pi, float *out, double theta) {
    int i;
    for (i=0; i<32; i++) {
        *out++ = ALPHA(theta);
        *out++ =  BETA(theta);
    }
    return twiddles_step_32(two_pi, out, 2*theta);
}

/****************************************************************************/

/* (MM) simpler setup */
static const int stages[][5] = {
	{4,4},
	{5,4},
	{5,5},
	{6,5},
	{4,4,4},
	{5,4,4},
	{5,5,4},
	{5,5,5},
	{6,5,5},
	{5,4,4,4},
	{5,4,4,5},
	{5,4,5,5},
	{5,5,5,5},
	{6,5,5,5},
	{6,6,5,5}
};

int gpu_fft_twiddle_size(int log2_N, int *shared, int *unique, int *passes) {
	const int* stage;
	if (log2_N<8 || log2_N>22) return -1;
	*shared = *passes = 0;
	stage = stages[log2_N-8];
	do
	{	++*passes;
		*shared += 1 << (*stage - 4);
	} while (*++stage);
	*unique = 1 << (stage[-1] - 4);
  return 0;
}

static float* (*const base_procs[3])(double two_pi, float *out, double theta) =
{	twiddles_base_16,
	twiddles_base_32,
	twiddles_base_64
};

static float* (*const step_procs[3])(double two_pi, float *out, double theta) =
{	twiddles_step_16,
	twiddles_step_32,
	twiddles_step_64
};

void gpu_fft_twiddle_data(int log2_N, int direction, float *out) {
	double two_pi = direction == GPU_FFT_FWD ? -2*GPU_FFT_PI : 2*GPU_FFT_PI;
	double two_pi_N = two_pi / (1 << log2_N);
	const int* stage = stages[log2_N-8];
	int q;
	/* shared */
	out = base_procs[*stage++ - 4](two_pi, out, 0);
	do
	{	const int* next = stage;
		if (!*++next)
		{	/* last stage */
			q = GPU_FFT_QPUS;
		} else
		{	q = 1 << *next;
			while (*++next)
				q *= 1 << *next;
		}
		out = step_procs[*stage - 4](two_pi, out, two_pi_N*q);
	} while (*++stage);
	/* unique */
	for (q=0; q<GPU_FFT_QPUS; q++)
		out = base_procs[stage[-1] - 4](two_pi, out, two_pi_N*q);
}
