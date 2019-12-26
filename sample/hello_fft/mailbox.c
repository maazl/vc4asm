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


Replacement for mailbox.c of hello_fft.
This version of the file switches to the vcio2 driver when available
that does not require root access.
Otherwise the old vcio driver is used. In this case you need to run as root.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "vcio2.h"

#include "mailbox.h"

#define PAGE_SIZE (4*1024)
#define IOCTL_MBOX_PROPERTY 0xc0046400


static char have_vcio2;


void *mapmem(int file_desc, uint32_t base, uint32_t size)
{
	void *mem;
	uint32_t offset = base % PAGE_SIZE;
	base = base - offset;

	if (!have_vcio2)
	{	file_desc = open("/dev/mem", O_RDWR|O_SYNC);
		if (file_desc < 0)
		{	printf("can't open /dev/mem\nThis program should be run as root unless /dev/vcio2 is available. Try prefixing command with: sudo\n");
			return NULL;
		}
	}

	mem = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, file_desc, base);
	#ifdef DEBUG
	printf("base=0x%x, mem=%p\n", base, mem);
	#endif

	if (!have_vcio2)
		close(file_desc);

	if (mem == MAP_FAILED) {
		//printf("mmap error: %s\n", strerror(errno));
		return NULL;
	}
	return (char *)mem + offset;
}

void unmapmem(void *addr, uint32_t size)
{
	int s = munmap(addr, size);
	if (s != 0)
	{	printf("munmap error %d\n", s);
		exit (-1);
	}
}

// use ioctl to send mbox property message for old vcio driver
static int mbox_property(int file_desc, void *buf)
{
	int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);
	if (ret_val < 0)
		printf("ioctl_set_msg failed:%d\n", ret_val);

	#ifdef DEBUG
	unsigned *p = buf; int i; unsigned size = *(unsigned *)buf;
	for (i=0; i<size/4; i++)
		printf("%04x: 0x%08x\n", i*sizeof *p, p[i]);
	#endif
	return ret_val;
}

uint32_t mem_alloc(int file_desc, uint32_t size, uint32_t align, uint32_t flags)
{
	if (have_vcio2)
	{	vcio_mem_allocate buf;
		int ret_val;

		buf.in.size = size;
		buf.in.alignment = align;
		buf.in.flags = flags;

		ret_val = ioctl(file_desc, IOCTL_MEM_ALLOCATE, &buf);
		if (ret_val)
		{	printf("mem_alloc ioctl failed: %d\n", ret_val);
			return 0;
		}
		return buf.out.handle;
	} else
	{	uint32_t p[9] =
		{	9 * sizeof(uint32_t), // size
			0x00000000,           // process request
			0x3000c,              // (the tag id)
			3 * sizeof(uint32_t), // (size of the buffer)
			3 * sizeof(uint32_t), // (size of the data)
			size,                 // (num bytes)
			align,                // (alignment)
			flags,                // (MEM_FLAG_L1_NONALLOCATING)
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

uint32_t mem_free(int file_desc, uint32_t handle)
{
	if (have_vcio2)
	{	int ret_val = ioctl(file_desc, IOCTL_MEM_RELEASE, handle);
		if (ret_val)
			printf("mem_free ioctl failed: %d\n", ret_val);
		return ret_val;
	} else
	{	uint32_t p[7] =
		{	7 * sizeof(uint32_t), // size
			0x00000000,           // process request
			0x3000f,              // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,               // (handle)
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

uint32_t mem_lock(int file_desc, uint32_t handle)
{
	if (have_vcio2)
	{	int ret_val = ioctl(file_desc, IOCTL_MEM_LOCK, &handle);
		if (ret_val)
		{	printf("mem_lock ioctl failed: %d\n", ret_val);
			return 0;
		}
		return handle;
	} else
	{	uint32_t p[7] =
		{	7 * sizeof(uint32_t), // size
			0x00000000,           // process request
			0x3000d,              // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,               // (handle)
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

uint32_t mem_unlock(int file_desc, uint32_t handle)
{
	if (have_vcio2)
	{	int ret_val = ioctl(file_desc, IOCTL_MEM_UNLOCK, handle);
		if (ret_val)
			printf("mem_unlock ioctl failed: %d\n", ret_val);
		return ret_val;
	} else
	{	uint32_t p[7] =
		{	7 * sizeof(uint32_t), // size
			0x00000000,           // process request
			0x3000e,              // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			handle,               // (handle)
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

uint32_t qpu_enable(int file_desc, uint32_t enable)
{
	if (have_vcio2)
	{	int ret_val = ioctl(file_desc, IOCTL_ENABLE_QPU, enable);
		if (ret_val)
			printf("qpu_enable ioctl failed: %d, %s\n", ret_val, strerror(errno));
		return ret_val;
	} else
	{	uint32_t p[7] =
		{	7 * sizeof(uint32_t), // size
			0x00000000,           // process request
			0x30012,              // (the tag id)
			1 * sizeof(uint32_t), // (size of the buffer)
			1 * sizeof(uint32_t), // (size of the data)
			enable,               // (enable QPU)
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

uint32_t execute_qpu(int file_desc, uint32_t num_qpus, uint32_t control, uint32_t noflush, uint32_t timeout)
{
	if (have_vcio2)
	{	vcio_exec_qpu buf;
		int ret_val;

		buf.in.num_qpus = num_qpus;
		buf.in.control = control;
		buf.in.noflush = noflush;
		buf.in.timeout = timeout;

		ret_val = ioctl(file_desc, IOCTL_EXEC_QPU, &buf);
		if (ret_val)
			printf("execute_qpu failed: %d\n", ret_val);
		return ret_val;
	} else
	{	uint32_t p[10] =
		{	10 * sizeof(uint32_t),// size
			0x00000000,           // process request
			0x30011,              // (the tag id)
			4 * sizeof(uint32_t), // (size of the buffer)
			4 * sizeof(uint32_t), // (size of the data)
			num_qpus,
			control,
			noflush,
			timeout,              // ms
			0                     // end tag
		};
		mbox_property(file_desc, p);
		return p[5];
	}
}

int mbox_open() {
	int file_desc;

	have_vcio2 = 1;
	// try vcio2 first
	file_desc = open("/dev/vcio2", O_RDWR);
	if (file_desc < 0)
	{	// try vcio next
		have_vcio2 = 0;
		file_desc = open("/dev/vcio", O_RDWR);
		if (file_desc < 0)
			printf("Can't open device file: /dev/vcio2 nor /dev/vcio.\n");
	}
	return file_desc;
}

void mbox_close(int file_desc)
{	close(file_desc);
}
