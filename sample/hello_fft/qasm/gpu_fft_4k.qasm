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
#                       rb2
.set ra_addr_x,         ra3
.set rb_addr_y,         rb3
.set ra_save_16,        ra4
#
.set ra_load_idx,       ra5
.set rb_inst,           rb5
.set ra_sync,           ra6
#
.set ra_points,         ra7
#                       rb7
.set ra_link_1,         ra8

.set ra_tw_re,          ra9
.set rb_tw_im,          rb9

.set ra_vpm,            ra26
.set rb_vpm,            rb26
.set ra_vdw,            ra27
.set rb_vdw,            rb27

.set rx_0x5555,         ra28
.set rx_0x3333,         ra29
.set rx_0x0F0F,         ra30
.set rx_0x00FF,         ra31

.set rb_0x20,           rb29
.set rb_0x40,           rb30
.set rb_0x80,           rb31

##############################################################################
# Constants

mov rb_0x20,    0x20
mov rb_0x40,    0x40
mov rb_0x80,    0x80

mov rx_0x5555,  0x5555
mov rx_0x3333,  0x3333
mov rx_0x0F0F,  0x0F0F
mov rx_0x00FF,  0x00FF

mov ra_vdw, vdw_setup_0(16, 16, dma_h32( 0,0))
mov rb_vdw, vdw_setup_0(16, 16, dma_h32(16,0))

##############################################################################
# Load twiddle factors

load_tw rb_0x80,         0, TW_SHARED, unif
load_tw rb_0x80, TW_SHARED, TW_UNIQUE, unif

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
# Saves several branch instructions and 2 registers
    sub.setf r0, unif, 1; mov r3, unif
    # get physical address for ra_link_0
    brr r2, 0
    mov r1, :save_16 - :0f
    add r2, r2, r1;  mov ra_save_16, 0
    shl r0, r0, 5;   mov ra_sync, 0
:0  mov r1, :sync_slave - :sync
    mov.ifnn ra_save_16,  :save_slave_16 - :save_16
    add.ifnn ra_sync, r1, r0
    # absolute and relative address of save_16
    add ra_link_0, r2, ra_save_16; mov rb_inst, r3

inst_vpm r3, ra_vpm, rb_vpm, -, -

##############################################################################
# Macros

.macro swap_vpm_vdw
    mov ra_vpm, rb_vpm; mov rb_vpm, ra_vpm
    mov ra_vdw, rb_vdw; mov rb_vdw, ra_vdw
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
    mov.setf ra_addr_x, unif # Ping buffer or null
    # (MM) Optimized: branch sooner
    brr.allz -, r:end
    mov      rb_addr_y, unif # Pong buffer or IRQ enable

##############################################################################
# Pass 1

    init_stage TW16_P1_BASE
    read_rev rb_0x80

        brr ra_link_1, r:pass_1
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80

        shr.setf -, ra_points, STAGES

        brr.allz -, r:pass_1
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    nop
    ldtmu0
    ldtmu0

##############################################################################
# Pass 2

    swap_buffers
    init_stage TW16_P2_BASE
    read_lin rb_0x80

    .rep i, 2
        brr ra_link_1, r:pass_2
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80
    .endr

        sub ra_link_1, ra_link_1, rb_0x20
        next_twiddles TW16_P2_STEP
        shr.setf -, ra_points, STAGES

        brr.allz -, r:pass_2
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    nop
    ldtmu0
    ldtmu0

##############################################################################
# Pass 3

    swap_buffers
    init_stage TW16_P3_BASE
    read_lin rb_0x80

        brr ra_link_1, r:pass_3
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80

        next_twiddles TW16_P3_STEP
        shr.setf -, ra_points, STAGES

        brr.allz -, r:pass_3
        swap_vpm_vdw
        add ra_points, ra_points, rb_0x80

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

:save_16
    body_ra_save_16 ra_vpm, ra_vdw

:save_slave_16
    body_rx_save_slave_16 ra_vpm

:sync
    body_ra_sync

:sync_slave
    body_rx_sync_slave

##############################################################################
# Subroutines

:fft_16
    body_fft_16

:pass_1
    read_rev rb_0x80
    nop;        ldtmu0
    mov r0, r4; ldtmu0
    mov r1, r4
    swizzle
    brr -, r:fft_16
    interleave

:pass_2
:pass_3
    read_lin rb_0x80
    brr -, r:fft_16
    nop;        ldtmu0
    mov r0, r4; ldtmu0
    mov r1, r4

