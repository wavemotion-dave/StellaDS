// =====================================================================================================
// Stella DSi - Improved Version by Dave Bernazzani (wavemotion)
//
// See readme.txt for a list of everything that has changed in the baseline 1.0 code.
// =====================================================================================================
#ifndef __CONFIG_H
#define __CONFIG_H

#include <nds.h>
#include "Cart.hxx"

// ---------------------------
// Config handling...
// ---------------------------
#define CONFIG_VER  0x0006

#define MAX_CONFIGS 1300

struct AllConfig_t
{
    uInt16                  config_ver;
    struct CartInfo         cart[MAX_CONFIGS];
    uInt32                  crc32;
};

extern struct AllConfig_t allConfigs;

void LoadConfig(void);
void ShowConfig(void);
void SaveConfig(bool bShow);

#endif
