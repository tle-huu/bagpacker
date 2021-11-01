#pragma once

#include "elf_structures.h"

#include <elf.h>
#include <utility>

void print_elf_header(struct elf64_hdr const & header);
void print_program_header(const elf64_phdr & header);
void print_section_header(const elf64_shdr & header);

const char* dynamic_tag_to_str(DynamicTag tag);
const char* rela_type_to_str(RelaType type);
const char* sh_rela_type_to_str(Elf64_Word type);
void print_rela(const elf64_rela & rel);
