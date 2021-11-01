#include "elf_file.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


ElfFile::~ElfFile()
{
        if (file_data_)
        {
                int ret = munmap((void*)((uintptr_t)file_data_), size_);
                if (ret < 0)
                {
                        std::cerr << "ElfFile munmap error" << strerror(errno) << '\n';
                }
        }
}

int ElfFile::load_elf_file(const char * path)
{
        if (origin_path_ != nullptr)
        {
                printf("Error: ElfFile already loaded\n");
                return -1;
        }

        int fd = open(path, O_RDONLY);
        if (fd < 0) return fd;

        origin_path_ = path;
        // Map full elf file into memory
        struct stat s;
        fstat(fd, &s);
        size_ = s.st_size;
        file_data_ = (char*)mmap(0, size_, PROT_READ, MAP_PRIVATE, fd, 0);

        while (file_data_ == MAP_FAILED)
        {
                fprintf(stderr, "Memory allocation failed for '%s'(Process aborted): %s\n", path, strerror(errno));
                exit(1);
        }

        int ret = close(fd);
        return ret;
}

int ElfFile::parse()
{
        if (file_data_ == nullptr) assert(false);
        int ret = -1;

        ret = retrieve_dynamic_section_header();
        if (ret == 1)
        {
                printf("No dynamic section\n");
        }

        ret = retrieve_pt_load_zones();
        if (ret < 0)
        {
                printf("Could not retrieve pt load zones from '%s'\n", origin_path_);
                return ret;
        }

        ret = retrieve_sht_dynsym();
        if (ret < 0)
        {
                printf("Could not retrieve sht_dynsym section header from '%s'\n", origin_path_);
                return ret;
        }

        ret = retrieve_dt_strtab();
        if (ret < 0)
        {
                printf("Could not retrieve dt strtab from '%s'\n", origin_path_);
                return ret;
        }

        ret = retrieve_dyn_symbols();
        if (ret < 0)
        {
                printf("Could not retrieve dynamic symbols from '%s'\n", origin_path_);
                return ret;
        }


        ret = retrieve_rpath();
        if (ret < 0)
        {
                printf("Could not retrieve run path from '%s'\n", origin_path_);
                return ret;
        }

        ret = retrieve_needed();
        if (ret < 0)
        {
                printf("Could not retrieve dynamic needed from '%s'\n", origin_path_);
                return ret;
        }

        ret = retrieve_relocation_entries();
        if (ret < 0)
        {
                printf("Could not retrieve relocation entries from '%s'\n", origin_path_);
                return ret;
        }
        return ret;
}

int ElfFile::retrieve_pt_load_zones()
{
        for (int i = 0; i < elf_header().e_phnum; ++i)
        {
                const auto & hdr = program_headers_table()[i];
                if (hdr.p_type == PT_LOAD)
                {
                        Elf64_Word prot_flags = 0;
                        prot_flags |= ((hdr.p_flags & PF_R) ? PROT_READ : 0);
                        prot_flags |= ((hdr.p_flags & PF_W) ? PROT_WRITE : 0);
                        prot_flags |= ((hdr.p_flags & PF_X) ? PROT_EXEC : 0);
                        load_zones_.emplace_back(LoadZone{prot_flags, hdr.p_vaddr, hdr.p_memsz});
                }
        }
        return 0;
}

int ElfFile::retrieve_relocation_entries()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        int size = dynamic_section_header_->sh_size / dynamic_section_header_->sh_entsize;
        const elf64_dyn* dyntab = (const elf64_dyn*)(file_data_ + dynamic_section_header_->sh_offset);


        const elf64_rela * rela_tab = nullptr;
        Elf64_Xword relatab_size = -1;
        Elf64_Xword relatab_ent_size = -1;
        for (int i = 0; i < size; ++i)
        {
                switch (dyntab[i].d_tag)
                {
                        case DT_RELA:
                                rela_tab = (const elf64_rela *)(file_data_ + dyntab[i].d_un.d_ptr);
                                break;
                        case DT_RELASZ:
                                relatab_size = dyntab[i].d_un.d_val;
                                break;
                        case DT_RELAENT:
                                relatab_ent_size = dyntab[i].d_un.d_val;
                                break;
                }
        }

        if (rela_tab == nullptr) return 0;
        for (int i = 0; i < relatab_size / relatab_ent_size; ++i)
        {
                relocation_entries_.emplace_back(&rela_tab[i]);
        }        
        return 0;
}

