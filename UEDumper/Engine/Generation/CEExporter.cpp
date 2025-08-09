#include "CEExporter.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>

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

    std::string toHex(uint64_t value) {
        std::ostringstream ss;
        ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
        return ss.str();
    }

    std::string getVartype(const EngineStructs::Member& member, int elementSize) {
        if (member.type.isPointer())
            return "Pointer";

        switch (member.type.propertyType) {
        case PropertyType::FloatProperty: return "Float";
        case PropertyType::DoubleProperty: return "Double";
        case PropertyType::StrProperty:
        case PropertyType::TextProperty:
        case PropertyType::NameProperty: return "String";
        default: break;
        }

        switch (elementSize) {
        case 1: return "Byte";
        case 2: return "2 Bytes";
        case 4: return "4 Bytes";
        case 8: return "8 Bytes";
        default: return "Byte";
        }
    }
}

void CEExporter::exportToCheatEngine(const EngineStructs::Struct& s, const std::string& filePath) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    xml << "<CheatTable CheatEngineTableVersion=\"46\">\n";
    xml << "  <CheatEntries/>\n";
    xml << "  <UserdefinedSymbols/>\n";
    xml << "  <Structures StructVersion=\"2\">\n";
    xml << "    <Structure Name=\"" << escapeXml(s.cppName)
        << "\" AutoFill=\"0\" AutoCreate=\"1\" DefaultHex=\"0\" AutoDestroy=\"0\" DoNotSaveLocal=\"0\" RLECompression=\"1\" AutoCreateStructsize=\"4096\">\n";
    xml << "      <Elements>\n";

    for (const auto& member : s.definedMembers) {
        int arrayCount = member.arrayDim > 1 ? member.arrayDim : 1;
        int elementSize = arrayCount ? member.size / arrayCount : member.size;
        xml << "        <Element Offset=\"" << member.offset << "\""
            << " Vartype=\"" << getVartype(member, elementSize) << "\""
            << " Bytesize=\"" << elementSize << "\"";
        if (arrayCount > 1)
            xml << " RLECount=\"" << arrayCount << "\"";
        xml << " OffsetHex=\"" << toHex(member.offset) << "\""
            << " Description=\"" << escapeXml(member.name) << "\""
            << " DisplayMethod=\"unsigned integer\"/>\n";
    }

    xml << "      </Elements>\n";
    xml << "    </Structure>\n";
    xml << "  </Structures>\n";
    xml << "</CheatTable>\n";

    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(filePath, std::ios::binary);
    out << xml.str();
}

