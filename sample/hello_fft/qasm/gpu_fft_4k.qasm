# BCM2835 "GPU_FFT"
#
# Copyright (c) 2013, Andrew Holme.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the copyright holder nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.set STAGES, 12

.include "gpu_fft.qinc"

##############################################################################
# Twiddles

.set TW_SHARED,     3
.set TW_UNIQUE,     1

.set TW16_P1_BASE,  0
.set TW16_P2_BASE,  TW16_P1_BASE
.set TW16_P2_STEP,  1
.set TW16_P3_STEP,  2

.set TW16_P3_BASE,  3

.set TW16_ACTIVE,   TW_SHARED+TW_UNIQUE

##############################################################################
# Registers

.set ra_link_0,         ra0
#                       rb0
.set ra_save_ptr,       ra1
#                       rb1
.set ra_temp,           ra2
.set rx_vpm,            rb2
.set ra_addr_x,         ra3
.set rb_addr_y,         rb3
.set ra_save_16,        ra4
#
.set ra_load_idx,       ra5
#                       rb5
.set ra_sync,           ra6
#
.set ra_points,         ra7
#                       rb7
.set ra_link_1,         ra8
.set rb_link_1,         rb8

.set ra_tw_re,          ra9
.set rb_tw_im,          rb9

#                       ra26
#                       rb26
.set ra_vdw,            ra27
.set rb_vdw,            rb27

.set rx_0x5555,         ra28
.set rx_0x3333,         ra29
.set rx_0x0F0F,         ra30
.set rx_inst,           ra31

.set rb_pass2_link,     rb29
#                       rb30
.set rb_0x80,           rb31

##############################################################################
# Redefine compile time constants

# (MM) Optimized: extracted stride from load_xxx to make read part of a procedure
# Redo this for the 4k FFT
.set DEF_STRIDE,        rb_0x80

##############################################################################
# Constants

mov rb_0x80,    0x80

mov rx_0x5555,  0x5555
mov rx_0x3333,  0x3333
mov rx_0x0F0F,  0x0F0F

mov ra_vdw, vdw_setup_0(16, 16, dma_h32( 0,0))
# (MM) Optimized: use xor for vpm swap (less memory I/O, save A register)
mov rb_vdw, vdw_setup_0(16, 16, dma_h32(16,0)) - vdw_setup_0(16, 16, dma_h32( 0,0))

##############################################################################
# Load twiddle factors

load_tw rb_0x80,         0, TW_SHARED, unif
load_tw rb_0x80, TW_SHARED, TW_UNIQUE, unif

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
# Saves several branch instructions and 2 registers
    mov r3, unif;         mov ra_save_16, 0
    shl.setf r0, r3, 5;   mov ra_sync, 0
    mov.ifnz r1, :sync_slave - :sync - 4*8 # -> rx_inst-1
    add.ifnz ra_sync, r1, r0
    mov.ifnz r1, :save_slave - :save_16
    mov.ifnz ra_save_16, r1;

# (MM) Optimized: reduced VPM registers to 1
inst_vpm r3, rx_vpm

    ;mov rx_inst, r3

##############################################################################
# Macros

# (MM) no longer used
.macro swap_vpm_vdw
    # (MM) Optimized: use xor for vpm swap (less memory I/O) and save A register
    xor ra_vdw, ra_vdw, rb_vdw;  mov r2, vpm_setup(1, 1, v32(16,0)) - vpm_setup(1, 1, v32(0,0))
    xor ra_vpm, ra_vpm, r2;
.endm

.macro swizzle
    mov.setf  -, [0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0]
    mov r2, r0; mov.ifnz r0, r0 << 6
    mov r3, r1; mov.ifnz r1, r1 << 6
    mov.setf  -, [0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0]
    nop; mov.ifnz r0, r2 >> 6
    nop; mov.ifnz r1, r3 >> 6
.endm

.macro next_twiddles, tw16
    next_twiddles_16 tw16
.endm

.macro init_stage, tw16
    init_stage_16 tw16, 4
.endm

##############################################################################
# Top level

:loop
    mov.setf ra_addr_x, unif  # Ping buffer or null
    # (MM) Optimized: branch sooner
    brr.allz -, r:end
    mov      rb_addr_y, unif; # Pong buffer or IRQ enable

