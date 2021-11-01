#pragma once

#include "elf_structures.h"
#include <cassert>
#include <filesystem>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

class ElfFile
{
public:

        struct LoadZone
        {
                Elf64_Word flags = 0;
                Elf64_Addr base = 0;
                Elf64_Xword length = 0;
        };

        const std::vector<LoadZone>& load_zones() const noexcept
        {
                return load_zones_;
        }

        ElfFile(const ElfFile &) = delete;
        ElfFile & operator=(const ElfFile & rhs) = delete; 

        ElfFile() = default;
        ElfFile(ElfFile && rhs)
        {
                *this = std::move(rhs);
        }
        ElfFile & operator=(ElfFile && rhs) 
        {
                file_data_ = rhs.file_data_;
                dynamic_section_header_ = rhs.dynamic_section_header_;
                dynamic_str_tab_ = rhs.dynamic_str_tab_;
                dynamic_sym_tab_ = rhs.dynamic_sym_tab_;
                dt_strtab_ = rhs.dt_strtab_;
                size_ = rhs.size_;
                run_paths_ = std::move(rhs.run_paths_);
                needed_ = std::move(rhs.needed_);
                load_zones_ = std::move(rhs.load_zones_);
                relocation_entries_ = std::move(rhs.relocation_entries_);
                dyn_symbols_ = std::move(rhs.dyn_symbols_);

                rhs.file_data_ = nullptr;
                rhs.dynamic_section_header_ = nullptr;
                rhs.dynamic_str_tab_ = nullptr;
                rhs.dynamic_sym_tab_ = nullptr;
                rhs.dt_strtab_ = nullptr;
                return *this;
        }

        ~ElfFile();

        int load_elf_file(const char * path);
        int parse();

        const elf64_hdr &elf_header()
        {
                return *(const elf64_hdr*)(file_data_);
        }

        const elf64_phdr *program_headers_table()
        {
                return (const elf64_phdr*)(file_data_ + elf_header().e_phoff);
        }
        const elf64_shdr *section_headers_table()
        {
                return (const elf64_shdr*)(file_data_ + elf_header().e_shoff);
        }  

        const std::vector<std::filesystem::path> &run_paths()
        {
                return run_paths_;
        }

        std::pair<uintptr_t, uintptr_t> get_pt_load_convex_hull();

        const std::vector<std::filesystem::path> & get_dependencies() const noexcept
        {
                return needed_;
        }

        const std::vector<const elf64_rela*>& relocations() const noexcept
        {
                return relocation_entries_;
        }

        const char *file_data()
        {
                return file_data_;
        }

        uintptr_t entry_point()
        {
                return elf_header().e_entry;
        }

        int retrieve_dyn_symbols();

        Elf64_Addr symbol_value_from_index(size_t sym_index)
        {
                return dyn_symbols_[sym_index]->st_value;
        }

        const std::vector<const elf64_sym*>& dyn_symbols()
        {
                return dyn_symbols_;
        }

        const char* dt_name_from_index(Elf64_Word index)
        {
                return dt_strtab_ + index;
        }

private:
        const char* sh_strtab()
        {
                return file_data_ + program_headers_table()[elf_header().e_shstrndx].p_offset;
        }

        int retrieve_pt_load_zones();
        int retrieve_rpath();
        int retrieve_dynamic_section_header();
        int retrieve_dt_strtab();
        int retrieve_sht_dynsym();
        int retrieve_needed();
        int retrieve_relocation_entries();

private:
        const char * origin_path_ = nullptr;
        char * file_data_ = nullptr;
        int size_ = 0;

        const elf64_shdr* dynamic_section_header_ = nullptr;
        const elf64_shdr* dynamic_str_tab_ = nullptr;
        const elf64_shdr* dynamic_sym_tab_ = nullptr;
        const char * dt_strtab_ = nullptr;
        const elf64_shdr * sht_dynsym_ = nullptr;

        std::vector<const elf64_rela*> relocation_entries_;
        std::vector<const elf64_sym*> dyn_symbols_;
        std::vector<LoadZone> load_zones_;
        std::vector<std::filesystem::path> run_paths_;
        std::vector<std::filesystem::path> needed_;
};