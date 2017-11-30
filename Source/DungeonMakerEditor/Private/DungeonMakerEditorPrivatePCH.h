#pragma once

#include "DungeonMakerGraph.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.
#include "IDungeonMakerEditor.h"

#define LOG_WARNING(FMT, ...) UE_LOG(DungeonMakerEditor, Warning, (FMT), ##__VA_ARGS__)
