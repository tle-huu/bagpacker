#pragma once

#include "elf_object.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <set>
#include <vector>

class Process
{
public:

        Process()
        {
                search_paths_.emplace_back("/usr/lib/x86_64-linux-gnu/");
        }

        int run()
        {
                using Fun = int(*)(void);

                void * entry = objects_[0].entry_point();
                Fun f = (Fun)(entry);

                printf("Jumping to entry %p ...\n", (void*)(entry));
                int ret = f();
                printf("After jump\n");
                return ret;
        }

        int load_object_and_dependencies(std::filesystem::path path);
        int load_object(std::filesystem::path path);
        int adjust_permissions();
        int apply_relocations();

        friend std::ostream& operator<<(std::ostream& os, const Process& process);
// private:

        std::vector<ElfObject> objects_;
        std::vector<std::filesystem::path> search_paths_;

};