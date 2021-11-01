#include "elf_object.h"

#include <filesystem>

int ElfObject::load_and_parse_elf_file(std::filesystem::path path)
{
        int ret = elf_file.load_elf_file(path.c_str());
        if (ret < 0)
        {
                printf("Error loading elf_file for '%s'\n", path.c_str());
                return ret;
        }

        ret = elf_file.parse();
        if (ret < 0)
        {
                printf("Error parsing elf file for '%s'\n", path.c_str());
                return ret;
        }

        convex_hull = elf_file.get_pt_load_convex_hull();
        return ret;
}
