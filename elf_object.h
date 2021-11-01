#pragma once

#include "elf_file.h"
#include "mapped_zone.h"

#include <filesystem>
#include <utility>

struct ElfObject
{
        using ConvexHull = std::pair<uintptr_t, uintptr_t>;

        ElfObject() = default;
        ~ElfObject() = default;
        ElfObject(const ElfObject &) = delete;
        ElfObject & operator=(const ElfObject & rhs) = delete; 

        ElfObject(ElfObject && rhs)
        {
                *this = std::move(rhs);
        }

        ElfObject & operator=(ElfObject && rhs)
        {
                path = rhs.path;
                convex_hull = rhs.convex_hull;
                elf_file = std::move(rhs.elf_file);
                load_zone = std::move(rhs.load_zone);

                return *this;
        }


public:

        int load_and_parse_elf_file(std::filesystem::path path);

        uintptr_t base() const
        {
                if (load_zone.ptr == nullptr) return -1;
                return ((uintptr_t)load_zone.ptr - convex_hull.first);
        }

        void *entry_point()
        {
                return (void*)((uintptr_t)base() + elf_file.entry_point());
        }

        int load()
        {
                size_t length = convex_hull.second - convex_hull.first;
                assert(length != 0);
                int ret = load_zone.map(length);
                if (ret < 0)
                {
                        printf("Error mapping in virtual memory\n");
                        return ret;
                }
                memcpy((void*)load_zone.ptr, elf_file.file_data(), load_zone.length);

                return 0;
        }

        int set_final_map_protections()
        {
                for (int i = 0; i < elf_file.load_zones().size(); ++i)
                {
                        const auto & zone = elf_file.load_zones()[i];
                        if (zone.length == 0) continue;
                        printf("Setting prot @ 0x%lx...0x%lx | %s%s%s\n",
                                ((uintptr_t)base() + zone.base),
                                ((uintptr_t)base() + zone.base + zone.length),
                                (zone.flags & PF_R) ? "R" : ".",
                                (zone.flags & PF_W) ? "W" : ".",
                                (zone.flags & PF_X) ? "X" : "."
                                );
                        int ret = mprotect((void*)(((uintptr_t)base() + zone.base) & ~(sysconf(_SC_PAGE_SIZE) - 1)), zone.length, zone.flags);

                }
                return 0;

        }


public:

        std::filesystem::path path;
        ElfFile elf_file;
        ConvexHull convex_hull;

// private:
        MappedZone load_zone;
};