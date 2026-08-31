#ifndef NOMINMAX
#define NOMINMAX
#endif
#define DBINITCONSTANTS
#include "WinFileSearchService.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QUrl>
#include <QImage>
#include <algorithm>
#include <string>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winsvc.h>
#include <oledb.h>
#include <oledberr.h>
#include <msdasc.h>
#include <shellapi.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// SHGetFileInfoW with SHGFI_USEFILEATTRIBUTES for native Windows Shell file/folder icons
static QString extractAndCacheShellFileIconWin(const QString &filePath, bool isDir) {
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/atlas/cache/icons";
    QDir().mkpath(cacheDir);

    const QString ext = isDir ? "folder" : QFileInfo(filePath).suffix().toLower();
    const QByteArray hash = QCryptographicHash::hash((isDir ? "folder" : ext).toUtf8(), QCryptographicHash::Md5).toHex();
    const QString iconPath = cacheDir + "/shell_" + QString::fromLatin1(hash) + ".png";

    if (!QFile::exists(iconPath)) {
        HICON hIcon = nullptr;
        SHFILEINFOW sfi = {};
        DWORD dwAttrs = isDir ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        const std::wstring target = isDir ? L"dummy" : (L"dummy." + ext.toStdWString());

        if (SHGetFileInfoW(target.c_str(), dwAttrs, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES) && sfi.hIcon) {
            hIcon = sfi.hIcon;
        }

        if (hIcon) {
            ICONINFO iconInfo = {};
            if (GetIconInfo(hIcon, &iconInfo)) {
                BITMAP bmp = {};
                if (GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp) && bmp.bmWidth > 0 && bmp.bmHeight > 0) {
                    HDC hDC = GetDC(NULL);
                    BITMAPINFO bmi = {};
                    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bmi.bmiHeader.biWidth = bmp.bmWidth;
                    bmi.bmiHeader.biHeight = -bmp.bmHeight;
                    bmi.bmiHeader.biPlanes = 1;
                    bmi.bmiHeader.biBitCount = 32;
                    bmi.bmiHeader.biCompression = BI_RGB;

                    QImage img(bmp.bmWidth, bmp.bmHeight, QImage::Format_ARGB32_Premultiplied);
                    GetDIBits(hDC, iconInfo.hbmColor, 0, bmp.bmHeight, img.bits(), &bmi, DIB_RGB_COLORS);
                    ReleaseDC(NULL, hDC);

                    if (!img.isNull()) {
                        img.save(iconPath, "PNG");
                    }
                }
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
            }
            DestroyIcon(hIcon);
        }
    }

    if (QFile::exists(iconPath)) {
        return QUrl::fromLocalFile(iconPath).toString();
    }
    return "";
}
#endif

