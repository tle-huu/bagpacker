#pragma once

#include "constants.h"

#include <sys/mman.h>
#include <cassert>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>


struct MappedZone
{
	void * ptr = nullptr;
	size_t length = 0;
	int flags = PROT_READ | PROT_WRITE;

	MappedZone () = default;

	MappedZone(const MappedZone & zone) = delete;
	MappedZone & operator=(const MappedZone & zone) = delete;
	MappedZone(MappedZone && zone)
	{
		ptr = zone.ptr;
		length = zone.length;
		flags = zone.flags;
		zone.ptr = nullptr;
	}

	MappedZone & operator=(MappedZone && zone)
	{
		ptr = zone.ptr;
		length = zone.length;
		flags = zone.flags;
		zone.ptr = nullptr;
		return *this;
	}

	~MappedZone()
	{
		if (ptr == nullptr) return;
		int ret = munmap((void*)((uintptr_t)ptr & ~(sysconf(_SC_PAGE_SIZE) - 1)), length);
		if (ret < 0)
		{
			printf("MappedZone::free: munmap: [%p] (%lu): %s\n", ptr, length, strerror(errno));
		}
	};

	int set_flags()
	{
		if (ptr == nullptr)
		{
			assert(false);
			return -1;
		}
		int ret = mprotect((void*)((uintptr_t)ptr & ~(sysconf(_SC_PAGE_SIZE) - 1)), length, flags);
		if (ret != 0)
		{
			printf("MappedZone::set_flags: mprotect (%d): %s\n", errno, strerror(errno));
			switch(errno)
			{
				case ENOMEM:
					std::cerr << "ENOMEM\n";
					break;
				case EACCES:
					std::cerr << "EACCESS\n";
					break;
				case EINVAL:
					printf("EINVAL: Invalid flags: %d\n", flags);
					break;

			}
		}
		return ret;
	}


	int map(size_t length_)
	{
		length = length_;
		ptr = mmap(nullptr, length, flags,  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (ptr == MAP_FAILED)
		{
			std::cerr << "MappedZone::map: mmap: " << strerror(errno) << '\n';
			return -1;
		}
		return 0;
	}
};

