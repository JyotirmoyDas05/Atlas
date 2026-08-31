#include "AppSearchService.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFileIconProvider>
#include <QCryptographicHash>
#include <QUrl>
#include <QPixmap>
#include <QIcon>
#include <QImage>
#include <QDebug>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <thumbcache.h>
#include <shellapi.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// Recommended Windows Shell IShellItemImageFactory pipeline (Raycast style)
static HBITMAP extractIShellItemBitmapWin(const std::wstring &path, int size = 48) {
    ComPtr<IShellItem> shellItem;
    HRESULT hr = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
    if (FAILED(hr) || !shellItem) return nullptr;

    ComPtr<IShellItemImageFactory> imageFactory;
    hr = shellItem.As(&imageFactory);
    if (FAILED(hr) || !imageFactory) return nullptr;

    SIZE sz = { size, size };
    HBITMAP hBmp = nullptr;
    // SIIGBF_RESIZETOFIT | SIIGBF_ICONONLY extracts pure high-res icon without shortcut overlay
    hr = imageFactory->GetImage(sz, SIIGBF_RESIZETOFIT | SIIGBF_ICONONLY, &hBmp);
    if (SUCCEEDED(hr) && hBmp) {
        return hBmp;
    }
    return nullptr;
}

// Fallback HICON extraction via System Image List
static HICON extractCleanAppIconWin(const std::wstring &path) {
    SHFILEINFOW sfi = {};
    DWORD_PTR res = SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);
    if (!res) {
        res = SHGetFileInfoW(path.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
    }
    if (!res) return nullptr;

    IImageList *pImageList = nullptr;
    HRESULT hr = SHGetImageList(SHIL_EXTRALARGE, IID_IImageList, reinterpret_cast<void **>(&pImageList));
    if (FAILED(hr) || !pImageList) {
        hr = SHGetImageList(SHIL_LARGE, IID_IImageList, reinterpret_cast<void **>(&pImageList));
    }

    HICON hIcon = nullptr;
    if (SUCCEEDED(hr) && pImageList) {
        pImageList->GetIcon(sfi.iIcon, ILD_NORMAL, &hIcon);
        pImageList->Release();
    }
    return hIcon;
}
#endif

