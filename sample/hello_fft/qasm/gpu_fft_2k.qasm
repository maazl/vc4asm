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

.set STAGES, 11

.include "gpu_fft.qinc"

##############################################################################
# Twiddles

.set TW_SHARED,     6
.set TW_UNIQUE,     2

.set TW64_P1_BASE0, 0
.set TW64_P1_BASE1, 1
.set TW32_P1_BASE,  2
.set TW16_P1_BASE,  3
.set TW32_P2_STEP,  4
.set TW16_P2_STEP,  5

.set TW32_P2_BASE,  6
.set TW16_P2_BASE,  7

.set TW32_ACTIVE,   TW_SHARED+TW_UNIQUE
.set TW16_ACTIVE,   TW_SHARED+TW_UNIQUE+1

##############################################################################
# Registers

.set ra_link_0,         ra0
.set rb_vpm,            rb0
.set ra_save_ptr,       ra1
.set rb_vpm_16,         rb1
.set ra_temp,           ra2
.set rb_vpm_32,         rb2
.set ra_addr_x,         ra3
.set rb_addr_y,         rb3
.set ra_save_32,        ra4
#
.set ra_load_idx,       ra5
.set rb_inst,           rb5
.set ra_sync,           ra6
#
.set ra_points,         ra7
.set rb_vpm_48,         rb7
.set ra_link_1,         ra8
#                       rb8
.set ra_32_re,          ra9
.set rb_32_im,          rb9
.set ra_save_64,        ra10
#

.set ra_64,             ra11 # 4
.set rb_64,             rb11 # 4

.set ra_tw_re,          ra15 # 13
.set rb_tw_im,          rb15 # 13

.set rx_0x5555,         ra28
.set rx_0x3333,         ra29
.set rx_0x0F0F,         ra30
.set rx_0x00FF,         ra31

.set rb_0x10,           rb28
.set rb_0x40,           rb29
.set rb_0xF0,           rb30
.set rb_0x1D0,          rb31

##############################################################################
# Dual-use registers

.set ra_vpm_lo,         ra_64+0
.set ra_vpm_hi,         ra_64+1
.set rb_vpm_lo,         rb_vpm_32
.set rb_vpm_hi,         rb_vpm_48
.set ra_vdw_32,         ra_64+2
.set rb_vdw_32,         rb_64+2

##############################################################################
# Constants

mov rb_0x10,    0x10
mov rb_0x40,    0x40
mov rb_0xF0,    0xF0
mov rb_0x1D0,   0x1D0

mov rx_0x5555,  0x5555
mov rx_0x3333,  0x3333
mov rx_0x0F0F,  0x0F0F
mov rx_0x00FF,  0x00FF

##############################################################################
# Load twiddle factors

mov r3, 0x80
load_tw r3,         0, TW_SHARED, unif
load_tw r3, TW_SHARED, TW_UNIQUE, unif

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
# Saves several branch instructions and 3 rb registers
    sub r0, unif, 1;     mov ra_sync, 0
    add.setf r3, r0, 1;  mov ra_save_32, 0
    shl r0, r0, 5;       mov ra_save_64, 0
    mov r1,              :sync_slave - :sync
    add.ifnz ra_sync, r1, r0; mov rb_inst, r3
    mov.ifnz ra_save_32, :save_slave_32 - :save_32
    mov.ifnz ra_save_64, :save_slave_64 - :save_64
inst_vpm r3, rb_vpm, rb_vpm_16, rb_vpm_32, rb_vpm_48

##############################################################################
# Macros

.macro swizzle
.endm

.macro next_twiddles, tw16, tw32
    next_twiddles_32 tw32
    next_twiddles_16 tw16
.endm

.macro init_stage, m, tw16, tw32
    init_stage_32 tw32
    init_stage_16 tw16, m
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

    init_stage 6, TW16_P1_BASE, TW32_P1_BASE
    read_rev rb_0x10

        brr ra_link_1, r:pass_1
        nop
        mov r0, 0x200
        add ra_points, ra_points, r0

        shr.setf -, ra_points, STAGES

        brr.allz -, r:pass_1
        nop
        mov r0, 0x200
        add ra_points, ra_points, r0

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    nop
    ldtmu0
    ldtmu0

##############################################################################
# Dual-use registers

    mov ra_vpm_lo, rb_vpm
    mov ra_vpm_hi, rb_vpm_16

    mov ra_vdw_32, vdw_setup_0(32, 16, dma_h32( 0,0))
    mov rb_vdw_32, vdw_setup_0(32, 16, dma_h32(32,0))

##############################################################################
# Pass 2

    swap_buffers
    init_stage 5, TW16_P2_BASE, TW32_P2_BASE
    read_lin rb_0x10

        brr ra_link_1, r:pass_2
        nop
        mov r0, 0x100
        add ra_points, ra_points, r0

        next_twiddles TW16_P2_STEP, TW32_P2_STEP

        shr.setf -, ra_points, STAGES

        brr.allz -, r:pass_2
        nop
        mov r0, 0x100
        add ra_points, ra_points, r0

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

:save_32
    body_ra_save_32

:save_slave_32
    body_rx_save_slave_32

:save_64
    body_ra_save_64 rb_0x40

:save_slave_64
    body_rx_save_slave_64

:sync
    body_ra_sync

:sync_slave
    body_rx_sync_slave

##############################################################################
# Subroutines

:fft_16
    body_fft_16

:pass_1
    body_pass_64 LOAD_REVERSED, rb_0x1D0

:pass_2
    body_pass_32 LOAD_STRAIGHT

