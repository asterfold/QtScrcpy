#pragma once

#include <QRegularExpression>
#include <QTimer>
#include <functional>
#include "adbprocess.h"

struct WifiService {
    QString name;
    QString address;
};

inline QList<WifiService> wifiServices(const QString &output)
{
    QList<WifiService> result;
    // Bonjour may append " (2)" to a duplicate service name, and either ADB
    // backend may omit the trailing dot in the service type.
    const QRegularExpression pattern("^(.+?)\\s+_adb-tls-connect\\._tcp\\.?\\s+(\\S+)\\s*$");
    for (const QString &line : output.split('\n')) {
        const auto match = pattern.match(line.trimmed());
        if (match.hasMatch()) {
            result.append({match.captured(1), match.captured(2)});
        }
    }
    return result;
}

inline QString wifiAddress(const QList<WifiService> &services, const QString &serial, const QString &ip)
{
    QStringList matches;
    for (const auto &service : services) {
        const bool sameSerial = !serial.isEmpty()
            && (service.name.startsWith("adb-" + serial + "-") || service.name == "adb-" + serial);
        const bool sameIp = !ip.isEmpty() && service.address.startsWith(ip + ":");
        if ((sameSerial || sameIp || (serial.isEmpty() && ip.isEmpty())) && !matches.contains(service.address)) {
            matches.append(service.address);
        }
    }
    return matches.size() == 1 ? matches.first() : QString();
}

// Each command completes before the next starts. No nested event loops or
// assumed USB/Bonjour timing; only the selected phone can become the target.
class WifiConnection : public QObject
{
public:
    std::function<void(const QString &)> log;
    std::function<void(const QString &, const QString &)> finished;

    explicit WifiConnection(QObject *parent = nullptr) : QObject(parent)
    {
        m_timeout.setSingleShot(true);
        connect(&m_timeout, &QTimer::timeout, this, [this]() {
            m_callback = nullptr;
            m_adb.kill();
            finish({}, QStringLiteral("无线连接超时。请检查手机无线调试、同一 Wi-Fi 和 VPN 的局域网访问设置。"));
        });
        connect(&m_adb, &qsc::AdbProcess::adbProcessResult, this, [this](qsc::AdbProcess::ADB_EXEC_RESULT result) {
            if (result == qsc::AdbProcess::AER_SUCCESS_START || !m_callback) { return; }
            m_timeout.stop();
            auto callback = std::move(m_callback);
            m_callback = nullptr;
            callback(result == qsc::AdbProcess::AER_SUCCESS_EXEC, m_adb.getStdOut() + "\n" + m_adb.getErrorOut());
        });
    }

    void start(const QString &preferred)
    {
        m_preferred = preferred;
        command({}, {"devices", "-l"}, [this](bool ok, const QString &output) {
            if (!ok) { finish({}, output); return; }
            QStringList usb;
            const QRegularExpression device("^(\\S+)\\s+device(?:\\s|$)");
            for (const auto &line : output.split('\n')) {
                const auto match = device.match(line);
                if (match.hasMatch() && !match.captured(1).contains(':')
                    && !match.captured(1).contains("._tcp")) { usb.append(match.captured(1)); }
            }
            if (usb.contains(m_preferred)) { m_serial = m_preferred; }
            else if (usb.size() == 1) { m_serial = usb.first(); }
            else if (usb.size() > 1) {
                finish({}, QStringLiteral("检测到多台 USB 手机，请先选中要连接的手机。")); return;
            }
            m_usb = !m_serial.isEmpty();
            if (m_serial.isEmpty()) {
                // Without USB, use the selected phone's IP, never a random
                // connected device or the first discovered phone.
                m_ip = m_preferred.section(':', 0, 0);
                if (!m_preferred.contains(':')) { m_ip.clear(); }
                if (!m_preferred.isEmpty() && !m_preferred.contains(':') && !m_preferred.contains("._tcp")) {
                    m_serial = m_preferred;
                }
                discover();
                return;
            }
            command(m_serial, {"shell", "ip", "-o", "-4", "addr", "show", "wlan0"},
                    [this](bool ipOk, const QString &ipOutput) {
                const auto match = QRegularExpression("\\binet\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)/").match(ipOutput);
                if (ipOk && match.hasMatch()) { m_ip = match.captured(1); }
                discover();
            });
        });
    }

private:
    using Callback = std::function<void(bool, const QString &)>;
    void command(const QString &serial, const QStringList &args, Callback callback)
    {
        m_callback = std::move(callback);
        m_timeout.start(15000);
        m_adb.execute(serial, args);
    }