static QString categorizeApp(const QString &appName, const QString &exePath, const QString &lnkPath) {
    const QString nameLower = appName.toLower();
    const QString pathLower = (exePath.isEmpty() ? lnkPath : exePath).toLower();

    // 1. Game Client & Title Parsing (Steam, Epic Games, GOG, Xbox Game Pass)
    if (pathLower.contains("steam://") || pathLower.contains("com.epicgames.launcher://") ||
        pathLower.contains("\\steam\\steamapps\\common\\") || pathLower.contains("\\gog.com\\") ||
        pathLower.contains("\\epic games\\") || pathLower.contains("\\saved games\\") ||
        nameLower.contains("cyberpunk") || nameLower.contains("witcher") || nameLower.contains("gta") ||
        nameLower.contains("counter-strike") || nameLower.contains("dota") || nameLower.contains("valorant") ||
        nameLower.contains("minecraft") || nameLower.contains("league of legends") || nameLower.contains("fortnite") ||
        nameLower.contains("overwatch") || nameLower.contains("apex legends") || nameLower.contains("call of duty") ||
        nameLower.contains("elden ring") || nameLower.contains("baldurs gate") || nameLower.contains("roblox") ||
        nameLower.contains("game")) {
        if (!nameLower.contains("steam") && !nameLower.contains("epic games launcher")) {
            return "Game";
        }
    }

    // 2. Developer Tools
    if (pathLower.endsWith("code.exe") || pathLower.endsWith("wt.exe") || pathLower.endsWith("devenv.exe") ||
        pathLower.endsWith("idea64.exe") || pathLower.endsWith("clion64.exe") || pathLower.endsWith("pycharm64.exe") ||
        pathLower.endsWith("webstorm64.exe") || pathLower.endsWith("datagrip64.exe") || pathLower.endsWith("rider64.exe") ||
        pathLower.endsWith("git-bash.exe") || pathLower.endsWith("githubdesktop.exe") || pathLower.endsWith("docker desktop.exe") ||
        pathLower.endsWith("postman.exe") || pathLower.endsWith("sublime_text.exe") || pathLower.endsWith("androidstudio.exe") ||
        nameLower.contains("visual studio") || nameLower.contains("vscode") || nameLower.contains("terminal") ||
        nameLower.contains("jetbrains") || nameLower.contains("sublime text") || nameLower.contains("antigravity")) {
        return "Developer";
    }

    // 3. Browsers / Internet
    if (pathLower.endsWith("chrome.exe") || pathLower.endsWith("msedge.exe") || pathLower.endsWith("firefox.exe") ||
        pathLower.endsWith("brave.exe") || pathLower.endsWith("opera.exe") || pathLower.endsWith("vivaldi.exe") ||
        nameLower.contains("chrome") || nameLower.contains("edge") || nameLower.contains("firefox") || nameLower.contains("browser")) {
        return "Browser";
    }

    // 4. Design / Graphics
    if (pathLower.endsWith("photoshop.exe") || pathLower.endsWith("illustrator.exe") || pathLower.endsWith("figma.exe") ||
        pathLower.endsWith("blender.exe") || pathLower.endsWith("gimp.exe") || pathLower.endsWith("canva.exe") ||
        nameLower.contains("photoshop") || nameLower.contains("figma") || nameLower.contains("blender") || nameLower.contains("gimp")) {
        return "Design";
    }

    // 5. Productivity / Office & Communication
    if (pathLower.endsWith("excel.exe") || pathLower.endsWith("winword.exe") || pathLower.endsWith("powerpnt.exe") ||
        pathLower.endsWith("slack.exe") || pathLower.endsWith("notion.exe") || pathLower.endsWith("obsidian.exe") ||
        pathLower.endsWith("teams.exe") || pathLower.endsWith("discord.exe") || pathLower.endsWith("zoom.exe") ||
        nameLower.contains("excel") || nameLower.contains("word") || nameLower.contains("powerpoint") ||
        nameLower.contains("slack") || nameLower.contains("notion") || nameLower.contains("obsidian") ||
        nameLower.contains("discord") || nameLower.contains("teams")) {
        return "Productivity";
    }

    // 6. System Apps
    if (pathLower.contains("\\system32\\") || pathLower.endsWith("cmd.exe") || pathLower.endsWith("powershell.exe") ||
        pathLower.endsWith("control.exe") || pathLower.endsWith("taskmgr.exe") || pathLower.endsWith("regedit.exe") ||
        nameLower.contains("settings") || nameLower.contains("control panel") || nameLower.contains("task manager")) {
        return "System";
    }

    return "Application";
}

static AppEntry resolveShortcutWin(const QString &lnkPath) {
    AppEntry entry;
    entry.lnkPath = lnkPath;
    entry.name = QFileInfo(lnkPath).completeBaseName();

#ifdef Q_OS_WIN
    ComPtr<IShellLinkW> link;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&link))))
        return entry;

    ComPtr<IPersistFile> file;
    if (FAILED(link.As(&file)))
        return entry;

    if (FAILED(file->Load(lnkPath.toStdWString().c_str(), STGM_READ)))
        return entry;

    wchar_t buf[MAX_PATH] = {};
    WIN32_FIND_DATAW fd{};
    link->GetPath(buf, MAX_PATH, &fd, SLGP_RAWPATH);
    entry.path = QString::fromWCharArray(buf);

    wchar_t descBuf[1024] = {};
    link->GetDescription(descBuf, 1024);
    entry.description = QString::fromWCharArray(descBuf);

    entry.isDirectory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif

    return entry;
}

int AppSearchService::fuzzyScore(const QString &name, const QString &query) {
    const QString nameLower = name.toLower();
    const QString qLower = query.toLower();

    if (nameLower.startsWith(qLower)) return 1000 + (100 - name.length());
    if (nameLower.contains(qLower))   return 500  + (100 - name.length());

    int qi = 0;
    for (int ni = 0; ni < nameLower.length() && qi < qLower.length(); ++ni) {
        if (nameLower[ni] == qLower[qi]) ++qi;
    }
    if (qi == qLower.length()) return 100 + (100 - name.length());

    return 0;
}

