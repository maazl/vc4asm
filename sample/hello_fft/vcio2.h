#ifndef _MACH_BCM2708_VCIO2_H
#define _MACH_BCM2708_VCIO2_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#endif

/** Version of vcio2 where this file belongs to.
 * This is the version of the file and not the version of the driver you attach to.
 * It is a good advise to check the version at program startup to avoid incompatibilities.
 * @example
 * int vcio = open("/dev/vcio2", O_RDWR);
 * if (vcio < 0)
 *    ;// no vcio2 driver or not accessible.
 * if (!vcio2_version_is_compatible(ioctl(vcio, IOCTL_GET_VCIO_VERSION, 0)))
 *    ;// Driver version does not match.
 */
#define VCIO2_VERSION_NUMBER 0x00000003

/** Check whether the version of the driver is compatible to the current program.
 * @param version Driver version number from IOCTL_GET_VCIO_VERSION.
 * @return true: version is compatible.
 * @remarks This method is adjusted with newer versions of this file to match
 * compatibility guarantees. */
#define vcio2_version_is_compatible(version) \
	version == VCIO2_VERSION_NUMBER

#define IOCTL_VCIO2_TYPE 101

/** Get driver version
 * return value: unsigned high word: major version, low word: minor version */
#define IOCTL_GET_VCIO_VERSION _IO(IOCTL_VCIO2_TYPE, 0xc0)

/** Allocate VC memory */
#define IOCTL_MEM_ALLOCATE _IOWR(IOCTL_VCIO2_TYPE, 0x0c, vcio_mem_allocate)
typedef struct
{	union {
		struct
		{	uint32_t size;       /**< Number of bytes to allocate */
			uint32_t alignment;  /**< Alignment */
			uint32_t flags;      /**< Allocation flags */
		} in;                  /**< IOCTL parameter data */
		struct
		{	uint32_t handle;     /**< QPU memory handle */
		} out;                 /**< IOCTL return data */
	};
} vcio_mem_allocate;       /**< IOCTL structure for IOCTL_MEM_ALLOCATE */

/** Allocate uncached, page aligned GPU memory.
 * @param handle File handle of the vcio2 device.
 * @param Size in bytes to allocate.
 * @return Virtual address of the memory.
 * The bus address of the memory is stored in the first uint32_t of the just allocated memory.
 * @remarks This is simply a short cut for calling mmap with the appropriate parameters. */
#define vcio2_malloc(handle, size) mmap(NULL, (size), PROT_READ|PROT_WRITE, MAP_SHARED, (handle), 0)

/** Release VC memory
 * IOCTL param: unsigned handle of memory from IOCTL_MEM_ALLOCATE. */
#define IOCTL_MEM_RELEASE  _IO(IOCTL_VCIO2_TYPE, 0x0f)

/** Lock VC memory at fixed address
 * IOCTL param input: uint32_t* handle of memory from IOCTL_MEM_ALLOCATE.
 * IOCTL param output: uint32_t* bus_address of the memory. */
#define IOCTL_MEM_LOCK     _IOWR(IOCTL_VCIO2_TYPE, 0x0d, uint32_t)

/** Unlock VC memory
 * IOCTL param: unsigned handle of memory from IOCTL_MEM_ALLOCATE. */
#define IOCTL_MEM_UNLOCK   _IO(IOCTL_VCIO2_TYPE, 0x0e)

/// Query information about memory location
#define IOCTL_MEM_QUERY    _IOWR(IOCTL_VCIO2_TYPE, 0x8f, vcio_mem_query)
typedef struct
{ uint32_t handle;         /**< Handle of the memory allocation */
  uint32_t bus_addr;       /**< Bus address of memory block */
  void*    virt_addr;      /**< Virtual address of the memory block in user space */
  uint32_t size;           /**< Size of the memory block */
} vcio_mem_query;          /**< IOCTL structure for IOCTL_MEM_QUERY */

/** Enable QPU power
 * IOCTL param: unsigned flag whether to enable or disable the QPU */
