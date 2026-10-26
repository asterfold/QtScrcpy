#include "../util/wificonnection.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto initial = wifiServices("adb-PHONE-random (2)\t_adb-tls-connect._tcp.\t192.168.1.2:40001\n"
                                      "adb-OTHER-random\t_adb-tls-connect._tcp\t192.168.1.3:40002\n"
                                      "adb-PHONE-random\t_adb-tls-pairing._tcp.\t192.168.1.2:39999");
    if (initial.size() != 2 || wifiAddress(initial, "PHONE", {}) != "192.168.1.2:40001"
        || !wifiAddress(initial, {}, {}).isEmpty()) { return 10; }
    const auto changed = wifiServices("adb-PHONE-random\t_adb-tls-connect._tcp\t192.168.1.2:45000");
    if (wifiAddress(changed, "PHONE", {}) != "192.168.1.2:45000") { return 11; }
    auto *connection = new WifiConnection(&app);
    connection->finished = [&app](const QString &address, const QString &error) {
        const QString expected = QString::fromLocal8Bit(qgetenv("WIFI_EXPECT"));
        if (expected == "error") { app.exit(address.isEmpty() && !error.isEmpty() ? 0 : 12); }
        else { app.exit(address == expected && error.isEmpty() ? 0 : 13); }
    };
    QTimer::singleShot(12000, &app, [&app]() { app.exit(14); });
    connection->start(QString::fromLocal8Bit(qgetenv("WIFI_PREFERRED")));
    return app.exec();
}
