#pragma once

#include "constants.h"
#include "dependency.h"
#include "elf_structures.h"
#include "mapped_zone.h"

#include <map>
#include <string_view>
#include <vector>

class Packer
{

public:
	int run();
	int load_elf_file(std::string_view file_path);
	int parse_program_headers();
	int parse_elf_header();

	void print_main_header();
	void print_program_headers();
	void print_dynamic_entries();
	void print_section_headers();
	void print_dynsyms();
	
	int handle_program_headers();
	int handle_dynamic_entries();
	int handle_relocation_entries();
	int parse_section_headers();

	int set_mapped_zone_protect_flags()
	{
		for (uint i = 0; i <  mapped_zones_.size(); ++i)
		{
			mapped_zones_[i].set_flags();
		}
		return 0;
	}

	void print_dependencies()
	{
		printf("Dependencies:\n");
		for (int i = 0; i < dependencies_.size(); ++i)
		{
			printf("\t- { name: %s }\n", dependencies_[i].name);
		}
	}

	void resolve_rpath()
	{
		rpath_ += (uintptr_t)dynstr_;
	}

	void print_rpath()
	{
		printf("rpath: %s\n", rpath_);
	}

private:

	int get_dependency_graph();

	const elf64_shdr * get_sht_dynsym()
	{
		for (int i = 0; i < section_headers_.size(); ++i)
		{
			if (section_headers_[i]->sh_type == SHT_DYNSYM)
			{
				return section_headers_[i];
			}
		}
		printf("Could not get dynsym section header\n");
		return nullptr;
	}

	void resolve_dependencies_names()
	{
		if (dynstr_ == nullptr)
		{
			printf("Cannot resolve dependencies names: need dynamic str table\n");
			return;
		}

		for (int i = 0; i < dependencies_.size(); ++i)
		{
			dependencies_[i].name = (char*)((uintptr_t)dynstr_ + dependencies_[i].offset);
		}
	}


private:
	int handle_program_header(elf64_phdr const & header);
	int handle_dynamic_entry(elf64_dyn const & header);

	int handle_load_phdr(elf64_phdr const & header);
	int retrieve_dynamic_phdrs(elf64_phdr const & header);

	int retrieve_symtab_(elf64_dyn const & header);
	// int handle_dynamic_symbols(elf64_phdr const & header);

	int relocate(const elf64_rela &);
	int relative_relocation(const elf64_rela & rel);
	int sixtyfour_relocation(const elf64_rela & rel);
	int copy_relocation(const elf64_rela & rel);

private:

	elf64_hdr main_header_;
	uintptr_t entry_ = 0;
	char *file_data_;

	std::vector<elf64_phdr> phdrs_;
	std::vector<const elf64_shdr*> section_headers_;
	std::vector<elf64_dyn> dynamic_entries_;
	std::vector<const elf64_sym*> symtab_;

	std::vector<Dependency> dependencies_;
	std::vector<MappedZone> mapped_zones_;

	std::map<std::string, Dependency> dependencies_graph_;

	const char* rpath_ = nullptr;
	const char* dynstr_ = nullptr;
	size_t relocation_table_size_ = 0;

	const elf64_dyn * rela_section_header_ = nullptr;

};
