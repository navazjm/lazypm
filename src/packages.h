//
// Copyright (c) 2025 Michael Navarro
// MIT license, see LICENSE for more
//
// packages.h - Track and display xbps packages
//

#pragma once

#include "common.h"
#include "logs.h"
#include "status.h"

#define LPM_PACKAGE_STATUS_INSTALLED "[*]"
#define LPM_PACKAGE_STATUS_AVAILABLE "[-]"

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
LPM_Exit_Code lpm_packages_get(LPM_Packages *pkgs, const char *pkg_name);
LPM_Exit_Code lpm_packages_install(LPM_Package *pkg);
LPM_Exit_Code lpm_packages_update_all(void);
LPM_Exit_Code lpm_packages_uninstall(LPM_Package *pkg);
LPM_Exit_Code lpm_packages_update_xbps(void);
