#include "CEExporter.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
    std::string escapeXml(const std::string& str) {
        std::string out;
        out.reserve(str.size());
        for (char c : str) {
            switch (c) {
            case '&': out += "&amp;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            default: out += c; break;
            }
        }
        return out;
    }
}

void CEExporter::exportToCheatEngine(const EngineStructs::Struct& s, const std::string& filePath) {
    std::ostringstream xml;
    xml << "<CheatTable><Structures>";
    xml << "<Structure Name=\"" << escapeXml(s.cppName) << "\"><Elements>";
    for (const auto& member : s.definedMembers) {
        xml << "<Element Offset=\"" << member.offset << "\" Description=\"" << escapeXml(member.name)
            << "\" Type=\"" << escapeXml(member.type.name) << "\"/>";
    }
    xml << "</Elements></Structure></Structures></CheatTable>";

    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(filePath, std::ios::binary);
    out << xml.str();
}

