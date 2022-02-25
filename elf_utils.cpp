#include "elf_utils.h"

#include <elf.h>
#include <cassert>
#include <stdio.h>
#include <string.h>
#include <iostream>

void print_elf_header(struct elf64_hdr const & hdr)
{
	printf("======== ELF Header ======\n");
	for (int i = 0; i < EI_NIDENT; ++i)
	{
		printf("%x ", hdr.e_ident[i]);
	}
	printf("\n");
	std::cout << "hdr.e_type:           "            <<    hdr.e_type << '\n';
	std::cout << "hdr.e_hdr.e_machine:  "    <<   hdr.e_machine << '\n';;
	std::cout << "hdr.e_hdr.e_version:  "    << hdr.e_version  << '\n' ;
	std::cout << "hdr.e_hdr.e_entry:    "      <<    hdr.e_entry    << '\n' ;
	std::cout << "hdr.e_hdr.e_phoff:    "      <<    hdr.e_phoff    << '\n' ;
	std::cout << "hdr.e_hdr.e_shoff:    "      <<    hdr.e_shoff    << '\n' ;
	std::cout << "hdr.e_hdr.e_flags:    "      <<    hdr.e_flags    << '\n' ;
	std::cout << "hdr.e_hdr.e_ehsize:   "     <<   hdr.e_ehsize   << '\n' ;
	std::cout << "hdr.e_hdr.e_phentsiz: "   << hdr.e_phentsize<< '\n' ;
	std::cout << "hdr.e_hdr.e_phnum:    "  << hdr.e_phnum    << '\n' ;
	std::cout << "hdr.e_hdr.e_shentsiz: "  << hdr.e_shentsize<< '\n' ;
	std::cout << "hdr.e_hdr.e_shnum:    "  << hdr.e_shnum    << '\n' ;
	std::cout << "hdr.e_hdr.e_shstrndx: "  << hdr.e_shstrndx << '\n' ;
	std::cout << "============\n\n";
}

void print_program_header(const elf64_phdr & header)
{
	std::cout << "p_type:       " << header.p_type  << '\n'  ;
  	std::cout << "p_flags:      " << header.p_flags << '\n'  ;
  	std::cout << "p_offset:     " << header.p_offset << '\n'  ;
  	std::cout << "p_vaddr:      " << header.p_vaddr  << '\n'  ;
  	std::cout << "p_paddr:      " << header.p_paddr  << '\n'  ;
  	std::cout << "p_filesz:     " << header.p_filesz << '\n'  ;
  	std::cout << "p_memsz:      " << header.p_memsz << '\n'  ;
  	std::cout << "p_align:      " << header.p_align << '\n'  ;
}

void print_section_header(const elf64_shdr & header)
{
	std::cout << "\tsh_name:        " << header.sh_name << '\n'  ;
  	std::cout << "\tsh_type:        " << sh_rela_type_to_str(header.sh_type) << '\n'  ;
  	std::cout << "\tsh_flags:       " << header.sh_flags<< '\n'  ;
  	std::cout << "\tsh_addr:        " << header.sh_addr << '\n'  ;
  	std::cout << "\tsh_offset:      " << header.sh_offset << '\n'  ;
  	std::cout << "\tsh_size:        " << header.sh_size << '\n'  ;
  	std::cout << "\tsh_link:        " << header.sh_link << '\n'  ;
  	std::cout << "\tsh_info:        " << header.sh_info << '\n'  ;
  	std::cout << "\tsh_addralign:   " << header.sh_addralign << '\n';
  	std::cout << "\tsh_entsize:     " << header.sh_entsize << '\n';
}


