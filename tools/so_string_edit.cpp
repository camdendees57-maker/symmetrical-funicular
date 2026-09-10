#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

enum {
    ID_OPEN=10, ID_SAVE=11, ID_LIST=12, ID_FIND=13, ID_REPL=14, ID_ZERO=15, ID_XOR=16,
    ID_OLD=20, ID_NEW=21, ID_BOX=22, ID_STAT=23
};

static std::vector<uint8_t> g_bin;
static std::wstring g_path;
static HWND g_box, g_old, g_new, g_stat;

static std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}
static std::wstring gettxt(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring w(n, 0);
    if (n) GetWindowTextW(h, &w[0], n + 1);
    return w;
}
static void status(const std::wstring& s) { SetWindowTextW(g_stat, s.c_str()); }
static bool printable(uint8_t c) { return c >= 0x20 && c < 0x7f; }

static bool load_file(const std::wstring& p) {
    std::ifstream f(p.c_str(), std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    auto n = (size_t)f.tellg();
    f.seekg(0);
    g_bin.resize(n);
    f.read((char*)g_bin.data(), (std::streamsize)n);
    g_path = p;
    return true;
}
static bool save_file(const std::wstring& p) {
    std::ofstream f(p.c_str(), std::ios::binary);
    if (!f) return false;
    f.write((const char*)g_bin.data(), (std::streamsize)g_bin.size());
    return true;
}
static std::wstring pick(bool save) {
    wchar_t buf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"Shared objects (*.so)\0*.so\0All\0*.*\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    if (save) return GetSaveFileNameW(&ofn) ? buf : L"";
    ofn.Flags |= OFN_FILEMUSTEXIST;
    return GetOpenFileNameW(&ofn) ? buf : L"";
}
static std::vector<uint8_t> bytes_of(const std::string& s) { return {s.begin(), s.end()}; }
static std::vector<uint8_t> utf16le(const std::string& s) {
    std::vector<uint8_t> o; o.reserve(s.size()*2);
    for (unsigned char c : s) { o.push_back(c); o.push_back(0); }
    return o;
}
static std::vector<size_t> find_bytes(const std::vector<uint8_t>& b, const std::vector<uint8_t>& n) {
    std::vector<size_t> h;
    if (n.empty() || n.size() > b.size()) return h;
    for (size_t i = 0; i + n.size() <= b.size(); ++i)
        if (!memcmp(b.data()+i, n.data(), n.size())) h.push_back(i);
    return h;
}
static int patch_at(size_t off, const std::vector<uint8_t>& oldv, const std::vector<uint8_t>& nv, bool z) {
    if (off + oldv.size() > g_bin.size()) return 0;
    if (memcmp(g_bin.data()+off, oldv.data(), oldv.size())) return 0;
    if (z) { std::fill(g_bin.begin()+off, g_bin.begin()+off+oldv.size(), 0); return 1; }
    if (nv.size() > oldv.size()) return 0;
    std::copy(nv.begin(), nv.end(), g_bin.begin()+off);
    std::fill(g_bin.begin()+off+nv.size(), g_bin.begin()+off+oldv.size(), 0);
    return 1;
}
static void addline(const std::wstring& s) {
    SendMessageW(g_box, LB_ADDSTRING, 0, (LPARAM)s.c_str());
}
static void clearbox() { SendMessageW(g_box, LB_RESETCONTENT, 0, 0); }

static void do_list() {
    clearbox();
    if (g_bin.empty()) { status(L"open a .so first"); return; }
    int n = 0;
    size_t i = 0;
    while (i < g_bin.size()) {
        if (!printable(g_bin[i])) { ++i; continue; }
        size_t j = i;
        while (j < g_bin.size() && printable(g_bin[j])) ++j;
        if (j - i >= 4) {
            char line[560];
            int sl = (int)std::min<size_t>(j - i, 480);
            std::snprintf(line, sizeof(line), "0x%08zx  %.*s", i, sl, (const char*)g_bin.data() + i);
            addline(wide(line));
            ++n;
            if (n > 8000) { addline(L"...truncated"); break; }
        }
        i = j + 1;
    }
    wchar_t st[80];
    wsprintfW(st, L"listed %d strings  %u bytes", n, (unsigned)g_bin.size());
    status(st);
}
static void do_find() {
    clearbox();
    auto t = narrow(gettxt(g_old));
    if (t.empty() || g_bin.empty()) { status(L"need file + find text"); return; }
    int n = 0;
    for (auto o : find_bytes(g_bin, bytes_of(t))) {
        char line[80]; std::snprintf(line, sizeof(line), "ascii 0x%08zx", o);
        addline(wide(line)); ++n;
    }
    for (auto o : find_bytes(g_bin, utf16le(t))) {
        char line[80]; std::snprintf(line, sizeof(line), "utf16 0x%08zx", o);
        addline(wide(line)); ++n;
    }
    wchar_t st[64]; wsprintfW(st, L"hits %d", n); status(st);
}
static void do_patch(bool z) {
    auto olds = narrow(gettxt(g_old));
    auto news = narrow(gettxt(g_new));
    if (g_bin.empty() || olds.empty()) { status(L"need file + old text"); return; }
    if (!z && news.size() > olds.size()) { status(L"new longer than old slot"); return; }
    int hits = 0;
    auto ov = bytes_of(olds), nv = bytes_of(news);
    for (auto o : find_bytes(g_bin, ov)) hits += patch_at(o, ov, nv, z);
    auto ow = utf16le(olds), nw = utf16le(news);
    for (auto o : find_bytes(g_bin, ow)) hits += patch_at(o, ow, nw, z);
    wchar_t st[80]; wsprintfW(st, L"%s %d slot(s)  SAVE to write", z ? L"zeroed" : L"patched", hits);
    status(st);
}
static void do_xor() {
    clearbox();
    auto t = narrow(gettxt(g_old));
    auto needle = bytes_of(t);
    if (g_bin.empty() || needle.size() < 3) { status(L"need file + find text (>=3)"); return; }
    int shown = 0;
    for (int klen = 1; klen <= 8; ++klen) {
        for (size_t i = 0; i + needle.size() <= g_bin.size(); ++i) {
            std::vector<uint8_t> key(klen);
            bool ok = true;
            for (size_t n = 0; n < needle.size(); ++n) {
                uint8_t kn = g_bin[i + n] ^ needle[n];
                if (n < (size_t)klen) key[n] = kn;
                else if (key[n % klen] != kn) { ok = false; break; }
            }
            if (!ok) continue;
            int good = 0, tot = 0;
            size_t lo = i > 16 ? i - 16 : 0;
            size_t hi = std::min(g_bin.size(), i + needle.size() + 16);
            for (size_t p = lo; p < hi; ++p) {
                uint8_t d = g_bin[p] ^ key[(p - i + (size_t)klen * 8) % klen];
                ++tot; if (printable(d) || d == 0) ++good;
            }
            if (good * 4 < tot * 3) continue;
            char line[96];
            int w = std::snprintf(line, sizeof(line), "xor 0x%08zx keylen=%d key=", i, klen);
            for (int k = 0; k < klen && w < 80; ++k) w += std::snprintf(line + w, sizeof(line) - w, "%02x", key[k]);
            addline(wide(line));
            if (++shown > 400) { addline(L"...truncated"); status(L"xor hits truncated"); return; }
        }
    }
    wchar_t st[64]; wsprintfW(st, L"xor hits %d (repeat XOR only)", shown); status(st);
}

static LRESULT CALLBACK Wnd(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        CreateWindowW(L"BUTTON", L"OPEN .SO", WS_CHILD|WS_VISIBLE, 12,12,110,28, h, (HMENU)ID_OPEN, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"LIST", WS_CHILD|WS_VISIBLE, 128,12,80,28, h, (HMENU)ID_LIST, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"FIND", WS_CHILD|WS_VISIBLE, 214,12,80,28, h, (HMENU)ID_FIND, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"REPLACE", WS_CHILD|WS_VISIBLE, 300,12,90,28, h, (HMENU)ID_REPL, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"ZERO", WS_CHILD|WS_VISIBLE, 396,12,80,28, h, (HMENU)ID_ZERO, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"XORFIND", WS_CHILD|WS_VISIBLE, 482,12,90,28, h, (HMENU)ID_XOR, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"SAVE AS", WS_CHILD|WS_VISIBLE, 578,12,90,28, h, (HMENU)ID_SAVE, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"find/old", WS_CHILD|WS_VISIBLE, 12,50,70,20, h, 0, nullptr, nullptr);
        g_old = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 86,46,280,24, h, (HMENU)ID_OLD, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"new", WS_CHILD|WS_VISIBLE, 376,50,40,20, h, 0, nullptr, nullptr);
        g_new = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 416,46,252,24, h, (HMENU)ID_NEW, nullptr, nullptr);
        g_box = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY, 12,80,656,360, h, (HMENU)ID_BOX, nullptr, nullptr);
        g_stat = CreateWindowW(L"STATIC", L"open a .so", WS_CHILD|WS_VISIBLE, 12,450,656,22, h, (HMENU)ID_STAT, nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case ID_OPEN: {
            auto p = pick(false);
            if (p.empty()) break;
            if (!load_file(p)) { status(L"open failed"); break; }
            status(L"loaded " + p);
            do_list();
            break;
        }
        case ID_SAVE: {
            if (g_bin.empty()) { status(L"nothing loaded"); break; }
            auto p = pick(true);
            if (p.empty()) break;
            status(save_file(p) ? L"wrote " + p : L"save failed");
            break;
        }
        case ID_LIST: do_list(); break;
        case ID_FIND: do_find(); break;
        case ID_REPL: do_patch(false); break;
        case ID_ZERO: do_patch(true); break;
        case ID_XOR: do_xor(); break;
        case ID_BOX:
            if (HIWORD(w) == LBN_DBLCLK) {
                int i = (int)SendMessageW(g_box, LB_GETCURSEL, 0, 0);
                if (i >= 0) {
                    wchar_t buf[600];
                    SendMessageW(g_box, LB_GETTEXT, i, (LPARAM)buf);
                    wchar_t* s = wcsstr(buf, L"  ");
                    if (s) SetWindowTextW(g_old, s + 2);
                }
            }
            break;
        }
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = Wnd;
    wc.hInstance = inst;
    wc.lpszClassName = L"TagtusSoEdit";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    HWND h = CreateWindowW(L"TagtusSoEdit", L"TAGTUSVR SO STRING EDIT", WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 696, 520, nullptr, nullptr, inst, nullptr);
    ShowWindow(h, show);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    return 0;
}

int WINAPI WinMain(HINSTANCE i, HINSTANCE p, LPSTR, int s) { return wWinMain(i, p, nullptr, s); }
