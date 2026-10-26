#ifndef DISPLAYLIST_H
#define DISPLAYLIST_H

#include <QList>
#include <QRegularExpression>
#include <QSize>
#include <QString>

struct DisplayTarget {
    int id;
    QString name;
    QString type;
    QString uniqueId;
    QSize size;
};

inline QList<DisplayTarget> parseDisplayTargets(const QString &output)
{
    QList<DisplayTarget> displays;
    const QRegularExpression pattern(
        QStringLiteral("Display id (\\d+): DisplayInfo\\{\"([^\"]+)\"[^\\n]*?real (\\d+) x (\\d+)[^\\n]*?type ([A-Z_]+)[^\\n]*?uniqueId \"([^\"]+)\""));
    auto matches = pattern.globalMatch(output);
    while (matches.hasNext()) {
        const auto match = matches.next();
        displays.append({match.captured(1).toInt(), match.captured(2), match.captured(5), match.captured(6),
                         QSize(match.captured(3).toInt(), match.captured(4).toInt())});
    }
    return displays;
}

#endif
