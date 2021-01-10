#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define CLK_RST_CONTROLLER_RST_DEV_L		0x004
#define CLK_RST_CONTROLLER_RST_DEV_V		0x058

#define CLK_RST_CONTROLLER_RST_DEV_L_SET_0	0x300
#define CLK_RST_CONTROLLER_RST_DEV_L_CLR_0	0x304

#define CLK_RST_CONTROLLER_RST_DEV_V_SET_0	0x430
#define CLK_RST_CONTROLLER_RST_DEV_V_CLR_0	0x434

#define CAR_3D	(1 << 24)
#define CAR_3D2	(1 << 2)

static int map_mem(void **mem_virt, off_t phys_address, int size)
{
	off_t PageOffset, PageAddress;
	int PagesSize;
	int mem_dev;

	mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_dev < 0)
		return mem_dev;

	PageOffset  = phys_address % getpagesize();
	PageAddress = phys_address - PageOffset;
	PagesSize   = (((size - 1) / getpagesize()) + 1) * getpagesize();

	*mem_virt = mmap(NULL, (size_t)PagesSize, PROT_READ | PROT_WRITE,
			 MAP_SHARED, mem_dev, PageAddress);

	if (*mem_virt == MAP_FAILED)
		return -1;

	*mem_virt += PageOffset;

	return 0;
}

static uint32_t reg_read(void *mem_virt, uint32_t offset)
{
	return *(volatile uint32_t*)(mem_virt + offset);
}

static void reg_write(void *mem_virt, uint32_t offset, uint32_t value)
{
	*(volatile uint32_t*)(mem_virt + offset) = value;
}

int main(int argc, char *argv[])
{
	void *CAR_io_mem_virt;
	int err;

	err = map_mem(&CAR_io_mem_virt, 0x60006000, 0x1000);
	if (err < 0) {
		fprintf(stderr, "mmap failed: %d (%s)\n",
			err, strerror(errno));
		return err;
	}

	printf("GR3D1 reset = %d\n",
	       !!(reg_read(CAR_io_mem_virt, CLK_RST_CONTROLLER_RST_DEV_L) & CAR_3D));

	printf("GR3D2 reset = %d\n",
	       !!(reg_read(CAR_io_mem_virt, CLK_RST_CONTROLLER_RST_DEV_V) & CAR_3D2));

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_L_SET_0, CAR_3D);

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_V_SET_0, CAR_3D2);

	usleep(1000);

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_L_CLR_0, CAR_3D);

	reg_write(CAR_io_mem_virt,
		  CLK_RST_CONTROLLER_RST_DEV_V_CLR_0, CAR_3D2);

	printf("done\n");

	return 0;
}
