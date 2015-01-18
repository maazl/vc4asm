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

.set STAGES, 9

.include "gpu_fft.qinc"

##############################################################################
# Twiddles

.set TW_SHARED,     3
.set TW_UNIQUE,     1

.set TW32_P1_BASE,  0
.set TW16_P1_BASE,  1
.set TW16_P2_STEP,  2

.set TW16_P2_BASE,  3

.set TW32_ACTIVE,   TW_SHARED+TW_UNIQUE
.set TW16_ACTIVE,   TW_SHARED+TW_UNIQUE+1

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
.set rx_inst,           ra4
#                       rb4
.set ra_load_idx,       ra5
#                       rb5
.set ra_sync,           ra6
#                       rb6
.set ra_points,         ra7
#                       rb7
.set ra_link_1,         ra8
#                       rb8
.set ra_32_re,          ra9
.set rb_32_im,          rb9
.set ra_save_16,        ra10
#                       rb10

.set ra_tw_re,          ra11 # 9
.set rb_tw_im,          rb11 # 9

#                       ra24
#                       ra25
.set ra_vdw_16,         ra26
.set ra_vdw_32,         ra27

.set rx_0x5555,         ra28
.set rx_0x3333,         ra29
.set rx_0x0F0F,         ra30
#                       ra31

#                       rb28
#                       rb29
.set rb_0x80,           rb30
.set rb_0xF0,           rb31

##############################################################################
# Constants

mov rb_0x80,    0x80
mov rb_0xF0,    0xF0

mov rx_0x5555,  0x5555
mov rx_0x3333,  0x3333
mov rx_0x0F0F,  0x0F0F

mov ra_vdw_16, vdw_setup_0(16, 16, dma_h32( 0,0))
mov ra_vdw_32, vdw_setup_0(32, 16, dma_h32( 0,0))

##############################################################################
# Load twiddle factors

load_tw rb_0x80,         0, TW_SHARED, unif
load_tw rb_0x80, TW_SHARED, TW_UNIQUE, unif

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
# Saves several branch instructions and 5 registers
    mov.setf r3, unif;  mov ra_sync, 0
    shl r0, r3, 5;      mov ra_save_16, 0
    mov r1, :sync_slave - :sync - 4*8 # -> rx_inst-1
    add.ifnz ra_sync, r1, r0;
    mov.ifnz r1, :save_slave - :save_16
    mov.ifnz ra_save_16, r1;
    
# (MM) Optimized: reduced VPM registers to 1
inst_vpm r3, rx_vpm

    ;mov rx_inst, r3

##############################################################################
# Macros

.macro swizzle
.endm

.macro init_stage, tw16, tw32
    init_stage_32 tw32
    init_stage_16 tw16, 5
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

    init_stage TW16_P1_BASE, TW32_P1_BASE
    read_rev 0x10

    # (MM) Optimized: move branch into the read_rev code
    .back 3
    brr ra_link_1, r:pass_1
    .endb

    brr ra_link_1, r:pass_1
    nop
    nop
    nop

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    nop
    ldtmu0
    ldtmu0

##############################################################################
# Pass 2

    swap_buffers
    init_stage_16 TW16_P2_BASE, 4
    read_lin rb_0x80

    ;mov ra_points, (1<<STAGES) / 0x80 - 1
    # (MM) Optimized: move branch into the read_rev code
    .back 3
    brr ra_link_1, r:pass_2
    .endb

:   # start of hidden loop
    next_twiddles_16 TW16_P2_STEP

    # (MM) Optimized: branch unconditional and patch the return address for
    # the last turn, move the branch before the last instruction of next_twiddles.
    .back 1
    brr r0, r:pass_2
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
# Subroutines

# (MM) Optimized: joined load_xxx and ldtmu in FFT-16 codelet
bodies_fft_16
    .back 3
    bra -, ra_link_0
    .endb

:pass_1
    body_pass_32 LOAD_REVERSED

    # (MM) Optimized: link to slave procedure without need for a register
    .back 9  # place deep inside fft_twiddles
    ;mov.setf -, rx_inst;
    .endb
    .back 3
    brr.allnz -, r:save_slave
    .endb

#:save_32
    body_ra_save_32

:save_slave
    body_rx_save_slave

:sync_slave
    body_rx_sync_slave

:sync
    body_ra_sync

:pass_2
    body_pass_16 LOAD_STRAIGHT

    .back 3
    brr -, ra_save_16, r:save_16
    .endb

:save_16
    body_ra_save_16 ra_vdw_16

