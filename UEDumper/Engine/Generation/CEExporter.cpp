#include "CEExporter.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include "Frontend/Windows/LogWindow.h"

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

    std::string toLower(const std::string& str) {
        std::string out = str;
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    std::string getVartype(const EngineStructs::Member& member) {
        if (member.type.isPointer())
            return "Pointer";

        const std::string lower = toLower(member.type.name);
        if (lower == "bool" || lower == "int8" || lower == "uint8" || lower == "char" || lower == "unsigned char")
            return "Byte";
        if (lower == "int16" || lower == "uint16" || lower == "short" || lower == "unsigned short")
            return "2 Bytes";
        if (lower == "int32" || lower == "uint32" || lower == "int" || lower == "unsigned int")
            return "4 Bytes";
        if (lower == "int64" || lower == "uint64" || lower == "long long" || lower == "unsigned long long")
            return "8 Bytes";
        if (lower == "float")
            return "Float";
        if (lower == "double")
            return "Double";
        if (lower.find("string") != std::string::npos || lower == "fname")
            return "String";

        int elementSize = member.size;
        if (member.arrayDim > 1 && elementSize > 0)
            elementSize /= member.arrayDim;
        switch (elementSize) {
        case 1: return "Byte";
        case 2: return "2 Bytes";
        case 4: return "4 Bytes";
        case 8: return "8 Bytes";
        default: return "Byte";
        }
    }

    std::string getDisplayMethod(const std::string& vartype) {
        if (vartype == "Float")
            return "float";
        if (vartype == "Double")
            return "double";
        return "unsigned integer";
    }
}

void CEExporter::exportToCheatEngine(const EngineStructs::Struct& s, const std::string& filePath) {
    std::ostringstream xml;
    xml << "<Structures>";
    const std::string& name = s.cppName.empty() ? std::string("unnamed structure") : s.cppName;
    int structSize = s.maxSize > 0 ? s.maxSize : (s.size > 0 ? s.size : 4096);
    xml << "<Structure Name=\"" << escapeXml(name) << "\" AutoFill=\"0\" AutoCreate=\"1\" DefaultHex=\"0\" AutoDestroy=\"0\" DoNotSaveLocal=\"0\" RLECompression=\"1\" AutoCreateStructsize=\"" << structSize << "\"><Elements>";
    for (size_t i = 0; i < s.definedMembers.size(); ++i) {
        const auto& member = s.definedMembers[i];
        if (member.isBit) {
            int currentOffset = member.offset;
            std::vector<const EngineStructs::Member*> bits{ &member };
            size_t j = i + 1;
            while (j < s.definedMembers.size()) {
                const auto& next = s.definedMembers[j];
                if (!next.isBit || next.offset != currentOffset)
                    break;
                bits.push_back(&next);
                ++j;
            }
            int maxBit = 0;
            for (const auto* bm : bits) {
                maxBit = std::max(maxBit, bm->bitOffset);
                std::string vartype = "Byte";
                xml << "<Element Offset=\"" << currentOffset << "\"";
                xml << " Vartype=\"" << vartype << "\"";
                xml << " Bytesize=\"1\"";
                std::ostringstream offhex;
                offhex << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << currentOffset;
                xml << " OffsetHex=\"" << offhex.str() << "\"";
                xml << " DisplayMethod=\"" << getDisplayMethod(vartype) << "\"";
                if (!bm->name.empty())
                    xml << " Description=\"" << escapeXml(bm->name) << "\"";
                xml << "/>";
            }
            int bitsUsed = maxBit + 1;
            int nextOffset = (j < s.definedMembers.size()) ? s.definedMembers[j].offset : currentOffset + 1;
            if (bitsUsed < 8) {
                if (nextOffset >= currentOffset + 1)
                    windows::LogWindow::Log(windows::LogWindow::logLevels::LOGLEVEL_INFO, "CE EXPORT", "Bitfield group at offset 0x%X uses %d bits; next member at 0x%X", currentOffset, bitsUsed, nextOffset);
                else
                    windows::LogWindow::Log(windows::LogWindow::logLevels::LOGLEVEL_WARNING, "CE EXPORT", "Bitfield group at offset 0x%X uses %d bits but next member at 0x%X overlaps", currentOffset, bitsUsed, nextOffset);
            }
            i = j - 1;
            continue;
        }

        std::string vartype = getVartype(member);
        int elementSize = member.size;
        if (member.arrayDim > 1 && elementSize > 0)
            elementSize /= member.arrayDim;

        xml << "<Element Offset=\"" << member.offset << "\"";
        xml << " Vartype=\"" << vartype << "\"";
        xml << " Bytesize=\"" << elementSize << "\"";
        if (member.arrayDim > 1)
            xml << " RLECount=\"" << member.arrayDim << "\"";
        std::ostringstream offhex;
        offhex << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << member.offset;
        xml << " OffsetHex=\"" << offhex.str() << "\"";
        xml << " DisplayMethod=\"" << getDisplayMethod(vartype) << "\"";
        if (!member.name.empty())
            xml << " Description=\"" << escapeXml(member.name) << "\"";
        xml << "/>";
    }
    xml << "</Elements></Structure></Structures>";

    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(filePath, std::ios::binary);
    out << xml.str();
}

