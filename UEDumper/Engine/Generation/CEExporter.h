#pragma once
#include "stdafx.h"
#include "Engine/Core/EngineStructs.h"

namespace CEExporter {
    void exportToCheatEngine(const EngineStructs::Struct& s, const std::string& filePath);
}

