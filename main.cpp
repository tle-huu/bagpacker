#include "constants.h"
#include "mapped_zone.h"
#include "elf_utils.h"
#include "elf_file.h"
#include "process.h"

#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>
#include <utility>

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Missing args\n");
		return 0;
	}


	std::filesystem::path file = argv[1];

	Process process;
        int ret = 0;

	ret = process.load_object_and_dependencies(file);
	if (ret < 0)
	{
		return 2;
	}
	std::cout << process;


	ret = process.apply_relocations();
	if (ret < 0)
	{
		printf("Could not apply relocations\n");
		return ret;
	}
	ret = process.adjust_permissions();
	if (ret < 0)
	{
		printf("Could not adjust permissions\n");
		return ret;
	}

	ret = process.run();

	return ret;
}
