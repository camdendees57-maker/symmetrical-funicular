#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void die(const char* m) {
    std::cerr << m << "\n";
    std::exit(1);
}

static std::vector<uint8_t> slurp(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) die("open failed");
    f.seekg(0, std::ios::end);
    auto n = (size_t)f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> b(n);
    f.read((char*)b.data(), (std::streamsize)n);
    return b;
}

static void dump(const std::string& p, const std::vector<uint8_t>& b) {
    std::ofstream f(p, std::ios::binary);
    if (!f) die("write failed");
    f.write((const char*)b.data(), (std::streamsize)b.size());
}

static bool printable(uint8_t c) {
    return c >= 0x20 && c < 0x7f;
}

static std::vector<size_t> find_bytes(const std::vector<uint8_t>& b, const std::vector<uint8_t>& n) {
    std::vector<size_t> hits;
    if (n.empty() || n.size() > b.size()) return hits;
    for (size_t i = 0; i + n.size() <= b.size(); ++i) {
        if (std::memcmp(b.data() + i, n.data(), n.size()) == 0) hits.push_back(i);
    }
    return hits;
}

static std::vector<uint8_t> to_utf16le(const std::string& s) {
    std::vector<uint8_t> o;
    o.reserve(s.size() * 2);
    for (unsigned char c : s) {
        o.push_back(c);
        o.push_back(0);
    }
    return o;
}

static std::vector<uint8_t> bytes_of(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static int patch_at(std::vector<uint8_t>& b, size_t off, const std::vector<uint8_t>& oldv, const std::vector<uint8_t>& nv, bool zero) {
    if (off + oldv.size() > b.size()) return 0;
    if (std::memcmp(b.data() + off, oldv.data(), oldv.size()) != 0) return 0;
    if (zero) {
        std::fill(b.begin() + off, b.begin() + off + oldv.size(), 0);
        return 1;
    }
    if (nv.size() > oldv.size()) {
        std::cerr << "skip 0x" << std::hex << off << std::dec << " new longer than slot\n";
        return 0;
    }
    std::copy(nv.begin(), nv.end(), b.begin() + off);
    std::fill(b.begin() + off + nv.size(), b.begin() + off + oldv.size(), 0);
    return 1;
}

static void list_ascii(const std::vector<uint8_t>& b, int minlen) {
    size_t i = 0;
    while (i < b.size()) {
        if (!printable(b[i])) { ++i; continue; }
        size_t j = i;
        while (j < b.size() && printable(b[j])) ++j;
        if ((int)(j - i) >= minlen) {
            std::printf("0x%08zx  %.*s\n", i, (int)(j - i), (const char*)b.data() + i);
        }
        i = j + 1;
    }
}

/* 1-byte XOR and repeating XOR key 1..8 if needle decrypts to printable match */
static void xor_hunt(const std::vector<uint8_t>& b, const std::vector<uint8_t>& needle) {
    if (needle.size() < 3) return;
    std::printf("xor hunt needle_len=%zu\n", needle.size());
    for (int klen = 1; klen <= 8; ++klen) {
        for (size_t i = 0; i + needle.size() <= b.size(); ++i) {
            std::vector<uint8_t> key(klen);
            bool ok = true;
            for (size_t n = 0; n < needle.size(); ++n) {
                uint8_t kn = b[i + n] ^ needle[n];
                if (n < (size_t)klen) key[n] = kn;
                else if (key[n % klen] != kn) { ok = false; break; }
            }
            if (!ok) continue;
            /* score: nearby bytes xor to printable */
            int good = 0, tot = 0;
            size_t lo = i > 16 ? i - 16 : 0;
            size_t hi = std::min(b.size(), i + needle.size() + 16);
            for (size_t p = lo; p < hi; ++p) {
                uint8_t d = b[p] ^ key[(p - i + (size_t)klen * 8) % klen];
                ++tot;
                if (printable(d) || d == 0) ++good;
            }
            if (good * 4 < tot * 3) continue;
            std::printf("xor hit off=0x%08zx keylen=%d key=", i, klen);
            for (int k = 0; k < klen; ++k) std::printf("%02x", key[k]);
            std::printf("\n");
        }
    }
}

static void usage() {
    std::cerr <<
        "so_string_edit.exe  list    <in.so> [minlen]\n"
        "so_string_edit.exe  find    <in.so> <text>\n"
        "so_string_edit.exe  replace <in.so> <out.so> <old> <new>\n"
        "so_string_edit.exe  zero    <in.so> <out.so> <text>\n"
        "so_string_edit.exe  xorfind <in.so> <text>\n"
        "notes: replace/zero also hit UTF-16LE. new must fit old slot.\n"
        "xorfind only brute 1-8 byte repeating XOR. not AES/RSA.\n";
}

int main(int argc, char** argv) {
    if (argc < 3) { usage(); return 1; }
    std::string cmd = argv[1];
    auto bin = slurp(argv[2]);

    if (cmd == "list") {
        int minlen = argc > 3 ? std::atoi(argv[3]) : 4;
        list_ascii(bin, minlen);
        return 0;
    }
    if (cmd == "find" && argc >= 4) {
        auto n = bytes_of(argv[3]);
        auto w = to_utf16le(argv[3]);
        for (auto o : find_bytes(bin, n)) std::printf("ascii 0x%08zx\n", o);
        for (auto o : find_bytes(bin, w)) std::printf("utf16 0x%08zx\n", o);
        return 0;
    }
    if ((cmd == "replace" && argc >= 6) || (cmd == "zero" && argc >= 5)) {
        std::string outp = argv[3];
        auto oldv = bytes_of(argv[4]);
        auto nv = (cmd == "replace") ? bytes_of(argv[5]) : std::vector<uint8_t>{};
        bool z = cmd == "zero";
        int hits = 0;
        for (auto o : find_bytes(bin, oldv)) hits += patch_at(bin, o, oldv, nv, z);
        auto oldw = to_utf16le(argv[4]);
        auto neww = z ? std::vector<uint8_t>{} : to_utf16le(argv[5]);
        for (auto o : find_bytes(bin, oldw)) hits += patch_at(bin, o, oldw, neww, z);
        dump(outp, bin);
        std::printf("patched %d slot(s) -> %s\n", hits, outp.c_str());
        return hits ? 0 : 2;
    }
    if (cmd == "xorfind" && argc >= 4) {
        xor_hunt(bin, bytes_of(argv[3]));
        return 0;
    }
    usage();
    return 1;
}