##############################################################################
# Pass 1

    init_stage TW16_P1_BASE
    read_rev rb_0x80

    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3
    .back 2
    # (MM) Optimized: extracted stride from load_xxx to make read part of a procedure
    # This is basically the concept of the 4k FFT, so we call the FFT procedure directly.
    brr ra_link_1, r:load_fft_16_rev
    .endb
    mov ra_points, (1<<STAGES) / 0x80 - 1

:   # start of hidden loop
    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3

    # (MM) Optimized: branch unconditional and patch the return address
    # for the last turn.
    brr r0, r:load_fft_16_rev + 8
    sub.setf ra_points, ra_points, 1
    mov.ifz ra_link_1, r0
    .clone r:load_fft_16_rev, 1

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 2

    swap_buffers
    init_stage TW16_P2_BASE
    read_lin rb_0x80

    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3

    .back 2
    # (MM) Optimized: keep return address additionally in rb_link_1 for loop.
    # (MM) Optimized: extracted stride from load_xxx to make read part of a procedure
    # This is basically the concept of the 4k FFT, so we call the FFT procedure directly.
    brr ra_link_1, rb_link_1, -, r:load_fft_16_lin
    .endb
    mov ra_points, (1<<STAGES) / 0x80 - 1

:   # start of hidden loop
    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3

    # (MM) Optimized: patch the return address for the last turn to save the
    # conditional branch and the unecessary twiddle load after the last turn.
    brr ra_link_1, r:load_fft_16_lin + 8
    sub.setf ra_points, ra_points, 2
    mov.ifn ra_link_1, rb_pass2_link
    .clone r:load_fft_16_lin, 1
:2
    next_twiddles TW16_P2_STEP

    ;mov ra_link_1, rb_link_1
    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3
    .back 3
    brr rb_pass2_link, r:load_fft_16_lin
    .endb
:3
    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 3

    swap_buffers
    init_stage TW16_P3_BASE
    read_lin rb_0x80

    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3

    .back 2
    # (MM) Optimized: extracted stride from load_xxx to make read part of a procedure
    # This is basically the concept of the 4k FFT, so we call the FFT procedure directly.
    brr ra_link_1, r:load_fft_16_lin
    .endb
    mov ra_points, (1<<STAGES) / 0x80 - 1

:   # start of hidden loop
    next_twiddles TW16_P3_STEP
    # (MM) Optimized: move swap_vpm_vdw to pass1/2/3

    # (MM) Optimized: branch unconditional and patch the return address for
    # the last turn, move the branch before the last instruction of swap_vpm_vdw.
    .back 1
    brr r0, r:load_fft_16_lin
    .endb
    sub.setf ra_points, ra_points, 1
    mov.ifz ra_link_1, r0

    # (MM) Optimized: easier procedure chains
    brr r0, r:sync, ra_sync
    # (MM) Optimized: redirect ra_link_1 to :loop to save branch and 3 nop.
    mov r1, :loop - :1f
    add ra_link_1, r0, r1; ldtmu0
    nop;                   ldtmu0
:1

##############################################################################

:end
    exit rb_addr_y

# (MM) Optimized: easier procedure chains
##############################################################################
# Master/slave procedures

##############################################################################
# Subroutines

# (MM) Optimized: joined load_xxx and ldtmu in FFT-16 codelet
bodies_fft_16

    # (MM) Optimized: move swap_vpm_vdw to pass1/2
    # (MM) Optimized: use xor for vpm swap (less memory I/O, save A register)
    ;mov r2, vpm_setup(1, 1, v32(16,0)) - vpm_setup(1, 1, v32(0,0))
   
    # (MM) Optimized: move write_vpm_16 to body_pass_16
    # and expand inline to pack with VPM swap code, saves 2 instructions
    xor vw_setup, rx_vpm, r2;      mov r3, rx_vpm
    xor rx_vpm,   r3,     r2;      mov vpm, r0
    xor ra_vdw,   ra_vdw, rb_vdw;  mov vpm, r1

    # (MM) Optimized: link directly to save_16
    .back 3
    brr -, ra_save_16, r:save_16
    .endb

:save_16
    body_ra_save_16 ra_vdw

:save_slave
    body_rx_save_slave

:sync_slave
    body_rx_sync_slave

:sync
    body_ra_sync

