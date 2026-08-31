#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QMutex>
#include <QPainter>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextStream>
#include "app/AtlasNativeHost.hpp"
#include "app/RootSearchModel.hpp"
#include "app/ThemeBridge.hpp"
#include "app/ConfigBridge.hpp"

static const QString kInstanceName = QStringLiteral("atlas-launcher-instance");

// Simple generated glyph icon so the tray works without shipping an asset yet.
static QIcon makeAppIcon() {
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(30, 30, 40));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(pm.rect(), 14, 14);
    QFont font(QStringLiteral("Segoe UI"), 34, QFont::Bold);
    p.setFont(font);
    p.setPen(QColor(240, 240, 245));
    p.drawText(pm.rect(), Qt::AlignCenter, QStringLiteral("A"));
    p.end();
    return QIcon(pm);
}

// Keep the log file open for the process lifetime: reopening per line costs a
// synchronous file open on hot paths (every qDebug during search/toggle).
void fileLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Q_UNUSED(type);
    Q_UNUSED(context);
    static QMutex mutex;
    static QFile file("atlas_debug.log");
    static QTextStream stream = [] {
        file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        return QTextStream(&file);
    }();

    QMutexLocker lock(&mutex);
    if (file.isOpen()) {
        stream << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << ' ' << msg << '\n';
        stream.flush();
    }
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(fileLogHandler);
    qDebug() << "[Atlas] main() started.";

    QApplication app(argc, argv);

    // CRITICAL: Prevent application from quitting when launcher window is hidden
    app.setQuitOnLastWindowClosed(false);

    app.setOrganizationName("Atlas");
    app.setApplicationName("Atlas Launcher");
    app.setWindowIcon(makeAppIcon());

    // Single instance: if another Atlas owns the socket, ask it to show itself
    // and exit. Two instances would fight over the keyboard hook.
    {
        QLocalSocket probe;
        probe.connectToServer(kInstanceName);
        if (probe.waitForConnected(150)) {
            probe.write("show");
            probe.flush();
            probe.waitForBytesWritten(150);
            qDebug() << "[Atlas] Another instance is already running; told it to show. Exiting.";
            return 0;
        }
    }
    QLocalServer::removeServer(kInstanceName); // clean up stale socket after a crash
    QLocalServer instanceServer;
    if (!instanceServer.listen(kInstanceName)) {
        qWarning() << "[Atlas] Could not claim single-instance socket:" << instanceServer.errorString();
    }

    QQmlApplicationEngine engine;

    // Expose Theme and Config bridges to QML
    ThemeBridge themeBridge;
    ConfigBridge configBridge;
    engine.rootContext()->setContextProperty("Theme", &themeBridge);
    engine.rootContext()->setContextProperty("Config", &configBridge);

    RootSearchModel searchModel;
    engine.rootContext()->setContextProperty("searchModel", &searchModel);

    const QUrl url(QStringLiteral("qrc:/Atlas/src/ui/MainPalette.qml"));
    qDebug() << "[Atlas] Loading QML from:" << url;

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        const QUrl fallbackUrl(QStringLiteral("qrc:/qt/qml/Atlas/src/ui/MainPalette.qml"));
        qWarning() << "[Atlas] Trying fallback QML URL:" << fallbackUrl;
        engine.load(fallbackUrl);
    }

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "[Atlas] Failed to load QML UI root object!";
        return -1;
    }

    QQuickWindow *rootWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!rootWindow) {
        qCritical() << "[Atlas] QML Root object is not a QQuickWindow!";
        return -1;
    }

    // Initialize native event host, DWM composition, and global hotkeys
    auto nativeHost = new AtlasNativeHost(rootWindow, &app);
    app.installNativeEventFilter(nativeHost);

    // A second launch pings the socket: show the window.
    QObject::connect(&instanceServer, &QLocalServer::newConnection, nativeHost, [&]() {
        while (QLocalSocket *conn = instanceServer.nextPendingConnection()) {
            conn->deleteLater();
        }
        qDebug() << "[Atlas] Second-instance ping received: showing window.";
        nativeHost->showWindow();
    });

    // System tray: show/hide, autostart toggle, quit.
    QSystemTrayIcon tray(makeAppIcon());
    tray.setToolTip(QStringLiteral("Atlas — Alt+Space"));
    QMenu trayMenu;
    QObject::connect(trayMenu.addAction(QStringLiteral("Show Atlas")), &QAction::triggered,
                     nativeHost, &AtlasNativeHost::showWindow);

    QAction *autoStart = trayMenu.addAction(QStringLiteral("Start with Windows"));
    autoStart->setCheckable(true);
    {
        QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::NativeFormat);
        autoStart->setChecked(runKey.contains(QStringLiteral("Atlas")));
    }
    QObject::connect(autoStart, &QAction::toggled, [](bool on) {
        QSettings runKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::NativeFormat);
        if (on) {
            runKey.setValue(QStringLiteral("Atlas"),
                            QLatin1Char('"') +
                                QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) +
                                QLatin1Char('"'));
        } else {
            runKey.remove(QStringLiteral("Atlas"));
        }
    });

    trayMenu.addSeparator();
    QObject::connect(trayMenu.addAction(QStringLiteral("Quit Atlas")), &QAction::triggered,
                     &app, &QCoreApplication::quit);
    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, nativeHost,
                     [nativeHost](QSystemTrayIcon::ActivationReason reason) {
                         if (reason == QSystemTrayIcon::Trigger ||
                             reason == QSystemTrayIcon::DoubleClick) {
                             nativeHost->toggleVisibility();
                         }
                     });
    tray.show();

    qDebug() << "[Atlas] Launcher engine initialized & running in background.";
    return app.exec();
}