int ElfFile::retrieve_sht_dynsym()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        const elf64_shdr* section_headers = section_headers_table();
        for (int i = 0; i < elf_header().e_shnum; ++i)
        {
                if (section_headers[i].sh_type == SHT_DYNSYM)
                {
                        sht_dynsym_ = &section_headers[i];
                        return 0;
                }
        }

        return -1;      
}

int ElfFile::retrieve_dyn_symbols()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        const elf64_sym* sym_runner = (const elf64_sym*)(file_data_ + sht_dynsym_->sh_offset);
        for (int i = 0; i < sht_dynsym_->sh_size / sht_dynsym_->sh_entsize; ++i)
        {
                dyn_symbols_.push_back(&sym_runner[i]);
        }
        return 0;
}

int ElfFile::retrieve_dt_strtab()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        dt_strtab_ = file_data_ + (sht_dynsym_ + 1)->sh_offset;
        return 0;

        return -1;      
}

int ElfFile::retrieve_dynamic_section_header()
{
        const elf64_shdr* section_headers = section_headers_table();
        for (int i = 0; i < elf_header().e_shnum; ++i)
        {
                if (section_headers[i].sh_type == SHT_DYNAMIC)
                {
                        dynamic_section_header_ = &section_headers[i];
                        return 0;
                }
        }
        return 1;
}

int ElfFile::retrieve_rpath()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        int size = dynamic_section_header_->sh_size / dynamic_section_header_->sh_entsize;
        const elf64_dyn* dyntab = (const elf64_dyn*)(file_data_ + dynamic_section_header_->sh_offset);
        for (int i = 0; i < size; ++i)
        {
                if (dyntab[i].d_tag == RPATH)
                {
                        std::string rpath = dt_strtab_ + dyntab[i].d_un.d_ptr;
                        int start = 0;
                        for (int j = 0; j < rpath.length(); ++j)
                        {
                                if (rpath[j] == ':')
                                {
                                        run_paths_.emplace_back(rpath.substr(start, j - start));
                                        start = j + 1;
                                }
                        }
                        run_paths_.emplace_back(rpath.substr(start, rpath.length() - start));
                        return 0;
                }
        }
        return 1;
}

int ElfFile::retrieve_needed()
{
        if (dynamic_section_header_ == nullptr)
        {
                return 0;
        }

        int size = dynamic_section_header_->sh_size / dynamic_section_header_->sh_entsize;
        const elf64_dyn* dyntab = (const elf64_dyn*)(file_data_ + dynamic_section_header_->sh_offset);
        for (int i = 0; i < size; ++i)
        {
                if (dyntab[i].d_tag == NEEDED)
                {
                        needed_.emplace_back(dt_strtab_ + dyntab[i].d_un.d_ptr);
                }
        }
        return 0;
}


std::pair<uintptr_t, uintptr_t> ElfFile::get_pt_load_convex_hull()
{
        uintptr_t low = -1;
        uintptr_t high = -1;

        for (int i = 0; i < elf_header().e_phnum; ++i)
        {
                if (program_headers_table()[i].p_type == PT_LOAD)
                {
                        if (low == -1)
                        {
                                low = program_headers_table()[i].p_vaddr;
                        }
                        high = program_headers_table()[i].p_vaddr + program_headers_table()[i].p_memsz;
                }
        }
        return {low, high};
}