AppSearchService::AppSearchService(QObject *parent)
    : QObject(parent) {
    // Icon cache persists across launches: wiping it here cost ~4s of
    // synchronous re-extraction on every start (and clobbered the shell icon
    // cache shared with file search). Extraction skips files that exist.
    m_iconCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/atlas/cache/icons";
    QDir().mkpath(m_iconCacheDir);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(300);
    connect(&m_debounce, &QTimer::timeout, this, &AppSearchService::refresh);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, &m_debounce,
            qOverload<>(&QTimer::start));
    refresh();
}

void AppSearchService::extractAndCacheIcon(AppEntry &entry) {
    QString targetPath = entry.path;
    if (targetPath.isEmpty() || !QFile::exists(targetPath)) {
        targetPath = entry.lnkPath;
    }
    if (targetPath.isEmpty()) return;

    const QByteArray hash = QCryptographicHash::hash((entry.name + targetPath).toUtf8(), QCryptographicHash::Md5).toHex();
    const QString iconPath = m_iconCacheDir + "/" + QString::fromLatin1(hash) + ".png";

    if (!QFile::exists(iconPath)) {
        bool saved = false;

#ifdef Q_OS_WIN
        // Step 1: Query IShellItemImageFactory for clean high-res vector/PNG icon without shortcut overlay
        HBITMAP hBmp = nullptr;
        if (!entry.path.isEmpty() && QFile::exists(entry.path)) {
            hBmp = extractIShellItemBitmapWin(entry.path.toStdWString(), 48);
        }
        if (!hBmp && !entry.lnkPath.isEmpty()) {
            hBmp = extractIShellItemBitmapWin(entry.lnkPath.toStdWString(), 48);
        }

        if (hBmp) {
            BITMAP bmp = {};
            if (GetObject(hBmp, sizeof(bmp), &bmp) && bmp.bmWidth > 0 && bmp.bmHeight > 0) {
                HDC hDC = GetDC(NULL);
                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = bmp.bmWidth;
                bmi.bmiHeader.biHeight = -bmp.bmHeight;
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                QImage img(bmp.bmWidth, bmp.bmHeight, QImage::Format_ARGB32_Premultiplied);
                GetDIBits(hDC, hBmp, 0, bmp.bmHeight, img.bits(), &bmi, DIB_RGB_COLORS);
                ReleaseDC(NULL, hDC);

                if (!img.isNull()) {
                    saved = img.save(iconPath, "PNG");
                }
            }
            DeleteObject(hBmp);
        }

        // Step 2: System Image List fallback
        if (!saved) {
            HICON hIcon = nullptr;
            if (!entry.path.isEmpty() && QFile::exists(entry.path)) {
                hIcon = extractCleanAppIconWin(entry.path.toStdWString());
            }
            if (!hIcon && !entry.lnkPath.isEmpty()) {
                hIcon = extractCleanAppIconWin(entry.lnkPath.toStdWString());
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
                            saved = img.save(iconPath, "PNG");
                        }
                    }
                    DeleteObject(iconInfo.hbmColor);
                    DeleteObject(iconInfo.hbmMask);
                }
                DestroyIcon(hIcon);
            }
        }
#endif

        if (!saved) {
            QFileIconProvider provider;
            QIcon icon = provider.icon(QFileInfo(targetPath));
            if (!icon.isNull()) {
                QPixmap pixmap = icon.pixmap(48, 48);
                if (!pixmap.isNull()) {
                    pixmap.save(iconPath, "PNG");
                }
            }
        }
    }

    if (QFile::exists(iconPath)) {
        entry.iconUrl = QUrl::fromLocalFile(iconPath).toString();
    }
}