const char* dynamic_tag_to_str(DynamicTag tag)
{
	switch (tag)
	{
		case Null:
			return "Null";
		case NEEDED:
			return "needed";
		case PLTRELSZ:
			return "pltrelsz";
		case PLTGOT:
			return "pltgot";
		case HASH:
			return "hash";
		case STRTAB:
			return "strtab";
		case SYMTAB:
			return "symtab";
		case RELA:
			return "rela";
		case RELASZ:
			return "relasz";
		case RELAENT:
			return "relaent";
		case STRSZ:
			return "strsz";
		case SYMENT:
			return "syment";
		case INIT:
			return "init";
		case FINI:
			return "fini";
		case SONAME:
			return "soname";
		case RPATH:
			return "rpath";
		case SYMBOLIC:
			return "symbolic";
		case REL:
			return "rel";
		case RELSZ:
			return "relsz";
		case RELENT:
			return "relent";
		case PLTREL:
			return "pltrel";
		case DEBUG:
			return "debug";
		case TEXTREL:
			return "textrel";
		case JMPREL:
			return "jmprel";
		case BINDNOW:
			return "bindnow";
		case INITARRAY:
			return "initarray";
		case FINIARRAY:
			return "finiarray";
		case RUNPATH:
			return "runpath";
		case INITARRAYSZ:
			return "initarraysz";
		case FINIARRAYSZ:
			return "finiarraysz";
		case FLAGS:
			return "flags";
		case LOOS:
			return "loos";
		case HIOS:
			return "hios";
		case LOPROC:
			return "loproc";
		case HIPROC:
			return "hiproc";
		case GNUHASH:
			return "gnuhash";
		case FLAGS1:
			return "flags1";
		case RELACOUNT:
			return "relacount";
		case VERDEF:
			return "verdef";
		case VERDEFNUM:
			return "verdefnum";
		case VERNEED:
			return "verneed";
		default:
			break;
	}
	return "Unknown";
}

const char* rela_type_to_str(RelaType type)
{
	switch (type)
	{
		case _64:
			return "64";
		case PC32:
			return "pc32";
		case GOT32:
			return "got32";
		case PLT32:
			return "plt32";
		case COPY:
			return "copy";
		case GLOB_DAT:
			return "glob_dat";
		case JUMP_SLOT:
			return "jump_slot";
		case RELATIVE	:
			return "relative";
	}
	return (std::string("Unknown: ") + std::to_string(type)).data();
}


/* sh_type */

const char* sh_rela_type_to_str(Elf64_Word type)
{

	switch (type)
	{

		case SHT_NULL:
			return "SHT_NULL";
		case SHT_PROGBITS:
			return "SHT_PROGBITS";
		case SHT_SYMTAB:
			return "SHT_SYMTAB";
		case SHT_STRTAB:
			return "SHT_STRTAB";
		case SHT_RELA:
			return "SHT_RELA";
		case SHT_HASH:
			return "SHT_HASH";
		case SHT_DYNAMIC:
			return "SHT_DYNAMIC";
		case SHT_NOTE:
			return "SHT_NOTE";
		case SHT_NOBITS:
			return "SHT_NOBITS";
		case SHT_REL:
			return "SHT_REL";
		case SHT_SHLIB:
			return "SHT_SHLIB";
		case SHT_DYNSYM:
			return "SHT_DYNSYM";
		case SHT_NUM:
			return "SHT_NUM";
		case 0x70000000:
			return "SHT_LOPROC";
		case 0x7fffffff:
			return "SHT_HIPROC";
		case 0x80000000:
			return "SHT_LOUSER";
		case 0xffffffff:
			return "SHT_HIUSER";
		default:
			return "Uknown";
	}
	return nullptr;
}

void print_rela(const elf64_rela & rel)
{
	uint32_t sym = (uint64_t)(rel.r_info) >> 32;
	RelaType type = (RelaType)(rel.r_info & 0x00000000FFFFFFFF);
	printf("Rela {\n"
	        "\toffset: %.8lx,\n"
	        "\ttype: %s,\n"
	        "\tsym: %u,\n"
	        "\taddend: %.8lx,\n"
	        "}\n", rel.r_offset, rela_type_to_str(type), sym, rel.r_addend);
}
