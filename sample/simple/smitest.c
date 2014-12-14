#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "mailbox.h"

#define VEC_COUNT 3*16
#define GPU_MEM_FLG 0xC // cached=0xC; direct=0x4

static const unsigned code[] =
{
	#include "smitest.hex"
};

static const unsigned input[VEC_COUNT*2] =
{	0,0, 1,1, 2,2, 3,3, 4,4, 5,5, 6,6, 7,7, 8,8, 9,9, 10,10, 11,11, 12,12, 13,13, 14,14, 15,15,
	-16,-16, -15,-15, -14,-14, -13,-13, -12,-12, -11,-11, -10,-10, -9,-9,
	-8,-8, -7,-7, -6,-6, -5,-5, -4,-4, -3,-3, -2,-2, -1,-1,
	0x3b800000,0x3b800000, 0x3c000000,0x3c000000, 0x3c800000,0x3c800000, 0x3d000000,0x3d000000,
	0x3d800000,0x3d800000, 0x3e000000,0x3e000000, 0x3e800000,0x3e800000, 0x3f000000,0x3f000000,
	0x3f800000,0x3f800000, 0x40000000,0x40000000, 0x40800000,0x40800000, 0x41000000,0x41000000,
	0x41800000,0x41800000, 0x42000000,0x42000000, 0x42800000,0x42800000, 0x43000000,0x43000000
};

static const char op[28][8] =
{	"fadd", "fsub", "fmin", "fmax", "fminabs", "fmaxabs", "ftoi", "itof",
	"add", "sub", "shr", "asr", "ror", "shl", "min", "max", "and", "or", "xor", "not", "clz",
	"fmul", "mul24", "v8muld", "v8min", "v8max", "v8adds", "v8subs"
};

struct GPU
{
	unsigned code[sizeof code / sizeof *code];
	unsigned data[VEC_COUNT*(2+28)]; // 2 input, 28 output
	unsigned unif[3];
	unsigned mail[2];
	unsigned handle;
	int      mb;
};

int gpu_prepare(
	volatile struct GPU **gpu)
{
	unsigned handle, vc;
	volatile struct GPU* ptr;
	int mb = mbox_open();
	if (mb < 0)
		return -1;

	if (qpu_enable(mb, 1)) return -2;

	handle = mem_alloc(mb, sizeof(struct GPU), 4096, GPU_MEM_FLG);
	//printf("handle=%x\n", handle);
	if (!handle)
	{	qpu_enable(mb, 0);
		return -3;
	}
	vc = mem_lock(mb, handle);
	//printf("vc=%x\n", vc);
	ptr = mapmem(vc, sizeof(struct GPU));
	//printf("ptr=%p\n", ptr);
	if (ptr == NULL)
	{	mem_free(mb, handle);
		mem_unlock(mb, handle);
		qpu_enable(mb, 0);
		return -4;
	}

	ptr->mb = mb;
	ptr->handle = handle;
	ptr->mail[0] = vc + offsetof(struct GPU, unif);
	ptr->mail[1] = vc;

	*gpu = ptr;
	return 0;
}

unsigned gpu_execute(volatile struct GPU *gpu)
{
	//printf("msg=%x\n", gpu->mail[1] + offsetof(struct GPU, mail));
	return execute_qpu(
		gpu->mb,
		1 /* 1 QPU */,
		gpu->mail[1] + offsetof(struct GPU, mail),
		1 /* no flush */,
		5000 /* timeout */);
}

void gpu_release(volatile struct GPU *gpu)
{
	int mb = gpu->mb;
	unsigned handle = gpu->handle;
	unmapmem((void*)gpu, sizeof(struct GPU));
	mem_unlock(mb, handle);
	mem_free(mb, handle);
	qpu_enable(mb, 0);
	mbox_close(mb);
}

int main()
{
	int i,j;
	volatile struct GPU* gpu;
	int ret = gpu_prepare(&gpu);
	if (ret < 0)
		return ret;

	memcpy((void*)gpu->code, code, sizeof gpu->code);

	gpu->unif[0] = VEC_COUNT/16;
	gpu->unif[1] = gpu->mail[1] + offsetof(struct GPU, data);
	gpu->unif[2] = gpu->mail[1] + offsetof(struct GPU, data) + 2*VEC_COUNT*4;

	memset((void*)gpu->data, 0xbb, sizeof gpu->data);
	memcpy((void*)gpu->data, input, sizeof input);

	printf("Exec: %x\n", gpu_execute(gpu));

	for (i = 0; i < VEC_COUNT; ++i)
	{
		unsigned A = gpu->data[2*i];
		unsigned B = gpu->data[2*i+1];
		printf("\n A\t%i\t0x%08x\n B\t%i\t0x%08x\n", A, A, B, B);
		volatile unsigned* rp = &gpu->data[2*VEC_COUNT+28*i];
		for (j = 0; j < 28; ++j)
			printf("%s\t%i\t0x%08x\n", op[j], rp[j], rp[j]);
	}

	gpu_release(gpu);

	return 0;
}
