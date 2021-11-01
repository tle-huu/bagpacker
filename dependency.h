#pragma once

#include "mapped_zone.h"

struct Dependency
{
        uintptr_t offset; // strtab offset
        char *name;
        MappedZone base_zone;
};