namespace {

#ifdef Q_OS_WIN
constexpr int MIN_CANDIDATES  = 200;
constexpr int CAND_MULTIPLIER = 15;
constexpr DBCOUNTITEM BATCH   = 64;
constexpr size_t PATH_CHARS   = 2048;
constexpr size_t MIME_CHARS   = 256;
constexpr const wchar_t *CONN = L"Provider=Search.CollatorDSO;Extended Properties='Application=Windows'";

struct RowBuf {
    DBSTATUS pathStatus; DBLENGTH pathLen; wchar_t path[PATH_CHARS];
    DBSTATUS mimeStatus; DBLENGTH mimeLen; wchar_t mime[MIME_CHARS];
    DBSTATUS attrStatus; DBLENGTH attrLen; ULONG   attr;
};

DBBINDING makeBinding(DBORDINAL ord, DBTYPE type,
                      size_t obStatus, size_t obLen, size_t obVal, DBLENGTH sz) {
    DBBINDING b{};
    b.iOrdinal = ord; b.obStatus = obStatus; b.obLength = obLen; b.obValue = obVal;
    b.dwPart = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
    b.dwMemOwner = DBMEMOWNER_CLIENTOWNED; b.eParamIO = DBPARAMIO_NOTPARAM;
    b.cbMaxLen = sz; b.wType = type;
    return b;
}

std::wstring escapeLike(const QString &word) {
    std::wstring out;
    out.reserve(word.size());
    for (QChar c : word) {
        switch (c.unicode()) {
        case L'\'': out += L"''";   break;
        case L'%':  out += L"[%]";  break;
        case L'_':  out += L"[_]";  break;
        case L'[':  out += L"[[]";  break;
        default:    out += c.unicode();
        }
    }
    return out;
}

std::wstring buildSql(const QString &query, int limit) {
    std::wstring pred;
    for (const auto &word : query.split(' ', Qt::SkipEmptyParts)) {
        if (!pred.empty()) pred += L" AND ";
        pred += L"System.FileName LIKE '%" + escapeLike(word) + L"%'";
    }
    if (pred.empty()) return {};
    return L"SELECT TOP " + std::to_wstring(limit) +
           L" System.ItemPathDisplay, System.MIMEType, System.FileAttributes"
           L" FROM SystemIndex WHERE SCOPE='file:' AND " + pred;
}

struct Cand { std::wstring path; std::string mime; bool isDir = false; };

std::vector<Cand> fetchRows(const std::wstring &sql, int limit) {
    std::vector<Cand> out;
    ComPtr<IDataInitialize> di;
    if (FAILED(CoCreateInstance(CLSID_MSDAINITIALIZE, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&di)))) return out;
    ComPtr<IDBInitialize> dbi;
    if (FAILED(di->GetDataSource(nullptr, CLSCTX_INPROC_SERVER, CONN,
                                 IID_IDBInitialize,
                                 reinterpret_cast<IUnknown **>(dbi.GetAddressOf()))) ||
        FAILED(dbi->Initialize())) return out;

    ComPtr<IDBCreateSession> cs; ComPtr<IDBCreateCommand> cc;
    ComPtr<ICommandText>     ct; ComPtr<IRowset>          rs;

    if (FAILED(dbi.As(&cs)) ||
        FAILED(cs->CreateSession(nullptr, IID_IDBCreateCommand,
                                 reinterpret_cast<IUnknown **>(cc.GetAddressOf()))) ||
        FAILED(cc->CreateCommand(nullptr, IID_ICommandText,
                                 reinterpret_cast<IUnknown **>(ct.GetAddressOf()))) ||
        FAILED(ct->SetCommandText(DBGUID_DEFAULT, sql.c_str())) ||
        FAILED(ct->Execute(nullptr, IID_IRowset, nullptr, nullptr,
                           reinterpret_cast<IUnknown **>(rs.GetAddressOf())))) return out;

    ComPtr<IAccessor> acc;
    if (FAILED(rs.As(&acc))) return out;

    DBBINDING bindings[] = {
        makeBinding(1, DBTYPE_WSTR, offsetof(RowBuf,pathStatus), offsetof(RowBuf,pathLen), offsetof(RowBuf,path), sizeof(RowBuf::path)),
        makeBinding(2, DBTYPE_WSTR, offsetof(RowBuf,mimeStatus), offsetof(RowBuf,mimeLen), offsetof(RowBuf,mime), sizeof(RowBuf::mime)),
        makeBinding(3, DBTYPE_UI4,  offsetof(RowBuf,attrStatus), offsetof(RowBuf,attrLen), offsetof(RowBuf,attr), sizeof(RowBuf::attr)),
    };
    HACCESSOR ha = DB_NULL_HACCESSOR;
    if (FAILED(acc->CreateAccessor(DBACCESSOR_ROWDATA, 3, bindings, 0, &ha, nullptr))) return out;

