#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <zlib.h>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <cstdint>

// --- Base64 encoding utility ---
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::vector<unsigned char>& bytes_to_encode) {
    std::string ret;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t in_len = bytes_to_encode.size();
    size_t pos = 0;

    while (in_len--) {
        char_array_3[i++] = bytes_to_encode[pos++];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

// --- ZLIB compression utility ---
std::vector<unsigned char> compress_zlib(const std::string& str) {
    z_stream zs{};
    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK)
        throw std::runtime_error("deflateInit failed");

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    std::vector<unsigned char> outbuffer;
    unsigned char temp[32768];

    do {
        zs.next_out = temp;
        zs.avail_out = sizeof(temp);

        ret = deflate(&zs, zs.avail_in ? Z_NO_FLUSH : Z_FINISH);

        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&zs);
            throw std::runtime_error("deflate failed");
        }

        size_t have = sizeof(temp) - zs.avail_out;
        outbuffer.insert(outbuffer.end(), temp, temp + have);
    } while (ret != Z_STREAM_END);

    deflateEnd(&zs);
    return outbuffer;
}

// Utility to remove all whitespace
std::string remove_whitespace(const std::string& str) {
    std::string out;
    for (char c : str) {
        if (!isspace(static_cast<unsigned char>(c))) {
            out += c;
        }
    }
    return out;
}

// Parse GON table into a map of key->value (flat, not recursive)
std::map<std::string, std::string> parse_gon_table(const std::string& gon) {
    std::map<std::string, std::string> result;
    size_t i = 0;
    // Find opening '{'
    while (i < gon.size() && gon[i] != '{') ++i;
    ++i;
    while (i < gon.size()) {
        // Skip whitespace
        while (i < gon.size() && isspace(gon[i])) ++i;
        if (i >= gon.size() || gon[i] == '}') break;
        // Parse key
        size_t key_start = i;
        while (i < gon.size() && gon[i] != '=') ++i;
        if (i >= gon.size()) break;
        std::string key = gon.substr(key_start, i - key_start);
        ++i; // skip '='
        // Parse value (brace depth for nested tables)
        int brace = 0;
        size_t value_start = i;
        while (i < gon.size()) {
            if (gon[i] == '{') ++brace;
            else if (gon[i] == '}') {
                if (brace == 0) break;
                --brace;
            } else if (gon[i] == ';' && brace == 0) break;
            ++i;
        }
        std::string value = gon.substr(value_start, i - value_start);
        result[key] = value;
        // Skip to next entry
        while (i < gon.size() && gon[i] != ';' && gon[i] != '}') ++i;
        if (i < gon.size() && gon[i] == ';') ++i;
    }
    return result;
}

// Rebuild GON table from map
std::string build_gon_table(const std::map<std::string, std::string>& table) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& kv : table) {
        if (!first) oss << ";";
        oss << kv.first << "=" << kv.second;
        first = false;
    }
    oss << "}";
    return oss.str();
}

// Build laser cell GON string for a given binary code
std::string build_laser_cell(int blue, int green) {
    std::ostringstream oss;
    // n1=0 (red), n2=green if given else 32, n3=blue if given else 0
    oss << "{srot=n1;sactivityclr={};sclr={n1=n0;n2=n" << (green > 0 ? green : 32) << ";n3=n" << (blue > 0 ? blue : 0) << ";};sactivity={};sid=semitter;}";
    return oss.str();
}

int main() {
    // 1. Read 256 16-bit binary codes from machinecode.txt
    std::ifstream mcfile("machinecode.txt");
    std::vector<uint16_t> codes;
    std::string line;
    while (std::getline(mcfile, line)) {
        if (line.size() >= 16) {
            codes.push_back(static_cast<uint16_t>(std::stoi(line.substr(0, 16), nullptr, 2)));
        }
    }
    while (codes.size() < 256) codes.push_back(0);

    // 2. Read photon save string from instMem.txt
    std::ifstream instfile("instMem.txt");
    std::string instMem((std::istreambuf_iterator<char>(instfile)), std::istreambuf_iterator<char>());
    instfile.close();

    // 3. Parse header and sections
    size_t p1 = instMem.find(';');
    size_t p2 = instMem.find(';', p1 + 1);
    size_t p3 = instMem.find(';', p2 + 1);
    size_t p4 = instMem.find(';', p3 + 1);
    size_t p5 = instMem.find(';', p4 + 1);

    std::string header = instMem.substr(0, p3 + 1);
    std::string cells_b64 = instMem.substr(p3 + 1, p4 - p3 - 1);
    std::string lasers_b64 = instMem.substr(p4 + 1, p5 - p4 - 1);

    // 4. Decompress cells section
    // --- base64 decode ---
    auto base64_decode = [](const std::string& encoded_string) -> std::vector<unsigned char> {
        int in_len = encoded_string.size();
        int i = 0, j = 0, in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;
        while (in_len-- && (encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || encoded_string[in_] == '+' || encoded_string[in_] == '/')) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                for (i = 0; i < 3; i++) ret.push_back(char_array_3[i]);
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; j++) char_array_4[j] = 0;
            for (j = 0; j < 4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (j = 0; j < (i - 1); j++) ret.push_back(char_array_3[j]);
        }
        return ret;
    };

    auto decompress_zlib = [](const std::vector<unsigned char>& data) -> std::string {
        z_stream zs{};
        zs.avail_in = data.size();
        zs.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(data.data()));
        if (inflateInit(&zs) != Z_OK)
            throw std::runtime_error("inflateInit failed");
        int ret;
        char outbuffer[32768];
        std::string outstring;
        do {
            zs.avail_out = sizeof(outbuffer);
            zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
            ret = inflate(&zs, 0);
            if (outstring.size() < zs.total_out)
                outstring.append(outbuffer, zs.total_out - outstring.size());
        } while (ret == Z_OK);
        inflateEnd(&zs);
        if (ret != Z_STREAM_END)
            throw std::runtime_error("Exception during zlib decompression");
        return outstring;
    };

    std::string cells_gon = decompress_zlib(base64_decode(cells_b64));

    // 5. Parse cells GON table
    auto cells_map = parse_gon_table(cells_gon);

    // 6. For each code, add/replace laser at s7 1, s9 2, ..., s(2*i+7) i+1
    for (int i = 0; i < 256; ++i) {
        int green = (codes[i] >> 8) & 0xFF; // swapped: green is high byte
        int blue = codes[i] & 0xFF;         // blue is low byte
        std::ostringstream key;
        key << "s" << (2 * i + 7) << " " << (i + 1);
        cells_map[key.str()] = build_laser_cell(blue, green);
    }

    // 7. Rebuild cells GON string
    std::string new_cells_gon = build_gon_table(cells_map);

    // 8. Compress and encode
    auto compressed_cells = compress_zlib(new_cells_gon);
    std::string encoded_cells = base64_encode(compressed_cells);

    // 9. Write new .phot2 file
    std::ofstream out("instMem.phot2");
    out << header << encoded_cells << ";" << lasers_b64 << ";" << std::endl;
    out.close();

    std::cout << "instMem.phot2 written with updated lasers." << std::endl;
    return 0;
}