    void discover()
    {
        command({}, {"mdns", "services"}, [this](bool, const QString &output) {
            const auto services = wifiServices(output);
            m_address = wifiAddress(services, m_serial, m_ip);
            if (!m_address.isEmpty()) { connectAddress(); return; }
            if (m_round++ < 3) {
                QTimer::singleShot(700, this, [this]() { discover(); }); return;
            }
            if (!m_usb) {
                finish({}, QStringLiteral("未发现选中手机的无线调试服务。请打开无线调试并保持同一 Wi-Fi；首次使用需先配对，或插入 USB 后重试。")); return;
            }
            for (const auto &service : services) {
                if (service.name.startsWith("adb-" + m_serial + "-") || (!m_ip.isEmpty() && service.address.startsWith(m_ip + ":"))) {
                    finish({}, QStringLiteral("发现多个无线端口，暂时无法确定当前端口。请关闭再开启手机无线调试后重试。")); return;
                }
            }
            if (m_ip.isEmpty()) {
                finish({}, QStringLiteral("无法读取手机 Wi-Fi 地址，请确认手机已连接 Wi-Fi。")); return;
            }
            // Legacy USB provisioning explicitly enables 5555. Always use
            // that exact port, not whatever was saved in the UI last time.
            command(m_serial, {"tcpip", "5555"}, [this](bool ok, const QString &tcpOutput) {
                if (!ok || !tcpOutput.contains("restarting in TCP mode")) { finish({}, tcpOutput); return; }
                m_address = m_ip + ":5555";
                m_legacy = true;
                QTimer::singleShot(1000, this, [this]() { connectAddress(); });
            });
        });
    }

    void connectAddress()
    {
        if (log) { log(QStringLiteral("正在连接当前无线地址：") + m_address); }
        command({}, {"connect", m_address}, [this](bool ok, const QString &output) {
            if (!ok || (!output.contains("connected to") && !output.contains("already connected"))) {
                if (output.contains("No route to host") && !m_restarted) {
                    // The shared daemon may still belong to an older ADB
                    // with different macOS network permissions. Restart once
                    // with the app's configured ADB, then retry the endpoint.
                    m_restarted = true;
                    if (log) { log(QStringLiteral("重启共用 ADB 后台后重试（当前投屏会短暂断开）。")); }
                    command({}, {"kill-server"}, [this](bool, const QString &) {
                        command({}, {"start-server"}, [this](bool started, const QString &startOutput) {
                            if (!started) { finish({}, startOutput); return; }
                            connectAddress();
                        });
                    });
                    return;
                }
                if (!m_legacy && !output.contains("authenticate") && m_retry++ < 2) {
                    QTimer::singleShot(700, this, [this]() { discover(); }); return;
                }
                if (m_legacy && m_retry++ < 3) {
                    QTimer::singleShot(700, this, [this]() { connectAddress(); }); return;
                }
                finish({}, QStringLiteral("无线连接失败：") + output); return;
            }
            command(m_address, {"get-state"}, [this](bool stateOk, const QString &state) {
                if (!stateOk || state.trimmed() != "device") { finish({}, QStringLiteral("无线设备尚未就绪：") + state); return; }
                command(m_address, {"shell", "getprop", "ro.serialno"}, [this](bool serialOk, const QString &serial) {
                    if (!serialOk || serial.trimmed().isEmpty() || (!m_serial.isEmpty() && serial.trimmed() != m_serial)) {
                        finish({}, QStringLiteral("无线手机身份验证失败，请检查配对和目标设备。")); return;
                    }
                    finish(m_address, {});
                });
            });
        });
    }

    void finish(const QString &address, const QString &error)
    {
        m_timeout.stop();
        if (finished) { finished(address, error); }
        deleteLater();
    }

    qsc::AdbProcess m_adb;
    QTimer m_timeout;
    Callback m_callback;
    QString m_preferred, m_serial, m_ip, m_address;
    int m_round = 0, m_retry = 0;
    bool m_legacy = false, m_restarted = false, m_usb = false;
};