    out.reserve(static_cast<size_t>(limit));
    while (out.size() < static_cast<size_t>(limit)) {
        HROW handles[BATCH]; HROW *rows = handles; DBCOUNTITEM got = 0;
        if (FAILED(rs->GetNextRows(DB_NULL_HCHAPTER, 0, BATCH, &got, &rows)) || got == 0) break;
        for (DBCOUNTITEM i = 0; i < got; ++i) {
            RowBuf row{};
            if (FAILED(rs->GetData(handles[i], ha, &row))) continue;
            if (row.pathStatus != DBSTATUS_S_OK) continue;
            Cand c{.path = row.path};
            if (row.mimeStatus == DBSTATUS_S_OK && row.mime[0])
                c.mime = QString::fromWCharArray(row.mime).toStdString();
            if (row.attrStatus == DBSTATUS_S_OK)
                c.isDir = (row.attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
            out.emplace_back(std::move(c));
        }
        rs->ReleaseRows(got, handles, nullptr, nullptr, nullptr);
    }
    acc->ReleaseAccessor(ha, nullptr);
    return out;
}
#endif

} // namespace

WinFileSearchService::WinFileSearchService(QObject *parent)
    : QObject(parent) {}

bool WinFileSearchService::isAvailable() const {
    return winSearchAvailable();
}

bool WinFileSearchService::winSearchAvailable() {
#ifdef Q_OS_WIN
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    bool running = false;
    if (SC_HANDLE svc = OpenServiceW(scm, L"WSearch", SERVICE_QUERY_STATUS)) {
        SERVICE_STATUS st{};
        if (QueryServiceStatus(svc, &st)) running = st.dwCurrentState == SERVICE_RUNNING;
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(scm);
    return running;
#else
    return false;
#endif
}

// static
QVariantList WinFileSearchService::runQuery(const QString &query, int limit) {
    QVariantList results;

#ifdef Q_OS_WIN
    const int candidateLimit = (std::max)(200, limit * 15);
    const std::wstring sql = buildSql(query, candidateLimit);
    if (sql.empty()) return results;

    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    struct Scored { std::wstring path; std::string mime; bool isDir; int score; };
    std::vector<Scored> scored;

    for (auto &c : fetchRows(sql, candidateLimit)) {
        const QString pathQ = QString::fromStdWString(c.path);
        const QString name  = QFileInfo(pathQ).fileName();
        // Simple prefix/contains scoring
        int s = 0;
        const QString nl = name.toLower();
        const QString ql = query.toLower();
        if (nl.startsWith(ql))    s = 1000;
        else if (nl.contains(ql)) s = 500;
        else {
            int qi = 0;
            for (int ni = 0; ni < nl.length() && qi < ql.length(); ++ni)
                if (nl[ni] == ql[qi]) ++qi;
            if (qi == ql.length()) s = 100;
        }
        if (s > 0)
            scored.push_back({std::move(c.path), std::move(c.mime), c.isDir, s});
    }

    std::stable_sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        return a.score != b.score ? a.score > b.score : a.path < b.path;
    });

    const int end = (std::min)(static_cast<int>(scored.size()), limit);
    results.reserve(end);

    for (int i = 0; i < end; ++i) {
        const QString pathQ = QString::fromStdWString(scored[i].path);
        const QFileInfo fi(pathQ);
        const QString iconUrl = extractAndCacheShellFileIconWin(pathQ, scored[i].isDir);
        results.append(QVariantMap{
            {"title",    fi.fileName()},
            {"subtitle", pathQ},
            {"path",     pathQ},
            {"icon",     iconUrl},
            {"type",     scored[i].isDir ? "Folder" : "File"},
            {"section",  "Files"},
        });
    }

    if (hrCo == S_OK || hrCo == S_FALSE) CoUninitialize();
#else
    Q_UNUSED(query); Q_UNUSED(limit);
#endif

    return results;
}

QFuture<QVariantList> WinFileSearchService::searchAsync(const QString &query, int limit) const {
    return QtConcurrent::run([q = query, limit]() { return runQuery(q, limit); });
}
