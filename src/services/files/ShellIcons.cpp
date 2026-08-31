#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "ShellIcons.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <string>
#endif

namespace ShellIcons {

#ifdef Q_OS_WIN
// SHGetFileInfoW with SHGFI_USEFILEATTRIBUTES for native Windows Shell file/folder icons
QString iconUrlFor(const QString &filePath, bool isDir) {
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
#else
QString iconUrlFor(const QString &, bool) { return {}; }
#endif

} // namespace ShellIcons
