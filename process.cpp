#include "process.h"

#include <filesystem>
#include <queue>

int Process::load_object_and_dependencies(std::filesystem::path path)
{
        std::set<std::filesystem::path> deja_vu;
        std::queue<std::filesystem::path> queue;
        queue.push(path);

        while (queue.size() > 0)
        {
                std::filesystem::path current = queue.front();
                queue.pop();
                if (deja_vu.count(current) > 0)
                {
                        continue;
                }
                deja_vu.insert(current);
                int ret = load_object(current);
                if (ret < 0)
                {
                        return -1;
                }
                const auto & deps = objects_.back().elf_file.get_dependencies();
                int deps_size = deps.size();
                for (int i = 0; i < deps_size; ++i)
                {
                        queue.push(deps[i]);
                }
        }

        return 0;
}

int Process::load_object(std::filesystem::path path)
{

        // Looking for the file in the different search paths
        int ret = std::filesystem::exists(path) ? 0 : -1;
        std::filesystem::path full_path = std::filesystem::absolute(path);
        if (ret < 0)
        {
                for (int i = 0; i < search_paths_.size(); ++i)
                {
                        auto && tmp = search_paths_[i] / path;
                        if (std::filesystem::exists(tmp))
                        {
                                ret = 0;
                                full_path.swap(tmp);
                                break;
                        }
                }
        }
        if (ret < 0)
        {
                printf("Error cannot find file '%s'\n", path.c_str());
                return ret;
        }

        ElfObject obj;
        // Load the elf file into memory
        ret = obj.load_and_parse_elf_file(full_path);
        if (ret < 0)
        {
                printf("Error cannot loading object\n");
                return -1;
        }

        // Mapping load sections into memory
        ret = obj.load();
        if (ret < 0)
        {
                printf("Error mapping load sections of file '%s'\n", full_path.c_str());
                return -1;
        }

        // Add search_paths for future lookups
        std::filesystem::path parent_path = full_path.parent_path();
        auto & run_paths = obj.elf_file.run_paths();
        for (const auto & rp : run_paths)
        {
                if (rp == "$ORIGIN")
                {
                        search_paths_.emplace_back(parent_path);
                }
                else
                {
                        search_paths_.emplace_back(rp);
                }
        }

        obj.path = full_path;
        objects_.emplace_back(std::move(obj));

        return 0;
}

int Process::adjust_permissions()
{
        for (int i = 0; i < objects_.size(); ++i)
        {
                objects_[i].set_final_map_protections();
        }
        return 0;
}

int Process::apply_relocations()
{
        for (int i = 0; i < objects_.size(); ++i)
        {
                const std::vector<const elf64_rela*> & relocations = objects_[i].elf_file.relocations();
                for (const auto & rela : relocations)
                {
                        Elf64_Xword sym_index = ELF64_R_SYM(rela->r_info);
                        const elf64_sym* symbol = objects_[i].elf_file.dyn_symbols()[sym_index]; 
                        uintptr_t sym_value = objects_[i].elf_file.symbol_value_from_index(sym_index);
                        switch (ELF64_R_TYPE(rela->r_info))
                        {
                                case (None):
                                        break;
                                case (_64):
                                {
                                        uintptr_t jump_addr = (uintptr_t)objects_[i].base() + sym_value;
                                        memcpy((void*)(objects_[i].base() + rela->r_offset), &jump_addr, 8);
                                        break;
                                }
                                case (COPY):
                                        for (int j = 0; j < objects_.size(); ++j)
                                        {
                                                if (j == i) continue;
                                                for (const auto & sym : objects_[j].elf_file.dyn_symbols())
                                                {
                                                        if (strcmp(objects_[j].elf_file.dt_name_from_index(sym->st_name)
                                                                , objects_[i].elf_file.dt_name_from_index(symbol->st_name)) == 0)
                                                        {
                                                                uintptr_t sym_location = sym->st_value;
                                                                memcpy((void*)(objects_[i].base() + rela->r_offset), (void*)(objects_[j].base() + sym_location), sym->st_size);
                                                                break;
                                                        }
                                                }
                                        }
                                        break;
                                case (RELATIVE):
                                {
                                        uintptr_t to_relocate = (uintptr_t)objects_[i].base() + rela->r_addend;
                                        memcpy((void*)(objects_[i].base() + rela->r_offset), (void*)(&to_relocate), 8); 
                                        break;
                                }
                                case (PC32):
                                case (GOT32):
                                case (PLT32):
                                case (GLOB_DAT):
                                case (JUMP_SLOT):
                                        printf("Relocation not implemented\n");
                                        return -1;

                        }
                }
        }
        return 0;
}



std::ostream& operator<<(std::ostream& os, const Process& process)
{
        printf("Process {\n");
        printf("\tsearchs_path: [\n");
        for (const auto& sp : process.search_paths_)
        {
                printf("\t\t- %s\n", sp.c_str());
        }
        printf("\t]\n");
        printf("\tObjects: [\n");
        for (const auto & obj : process.objects_)
        {
                printf("\t\t- Object {\n");
                printf("\t\t\tpath: %s\n", obj.path.c_str());
                printf("\t\t\tbase: %lx\n", (uintptr_t)obj.base());
                printf("\t\t\tmem_range: 0x%lx...0x%lx\n", (uintptr_t)obj.convex_hull.first, (uintptr_t)obj.convex_hull.second);
                printf("\t\t}\n");
        }
        printf("\t]\n");

        printf("}\n");
        return os;
}