#define IOCTL_ENABLE_QPU   _IO(IOCTL_VCIO2_TYPE, 0x12)

/** Execute QPU code */
#define IOCTL_EXEC_QPU     _IOW(IOCTL_VCIO2_TYPE, 0x11, vcio_exec_qpu)
typedef struct
{	uint32_t uniforms;       /**< Bus address of start of uniforms for one QPU */
	uint32_t code;           /**< Bus address of start of code for one QPU */
} vcio_exec_qpu_entry;     /**< Startup data for one QPU */
typedef struct
{	uint32_t num_qpus;       /**< Number of QPUs to use */
	uint32_t control;        /**< Bus address of array of vcio_exec_qpu_entry with num_qpus elements */
	uint32_t noflush;        /**< 1 => do not flush L2 cache */
	uint32_t timeout;        /**< Timeout in ms */
} vcio_exec_qpu;           /**< IOCTL structure for IOCTL_EXEC_QPU */

/** Enable performance counters
 * IOCTL param: Bit vector of counters to enable for this instance.
 * Note that no more than 16 performance counters may be enabled at the same time. */
#define IOCTL_SET_V3D_PERF_COUNT _IO(IOCTL_VCIO2_TYPE, 0xc1)
/** Performance counters supported by this driver. */
enum v3d_perf_count
{	V3D_PERF_COUNT_QPU_CYCLES_IDLE               = 1<<13,
	V3D_PERF_COUNT_QPU_CYCLES_VERTEX_SHADING     = 1<<14,
	V3D_PERF_COUNT_QPU_CYCLES_FRAGMENT_SHADING   = 1<<15,
	V3D_PERF_COUNT_QPU_CYCLES_VALID_INSTRUCTIONS = 1<<16,
	V3D_PERF_COUNT_QPU_CYCLES_STALLED_TMU        = 1<<17,
	V3D_PERF_COUNT_QPU_CYCLES_STALLED_SCOREBOARD = 1<<18,
	V3D_PERF_COUNT_QPU_CYCLES_STALLED_VARYINGS   = 1<<19,
	V3D_PERF_COUNT_QPU_INSTRUCTION_CACHE_HITS    = 1<<20,
	V3D_PERF_COUNT_QPU_INSTRUCTION_CACHE_MISSES  = 1<<21,
	V3D_PERF_COUNT_QPU_UNIFORMS_CACHE_HITS       = 1<<22,
	V3D_PERF_COUNT_QPU_UNIFORMS_CACHE_MISSES     = 1<<23,
	V3D_PERF_COUNT_TMU_TEXTURE_QUADS_PROCESSED   = 1<<24,
	V3D_PERF_COUNT_TMU_TEXTURE_CACHE_MISSES      = 1<<25,
	V3D_PERF_COUNT_VPM_CYCLES_STALLED_VDW        = 1<<26,
	V3D_PERF_COUNT_VPM_CYCLES_STALLED_VCD        = 1<<27,
	V3D_PERF_COUNT_L2C_L2_CACHE_HITS             = 1<<28,
	V3D_PERF_COUNT_L2C_L2_CACHE_MISSES           = 1<<29,
};

/** Get currently enabled performance counters of instance.
 * IOCTL param output: uint32_t
 */
#define IOCTL_GET_V3D_PERF_COUNT _IOR(IOCTL_VCIO2_TYPE, 0xc1, uint32_t)

/** Read performance counters
 * IOCTL param input: uint32_t[] where to store the counter values in ascending order.
 * No more counters than enabled for this device handle will be stored and at most 16. */
#define IOCTL_READ_V3D_PERF_COUNT _IOR(IOCTL_VCIO2_TYPE, 0xc2, uint32_t[16])

/** Reset performance counters */
#define IOCTL_RESET_V3D_PERF_COUNT _IO(IOCTL_VCIO2_TYPE, 0xc3)

#undef DEVICE_FILE_NAME
#define DEVICE_FILE_NAME "vcio2" /**< The name of the device file */

#endif
