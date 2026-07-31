#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QIcon>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "app/AtlasNativeHost.hpp"
#include "app/SearchViewModel.hpp"
#include "app/ThemeBridge.hpp"
#include "app/ConfigBridge.hpp"

void fileLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QFile file("atlas_debug.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << msg << "\n";
    }
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(fileLogHandler);
    qDebug() << "[Atlas] main() started.";

    QGuiApplication app(argc, argv);

    // CRITICAL: Prevent application from quitting when launcher window is hidden
    app.setQuitOnLastWindowClosed(false);

    app.setOrganizationName("Atlas");
    app.setApplicationName("Atlas Launcher");

    qmlRegisterType<SearchViewModel>("Atlas", 1, 0, "SearchViewModel");

    QQmlApplicationEngine engine;

    // Expose Theme and Config bridges to QML
    ThemeBridge themeBridge;
    ConfigBridge configBridge;
    engine.rootContext()->setContextProperty("Theme", &themeBridge);
    engine.rootContext()->setContextProperty("Config", &configBridge);

    SearchViewModel viewModel;
    engine.rootContext()->setContextProperty("searchModel", &viewModel);

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

    qDebug() << "[Atlas] Launcher engine initialized & running in background.";
    return app.exec();
}
