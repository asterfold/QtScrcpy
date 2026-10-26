#include "../util/displaylist.h"
#include <QCoreApplication>
#include <QFile>
#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString monitor = QStringLiteral("Display id %1: DisplayInfo{\"Monitor\", displayId %1, FLAG_PRESENTATION, real 1920 x 1200, rotation 0, type EXTERNAL, uniqueId \"local:4632668068469674517\", layerStack %1}\n");
    const QString phone = QStringLiteral("Display id 0: DisplayInfo{\"内置屏幕\", real 1080 x 2340, type INTERNAL, uniqueId \"local:phone\"}\n");
    auto initial = parseDisplayTargets(phone + monitor.arg(67));
    auto reconnected = parseDisplayTargets(phone + monitor.arg(71));
    if (initial.size() != 2 || reconnected.size() != 2 || initial[1].id != 67 || reconnected[1].id != 71
            || initial[1].uniqueId != reconnected[1].uniqueId || initial[1].type != "EXTERNAL"
            || initial[1].size != QSize(1920, 1200)) { return 1; }
    auto disconnected = parseDisplayTargets(phone);
    for (const auto &display : disconnected) { if (display.uniqueId == initial[1].uniqueId) { return 2; } }
    if (!parseDisplayTargets("Error: device offline").isEmpty()) { return 3; }
    if (argc > 1) {
        QFile live(QString::fromLocal8Bit(argv[1]));
        if (!live.open(QIODevice::ReadOnly)) { return 4; }
        auto displays = parseDisplayTargets(QString::fromUtf8(live.readAll()));
        bool external = false;
        for (const auto &display : displays) { external |= display.type == "EXTERNAL" && display.name == "Monitor"; }
        if (!external) { return 5; }
    }
    std::cout << "PASS: display enumeration, reconnect identity, missing target, error output, live phone metadata\n";
}
