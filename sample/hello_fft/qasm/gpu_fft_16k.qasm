# BCM2835 "GPU_FFT" release 3.0
#
# Copyright (c) 2015, Andrew Holme.
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

.set STAGES, 14

.include "gpu_fft.qinc"

##############################################################################
# Twiddles: src

.set TW32_BASE,     0   # rx_tw_shared
.set TW16_BASE,     1
.set TW32_P2_STEP,  2
.set TW16_P2_STEP,  3
.set TW16_P3_STEP,  4

.set TW16_P3_BASE,  0   # rx_tw_unique

##############################################################################
# Twiddles: dst

.set TW16_STEP, 0  # 1
.set TW32_STEP, 1  # 1
.set TW16,      2  # 5
.set TW32,      7  # 2

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
#
.set ra_points,         ra7
#                       rb7
.set ra_link_1,         ra8
.set rb_link_1,         rb8
.set ra_32_re,          ra9
.set rb_32_im,          rb9
.set ra_save_32,        ra10
#                       rb10

.set rx_tw_shared,      ra11
.set rx_tw_unique,      rb11

.set ra_tw_re,          ra12 # 9
.set rb_tw_im,          rb12 # 9
.set ra_vdw_16,         ra27
.set ra_vdw_32,         ra28

.set rx_0x5555,         rb26
.set rx_0x3333,         rb27
.set rx_0x0F0F,         rb28
.set rb_0x80,           rb29
.set rb_0xF0,           rb30
.set rb_pass2_link,     rb31

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
# Twiddles: ptr

mov rx_tw_shared, unif
mov rx_tw_unique, unif

##############################################################################
# Instance

# (MM) Optimized: better procedure chains
# Saves several branch instructions and 4 registers
    mov r3, unif;         mov ra_sync, 0
    shl.setf r0, r3, 5;   mov ra_save_32, 0
    mov.ifnz r1, :sync_slave - :sync - 4*8 # -> rx_inst-1
    add.ifnz ra_sync, r1, r0
    mov.ifnz r1, :save_slave - :save_32
    mov.ifnz ra_save_32, r1;

# (MM) Optimized: reduced VPM registers to 1
inst_vpm r3, rx_vpm

    ;mov rx_inst, r3

##############################################################################
# Top level

:loop
    mov.setf ra_addr_x, unif  # Ping buffer or null
    # (MM) Optimized: branch sooner
    brr.allz -, r:end
    mov      rb_addr_y, unif; # Pong buffer or IRQ enable

##############################################################################
# Pass 1

    load_tw rx_tw_shared, TW16+3, TW16_BASE
    load_tw rx_tw_shared, TW32+0, TW32_BASE
    init_stage 5
    read_rev 0x10

    # (MM) Optimized: place branch before the last two instructions of read_rev
    .back 2
    brr ra_link_1, r:pass_1
    .endb
    mov ra_points, (1<<STAGES) / 0x100 - 1

:   # start of hidden loop
    # (MM) Optimized: branch unconditional and patch the return address
    # for the last turn.
    brr r0, r:pass_1
    sub.setf ra_points, ra_points, 1
    mov.ifz ra_link_1, r0
    nop

    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 2

    swap_buffers
    load_tw rx_tw_shared, TW16+3, TW16_BASE
    load_tw rx_tw_shared, TW32+0, TW32_BASE
    load_tw rx_tw_shared, TW16_STEP, TW16_P2_STEP
    load_tw rx_tw_shared, TW32_STEP, TW32_P2_STEP
    init_stage 5
    read_lin 0x10

    # (MM) Optimized: place branch before the last instruction of read_lin
    # and keep return address additionally in rb_link_1 for loop.
    .back 2
    brr ra_link_1, rb_link_1, -, r:pass_2
    .endb
    mov ra_points, (1<<STAGES) / 0x100 - 1

:   # start of hidden loop
    # (MM) Optimized: patch the return address for the last turn to save the
    # conditional branch and the unecessary twiddle load after the last turn.
    brr ra_link_1, r:pass_2
    sub.setf ra_points, ra_points, 2
    mov.ifn ra_link_1, rb_pass2_link
    nop
:2
    next_twiddles_32
    next_twiddles_16

    # (MM) Optimized: place branch before the last two instructions of next_twiddles
    .back 2
    brr rb_pass2_link, r:pass_2
    .endb
    mov ra_link_1, rb_link_1
:3
    # (MM) Optimized: easier procedure chains
    brr ra_link_1, r:sync, ra_sync
    ldtmu0
    nop
    ldtmu0

##############################################################################
# Pass 3

    swap_buffers
    load_tw rx_tw_unique, TW16+3, TW16_P3_BASE
    load_tw rx_tw_shared, TW16_STEP, TW16_P3_STEP
    init_stage 4
    read_lin rb_0x80

    # (MM) Optimized: place branch before the last two instructions of read_lin
    .back 2
    brr ra_link_1, r:pass_3
    .endb
    mov ra_points, (1<<STAGES) / 0x80 - 1

:   # start of hidden loop
    next_twiddles_16

    # (MM) Optimized: place the branch before the last instruction of next_twiddles
    # and branch unconditional and patch the return address of the last turn.
    .back 1
    brr r0, r:pass_3
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

    .back 3
    brr -, ra_save_32, r:save_32
    .endb

:save_32
    body_ra_save_32

:save_slave
    body_rx_save_slave

:sync_slave
    body_rx_sync_slave

:sync
    body_ra_sync

:pass_2
    body_pass_32 LOAD_STRAIGHT

    .back 3
    brr -, ra_save_32, r:save_32
    .endb

:pass_3
    body_pass_16 LOAD_STRAIGHT

    # (MM) Optimized: link to slave procedure without need for a register
    .back 3
    ;mov.setf -, rx_inst
    brr.allnz -, r:save_slave
    .endb

#:save_16
    body_ra_save_16 ra_vdw_16

