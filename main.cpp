#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "Qrimageprovider.hpp"
#include "Qrimagedecoder.hpp"
#if defined(FORCE_STYLE)
#include <QQuickStyle>
#endif
int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);
#if defined(FORCE_STYLE)
    QQuickStyle::setStyle(FORCE_STYLE);
#endif

    QQmlApplicationEngine engine;
    engine.addImageProvider(QLatin1String("qrcode"), new QRImageProvider(1));
    engine.addImageProvider(QLatin1String("wasm"), new WasmImageProvider());
    engine.addImportPath("qrc:/esterVtech.com/imports");

    const QUrl url("qrc:/esterVtech.com/imports/Esterv/Iota/NFTMinter/qml/window.qml");
    engine.load(url);

    return app.exec();
}
