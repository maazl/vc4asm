/*
Copyright (c) 2012, Broadcom Europe Ltd.
Copyright (c) 2019, Marcel Müller
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


Replacement for mailbox.h of hello_fft.
This version of the file switches to the vcio2 driver when available
that does not require root access.
Otherwise the old vcio driver is used. In this case you need to run as root.
Remember to change the call to mapmem and pass the additional file_desc parameter.
*/

#include <stdint.h>

int mbox_open();
void mbox_close(int file_desc);

uint32_t mem_alloc(int file_desc, uint32_t size, uint32_t align, uint32_t flags);
uint32_t mem_free(int file_desc, uint32_t handle);
uint32_t mem_lock(int file_desc, uint32_t handle);
uint32_t mem_unlock(int file_desc, uint32_t handle);
void *mapmem(int file_desc, uint32_t base, uint32_t size);
void unmapmem(void *addr, uint32_t size);

uint32_t execute_qpu(int file_desc, uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout);
uint32_t qpu_enable(int file_desc, uint32_t enable);
