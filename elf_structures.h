#pragma once

#include <elf.h>

struct elf64_hdr {
	unsigned char    e_ident[EI_NIDENT];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
};

struct elf64_phdr {
	Elf64_Word p_type;
	Elf64_Word p_flags;
	Elf64_Off p_offset;		/* Segment file offset */
	Elf64_Addr p_vaddr;		/* Segment virtual address */
	Elf64_Addr p_paddr;		/* Segment physical address */
	Elf64_Xword p_filesz;		/* Segment size in file */
	Elf64_Xword p_memsz;		/* Segment size in memory */
	Elf64_Xword p_align;		/* Segment alignment, file & memory */
};

struct elf64_shdr {
	Elf64_Word sh_name;		/* Section name, index in string tbl */
	Elf64_Word sh_type;		/* Type of section */
	Elf64_Xword sh_flags;		/* Miscellaneous section attributes */
	Elf64_Addr sh_addr;		/* Section virtual addr at execution */
	Elf64_Off sh_offset;		/* Section file offset */
	Elf64_Xword sh_size;		/* Size of section in bytes */
	Elf64_Word sh_link;		/* Index of another section */
	Elf64_Word sh_info;		/* Additional section information */
	Elf64_Xword sh_addralign;	/* Section alignment */
	Elf64_Xword sh_entsize;	/* Entry size if section holds table */
};

struct elf64_sym {
	Elf64_Word st_name;		/* Symbol name, index in string tbl */
	unsigned char	st_info;	/* Type and binding attributes */
	unsigned char	st_other;	/* No defined meaning, 0 */
	Elf64_Half st_shndx;		/* Associated section index */
	Elf64_Addr st_value;		/* Value of the symbol */
	Elf64_Xword st_size;		/* Associated symbol size */
};

struct elf64_dyn {
	Elf64_Sxword d_tag;		/* entry tag value */
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
};

struct elf64_rela {
	Elf64_Addr r_offset;		/* Location at which to apply the action */
	Elf64_Xword r_info;		/* index and type of relocation */
	Elf64_Sxword r_addend;	/* Constant addend used to compute value */
};

enum DynamicTag : long int
{
	Null = 0,
	NEEDED = 1,
	PLTRELSZ = 2,
	PLTGOT = 3,
	HASH = 4,
	STRTAB = 5,
	SYMTAB = 6,
	RELA = 7,
	RELASZ = 8,
	RELAENT = 9,
	STRSZ = 10,
	SYMENT = 11,
	INIT = 12,
	FINI = 13,
	SONAME = 14,
	RPATH = 29,
	SYMBOLIC = 16,
	REL = 17,
	RELSZ = 18,
	RELENT = 19,
	PLTREL = 20,
	DEBUG = 21,
	TEXTREL = 22,
	JMPREL = 23,
	BINDNOW = 24,
	INITARRAY = 25,
	FINIARRAY = 26,
	INITARRAYSZ = 27,
	FINIARRAYSZ = 28,
	FLAGS = 30,
	LOOS = 0X60000000,
	LOPROC = 0X70000000,
	HIPROC = 0X7FFFFFFF,
	GNUHASH = 0X6FFFFEF5,
	FLAGS1 = 0X6FFFFFFB,
	RELACOUNT = 0X6FFFFFF9,


    // AS IT TURNS OUT, `LOOS` AND `HIOS` WERE THE MINIMUM AND MAXIMUM VALUES
    // FOR OPERATING SYSTEM SPECIFIC TAGS. AND THERE'S A BUNCH OF THOSE..
	VERDEF = 0X6FFFFFFC,
	VERDEFNUM = 0X6FFFFFFD,
	VERNEED = 0X6FFFFFFE,
	HIOS = 0X6FFFFFFF,
};

enum RelaType : uint32_t
{
	None,
	_64,
	PC32,
	GOT32,
	PLT32,
	COPY,
	GLOB_DAT,
	JUMP_SLOT,
	RELATIVE,
};