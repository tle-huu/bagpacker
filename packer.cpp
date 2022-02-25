#include "packer.h"
#include "elf_utils.h"

#include <cassert>
#include <fcntl.h>
#include <unordered_set>
#include <string.h>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

int Packer::run()
{
	if (entry_ - BASE_ADDRESS == 0)
	{
		return -1;
	}
	using Fun = int(*)(void);

	Fun f = (Fun)(entry_);

	printf("Jumping to entry %p   (%p)...\n", (void*)(entry_ - BASE_ADDRESS), (void*)entry_);
	int ret = f();
	printf("After jump\n");
	return ret;
}


int Packer::load_elf_file(std::string_view file_path)
{
	int fd = open(file_path.data(), O_RDONLY);
	if (fd < 0) return fd;

	// Map full elf file into memory
	struct stat s;
	fstat(fd, &s);
	int size = s.st_size;
	file_data_ = (char*)mmap(0, size, PROT_READ | PROT_EXEC, MAP_PRIVATE, fd, 0);
	close(fd);
	return 0;
}

int Packer::parse_program_headers()
{
	assert(sizeof(elf64_phdr) == main_header_.e_phentsize);
	phdrs_.resize(main_header_.e_phnum);

	for (int i = 0; i < main_header_.e_phnum; ++i)
	{
		phdrs_[i] = *((elf64_phdr*)(file_data_ + main_header_.e_phoff + main_header_.e_phentsize * i));
	}
	return 0;
}

int Packer::parse_elf_header()
{
	memcpy(&main_header_, file_data_, sizeof(elf64_hdr));
	entry_ = BASE_ADDRESS + main_header_.e_entry;
	return 0;
}


int Packer::handle_program_headers()
{
	int ret = 0;
	for (int i = 0; i < phdrs_.size(); ++i)
	{
		ret |= handle_program_header(phdrs_[i]);
	}
	return ret;
}

int Packer::handle_program_header(elf64_phdr const & header)
{
	switch (header.p_type)
	{
		case PT_NULL:
			break;
		case PT_LOAD:
			return handle_load_phdr(header);
		case PT_DYNAMIC:
			return retrieve_dynamic_phdrs(header);
			break;
		case PT_INTERP:
		case PT_NOTE:
		case PT_SHLIB:
		case PT_PHDR:
		case PT_TLS:
		case PT_LOOS:
		case PT_HIOS:
		case PT_LOPROC:
		case PT_HIPROC:
		case PT_GNU_EH_FRAME:
		default:
			printf("Unrecognized header type %d\n", header.p_type);
	}
	return 0;
}

int Packer::handle_dynamic_entries()
{
	int ret = 0;
	for (int i = 0; i < dynamic_entries_.size(); ++i)
	{
		ret |= handle_dynamic_entry(dynamic_entries_[i]);
	}
	resolve_dependencies_names();
	return ret;
}

int Packer::handle_relocation_entries()
{
	if (rela_section_header_ == nullptr || relocation_table_size_ == 0)
	{
		printf("No relocation.\n");
		return 0;
	}
	const elf64_rela * rel_array = (elf64_rela*)(file_data_ + rela_section_header_->d_un.d_ptr);
	for (size_t i = 0; i < relocation_table_size_; ++i)
	{
		relocate(rel_array[i]);
	}
	return 0;

}

int Packer::retrieve_symtab_(elf64_dyn const & header)
{
	printf("Retrieving symtab...\n");
	const elf64_shdr * sht_dynsym = get_sht_dynsym();
	if (sht_dynsym == nullptr) return -1;

	int size = sht_dynsym->sh_size / sht_dynsym->sh_entsize;
	const elf64_sym * sym_runner = (const elf64_sym*)(file_data_ + sht_dynsym->sh_offset);

	for (int i = 0; i < size; ++i)
	{
		symtab_.push_back(&sym_runner[i]);		
	}
	printf("Retrived %d dyn syms\n", size);
	print_dynsyms();
	return 0;
}


int Packer::handle_dynamic_entry(elf64_dyn const & header)
{
	switch (header.d_tag)
	{
		case Null:
			break;
		case NEEDED:
		{
			dependencies_.emplace_back(Dependency{header.d_un.d_ptr, nullptr, {}});
			break;
		}
		case PLTRELSZ:
			break;
		case PLTGOT:
			break;
		case HASH:
			break;
		case STRTAB:
			if (dynstr_ == nullptr)
			{
				dynstr_ = file_data_ + header.d_un.d_ptr;
			}
			break;
		case SYMTAB:
			retrieve_symtab_(header);
			break;
		case RELA:
			rela_section_header_ = &header;
			break;
		case RELASZ:
			relocation_table_size_ = header.d_un.d_val;
			break;
		case RELAENT:
			relocation_table_size_ /= header.d_un.d_val;
			printf("Setting relocation_table_size_ to %lu\n", relocation_table_size_);
			break;
		case RPATH:
		case RUNPATH:
			rpath_ = (char*)header.d_un.d_ptr;
			break;
		case STRSZ:
		case SYMENT:
		case INIT:
		case FINI:
		case SONAME:
			break;
		case SYMBOLIC:
		case REL:
		case RELSZ:
		case RELENT:
		case PLTREL:
		case DEBUG:
		case TEXTREL:
		case JMPREL:
		case BINDNOW:
		case INITARRAY:
		case FINIARRAY:
		case INITARRAYSZ:
		case FINIARRAYSZ:
		case LOOS:
		case HIOS:
		case LOPROC:
		case HIPROC:
		case GNUHASH:
		case FLAGS1:
		case RELACOUNT:
		default:
			break;
	}
	// printf("Unrecognized dynamic tag %s\n", dynamic_tag_to_str(DynamicTag(header.d_tag)));
	return 0;
}