void AppSearchService::refresh() {
    m_apps.clear();
    m_apps.reserve(256);

    const QStringList dirs = {
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
        QDir::homePath() + "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs",
        "C:/ProgramData/Microsoft/Windows/Start Menu/Programs",
        QDir::homePath() + "/Desktop",
        "C:/Users/Public/Desktop",
        "C:/ProgramData/Microsoft/Windows/Start Menu/Programs/Games",
        "C:/ProgramData/Microsoft/Windows/Start Menu/Programs/GOG.com",
        "C:/ProgramData/Microsoft/Windows/Start Menu/Programs/Steam",
    };

    for (const auto &d : dirs) {
        if (!QDir(d).exists()) continue;
        m_watcher.addPath(d);
        enumerateDirectory(d);
    }

    // Deduplicate by name (keep first occurrence)
    std::vector<QString> seen;
    seen.reserve(m_apps.size());
    m_apps.erase(std::remove_if(m_apps.begin(), m_apps.end(), [&](const AppEntry &e) {
        auto it = std::find(seen.begin(), seen.end(), e.name.toLower());
        if (it != seen.end()) return true;
        seen.push_back(e.name.toLower());
        return false;
    }), m_apps.end());

    std::stable_sort(m_apps.begin(), m_apps.end(), [](const AppEntry &a, const AppEntry &b) {
        return a.name.toLower() < b.name.toLower();
    });

    emit appsChanged();
    qDebug() << "[Atlas] AppSearch: indexed" << m_apps.size() << "apps and games.";
}

void AppSearchService::enumerateDirectory(const QString &dir) {
    QDirIterator it(dir, {"*.lnk", "*.exe", "*.url"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        AppEntry entry;

        if (path.endsWith(".lnk", Qt::CaseInsensitive)) {
            entry = resolveShortcutWin(path);
        } else if (path.endsWith(".url", Qt::CaseInsensitive)) {
            entry.path = path;
            entry.lnkPath = path;
            entry.name = QFileInfo(path).completeBaseName();
        } else {
            entry.path = path;
            entry.name = QFileInfo(path).completeBaseName();
        }

        if (entry.name.isEmpty()) continue;
        if (entry.path.isEmpty() && entry.lnkPath.isEmpty()) continue;

        entry.category = categorizeApp(entry.name, entry.path, entry.lnkPath);
        extractAndCacheIcon(entry);
        m_apps.emplace_back(std::move(entry));
    }
}

QVariantList AppSearchService::all() const {
    QVariantList out;
    out.reserve(static_cast<int>(m_apps.size()));
    for (const auto &a : m_apps) {
        out.append(QVariantMap{
            {"title",    a.name},
            {"subtitle", a.description.isEmpty() ? a.path : a.description},
            {"path",     a.path.isEmpty() ? a.lnkPath : a.path},
            {"icon",     a.iconUrl},
            {"type",     a.category},
            {"section",  "Applications"},
        });
    }
    return out;
}

QVariantList AppSearchService::search(const QString &query) const {
    if (query.isEmpty()) return all();

    struct Scored { const AppEntry *e; int score; };
    std::vector<Scored> scored;
    scored.reserve(m_apps.size());

    for (const auto &a : m_apps) {
        const int s = fuzzyScore(a.name, query);
        if (s > 0) scored.push_back({&a, s});
    }

    std::stable_sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        return a.score > b.score;
    });

    // Loose fuzzy matches on 1-2 char queries hit ~100 of 138 apps; building
    // QVariantMaps for all of them dominated keystroke latency. Top 15 is more
    // than the visible list anyway.
    constexpr size_t MAX_APP_RESULTS = 15;
    if (scored.size() > MAX_APP_RESULTS) scored.resize(MAX_APP_RESULTS);

    QVariantList out;
    out.reserve(static_cast<int>(scored.size()));
    for (const auto &[e, _] : scored) {
        out.append(QVariantMap{
            {"title",    e->name},
            {"subtitle", e->description.isEmpty() ? e->path : e->description},
            {"path",     e->path.isEmpty() ? e->lnkPath : e->path},
            {"icon",     e->iconUrl},
            {"type",     e->category},
            {"section",  "Applications"},
        });
    }
    return out;
}
