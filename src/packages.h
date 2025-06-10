//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// packages.h - Track and display xbps packages
//

#pragma once

#include <stddef.h>

typedef struct
{
    char *status;
    char *name;
    char *description;
} LPM_Package;

typedef struct
{
    LPM_Package *items;
    size_t count;
    size_t capacity;
} LPM_Packages;

void lpm_packages_teardown(LPM_Packages *pkgs);
int lpm_packages_get(LPM_Packages *pkgs, const char *cmd);
