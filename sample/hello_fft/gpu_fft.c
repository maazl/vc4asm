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

#include <string.h>
#include <stdio.h>

#include "gpu_fft.h"

#define GPU_FFT_BUSY_WAIT_LIMIT (5<<12) // ~1ms
/* (MM) Parameters to force V3D L2C invalidation by forcing collisions. */
#define GPU_FFT_MIN_BUFF_SIZE (1<<14)
#define GPU_FFT_MAX_BUFF_COUNT 4

typedef struct GPU_FFT_COMPLEX COMPLEX;

int gpu_fft_prepare(
    int mb,         // mailbox file_desc
    int log2_N,     // log2(FFT_length) = 8...22
    int direction,  // GPU_FFT_FWD: fft(); GPU_FFT_REV: ifft()
    int jobs,       // number of transforms in batch
    struct GPU_FFT **fft) {

    unsigned info_bytes, twid_bytes, fft_bytes, data_bytes,
             fft_bytes_4k, buff_bytes, num_buff,
             code_bytes, unif_bytes, mail_bytes;
    unsigned size, *uptr, vc_tw, vc_buff, vc_data;
    int i, j, q, shared, unique, passes, ret;

    struct GPU_FFT_PTR ptr;
    struct GPU_FFT *info;

    if (gpu_fft_twiddle_size(log2_N, &shared, &unique, &passes)) return -2;

    info_bytes = 4096;
    fft_bytes  = sizeof(COMPLEX)<<log2_N;
    code_bytes = gpu_fft_shader_size(log2_N);
    twid_bytes = sizeof(COMPLEX)*16*(shared+GPU_FFT_QPUS*unique);
    unif_bytes = sizeof(int)*GPU_FFT_QPUS*(4+2*jobs*passes);
    mail_bytes = sizeof(int)*GPU_FFT_QPUS*2;

    // (MM) Use as few temporary buffers as possible
    if (fft_bytes >= GPU_FFT_MIN_BUFF_SIZE)
        // FFT size larger than cache => use exactly one buffer
        num_buff = 1;
    else
    {   // use up to GPU_FFT_TEMP_BUFF_SIZE bytes buffer, but no more than needed
        num_buff = GPU_FFT_MIN_BUFF_SIZE / fft_bytes;
        i = (passes-1) * jobs;
        if (i > GPU_FFT_MAX_BUFF_COUNT)
            i = GPU_FFT_MAX_BUFF_COUNT;
        if (num_buff > i)
            num_buff = i;
    }
    // Missing 4k alignment for 256 points
    fft_bytes_4k = ((fft_bytes-1)|4095)+1;
    buff_bytes = fft_bytes_4k * num_buff;
    data_bytes = fft_bytes * jobs;

    size  = info_bytes +  // header
            buff_bytes +  // additional buffers
            data_bytes +  // source data
            code_bytes +  // shader, aligned
            twid_bytes +  // twiddles
            unif_bytes +  // uniforms
            mail_bytes;   // mailbox message

    //fprintf(stderr, \
        "fft_bytes = %x\n" \
        "code_bytes = %x\n" \
        "twid_bytes = %x\n" \
        "unif_bytes = %x\n" \
        "mail_bytes = %x\n" \
        "buff_bytes = %x\n" \
        "data_bytes = %x\n" \
        "num_buff = %x\n" \
        "size = %x\n" \
        "fft_bytes_4k = %x\n", \
        fft_bytes, code_bytes, twid_bytes, unif_bytes, mail_bytes, buff_bytes, data_bytes, num_buff, size, fft_bytes_4k);
    ret = gpu_fft_alloc(mb, size, &ptr);
    //fprintf(stderr, "rc = %i, vc = %x, arm = %x\n", ret, ptr.vc, ptr.arm.vptr);
    if (ret) return ret;

    // Header
    info = (struct GPU_FFT *) ptr.arm.vptr;
    gpu_fft_ptr_inc(&ptr, info_bytes);

    // For transpose
    info->x = 1<<log2_N;
    info->y = jobs;

    // (MM) Use dedicated temporary buffers so all transforms can operate in place,
    // except if we only have 1 buffer.
    info->out = ptr.arm.cptr;
    vc_buff = gpu_fft_ptr_inc(&ptr, buff_bytes);
    if (log2_N <= 8)
        vc_buff += 256 * sizeof(COMPLEX);
    info->step = fft_bytes / sizeof(COMPLEX);
    info->in = ptr.arm.cptr;
    if (num_buff > 1 || !(passes & 1))
        info->out = info->in;
    vc_data = gpu_fft_ptr_inc(&ptr, data_bytes);
    //fprintf(stderr, \
        "vc_buff = %x\n" \
        "vc_data = %x\n" \
        "in = %x\n" \
        "out = %x\n", \
        vc_buff, vc_data, info->in, info->out);

    // Shader code
    memcpy(ptr.arm.vptr, gpu_fft_shader_code(log2_N), code_bytes);
    info->base.vc_code = gpu_fft_ptr_inc(&ptr, code_bytes);

    // Twiddles
    gpu_fft_twiddle_data(log2_N, direction, ptr.arm.fptr);
    vc_tw = gpu_fft_ptr_inc(&ptr, twid_bytes);

    uptr = ptr.arm.uptr;

    // Uniforms
    for (q=0; q<GPU_FFT_QPUS; q++) {
        int current_buff = num_buff-1;
        *uptr++ = vc_tw;
        *uptr++ = vc_tw + sizeof(COMPLEX)*16*(shared + q*unique);
        *uptr++ = q;
        for (i=0; i < jobs; ++i) {
            unsigned data = vc_data + i * fft_bytes;
            *uptr++ = data;
            if (num_buff == 1)
            {   // use ping pong buffers
                unsigned buff = vc_buff + current_buff * fft_bytes_4k;
                for (j = 1; j < passes; j++)
                {   uptr[0] = uptr[1] = buff;
                    // swap buffers
                    buff = data;
                    data = uptr[0];
                    uptr += 2;
                }
                if (passes & 1)
                    ++current_buff;
                *uptr++ = buff;
            } else
            {   // use dedicated buffers
                for (j = 1; j < passes; j++)
                {   // round robin
                    current_buff = (current_buff+1) % num_buff;
                    uptr[0] = uptr[1] = vc_buff + current_buff * fft_bytes_4k;
                    uptr += 2;
                }
                *uptr++ = data;
            }
        }
        *uptr++ = 0;
        //if (q == 0) for (i = -(4+2*jobs*passes); i < 0; ++i) fprintf(stderr, "%x\n", uptr[i]);
        info->base.vc_unifs[q] = gpu_fft_ptr_inc(&ptr, sizeof(int)*(4+2*jobs*passes));
    }

    if (info->base.peri && (jobs<<log2_N) <= GPU_FFT_BUSY_WAIT_LIMIT) {
        // Direct register poking with busy wait
        info->base.vc_msg = 0;
    }
    else {
        // Mailbox message
        for (q=0; q<GPU_FFT_QPUS; q++) {
            *uptr++ = info->base.vc_unifs[q];
            *uptr++ = info->base.vc_code;
        }

        info->base.vc_msg = ptr.vc;
    }

	*fft = info;
	return 0;
}

unsigned gpu_fft_execute(struct GPU_FFT *info) {
    return gpu_fft_base_exec(&info->base, GPU_FFT_QPUS);
}

void gpu_fft_release(struct GPU_FFT *info) {
    gpu_fft_base_release(&info->base);
}