int Packer::handle_load_phdr(elf64_phdr const & phdr)
{

	printf("Mapping header @ 0x%lx ..... 0x%lx | %s%s%s\n",
		BASE_ADDRESS + phdr.p_vaddr,
		BASE_ADDRESS + phdr.p_vaddr + phdr.p_filesz,
		(phdr.p_flags & PF_R) ? "R" : ".",
		(phdr.p_flags & PF_W) ? "W" : ".",
		(phdr.p_flags & PF_X) ? "X" : "."
		);

	if (phdr.p_filesz == 0) return 0;
	// Length needs to account for page overflow
	size_t length = phdr.p_filesz + phdr.p_vaddr % sysconf(_SC_PAGE_SIZE);

	MappedZone mapped_zone;
	// Mapping to p_vaddr
	int ret = mapped_zone.map((void*)phdr.p_vaddr, length);
	if (ret == -1)
	{
		printf("Error mapping zone\n");
		return -1;
	}
	mapped_zone.length = length;
	mapped_zone.flags = phdr.p_flags;
	mapped_zones_.emplace_back(std::move(mapped_zone));

	// Copy data from elf base file to virtual memory
	memcpy((void*)(BASE_ADDRESS + (uintptr_t)phdr.p_vaddr), file_data_ + phdr.p_offset, phdr.p_filesz);

	return 0;
}

int Packer::retrieve_dynamic_phdrs(elf64_phdr const & header)
{
	printf("Handling dynamic header\n");
	elf64_dyn *dyn_entries = ((elf64_dyn*)(file_data_ + header.p_offset));
	for (int i = 0; i < header.p_filesz / sizeof(elf64_dyn); ++i)
	{
		if (dyn_entries[i].d_tag == 0) break;
		dynamic_entries_.push_back(dyn_entries[i]);
	}
	return 0;
}

void Packer::print_main_header()
{
	print_elf_header(main_header_);
}

void Packer::print_program_headers()
{
	for (int i = 0 ; i < main_header_.e_phnum; ++i)
	{
		printf("============ Program Header %d =========\n", i);
		print_program_header(phdrs_[i]);
		printf("\n");
	}
}

void Packer::print_dynamic_entries()
{
	printf("Dynamic Entries:\n");
	for (int i = 0 ; i < dynamic_entries_.size(); ++i)
	{
		printf("\t- Dynamic Entry { tag: %s, addr: %.8lx }\n", dynamic_tag_to_str((DynamicTag)dynamic_entries_[i].d_tag), (long int)(dynamic_entries_[i].d_un.d_val));
	}
}

void Packer::print_section_headers()
{
	printf("Section Headers:\n");
	for (int i = 0 ; i < section_headers_.size(); ++i)
	{
		printf("{\n");
		print_section_header(*section_headers_[i]);
		printf("}\n");
	}
}


int Packer::relocate(const elf64_rela & rel)
{
	printf("Relocation...\n");
	print_rela(rel);
	RelaType type = (RelaType)(rel.r_info & 0x00000000FFFFFFFF);

	switch (type)
	{
		case RELATIVE:
		{
			printf("Relative relocating 8-byte word %p to %p\n", (void*)(BASE_ADDRESS + rel.r_addend),(void*)(BASE_ADDRESS + rel.r_offset));
			uint64_t to_relocate = BASE_ADDRESS + rel.r_addend;
			memcpy((void*)(BASE_ADDRESS + rel.r_offset), (void*)(&to_relocate), 8);	
			break;
		}
		case _64:
			// return sixtyfour_relocation(rel);
		case COPY:
			// return copy_relocation(rel);
		default:
			printf("Cannot handle relocation type '%s'\n", rela_type_to_str(type));
			return 0;
	}
	return 0;
}

int Packer::parse_section_headers()
{
	const elf64_shdr * runner = (elf64_shdr *)(file_data_ + (uintptr_t)main_header_.e_shoff);

	for (int i = 0; i < main_header_.e_shnum; i++)
	{
		section_headers_.push_back(&runner[i]);
	}

	return 0;
}

int Packer::sixtyfour_relocation(const elf64_rela & rel)
{
	return 1;
}

int Packer::copy_relocation(const elf64_rela & rel)
{
	return 1;
}

void Packer::print_dynsyms()
{
	printf("Dynamic symbols:\n");
	for (int i = 0; i < symtab_.size(); ++i)
	{
		const elf64_sym & sym = *symtab_[i];
		printf("\t- %d - { st_name: '%s' (%u), st_info: %d, st_other: %d, st_shndx: %d, st_value: 0x%lx, st_size: 0x%lx }\n"
			, i, dynstr_ + sym.st_name, sym.st_name, sym.st_info ,sym.st_other ,sym.st_shndx, sym.st_value, sym.st_size);

	}
}


int Packer::get_dependency_graph()
{
	std::vector<Dependency> ordered_dependencies;
	std::unordered_set<std::string> deja_vu;
	std::vector<std::string> process_queue;

	// Init queue in reverse order
	for (int i = dependencies_.size() - 1; i >= 0; --i)
	{
		process_queue.push_back(dependencies_[i].name);
		deja_vu.emplace(dependencies_[i].name);
	}


	while (process_queue.size() > 0)
	{
		const std::string & current = process_queue.back();
		process_queue.pop_back();

		
	}

	return 0;
}
