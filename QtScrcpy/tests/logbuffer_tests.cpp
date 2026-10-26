#include <QApplication>
#include <QTextDocument>
#include <QTextCursor>
#include <thread>
#include <iostream>
#include "../util/logbuffer.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    LogBuffer buffer;
    std::thread producer([&] {
        for (int i = 0; i < 100000; ++i) { buffer.append(QString::number(i), false); }
    });
    producer.join();
    const QString first = buffer.takeBatch();
    if (first.split('\n').size() != 32 || !first.startsWith("99744\n")) { return 1; }
    int retained = 32;
    QString last;
    for (;;) {
        const QString batch = buffer.takeBatch();
        if (batch.isEmpty()) { break; }
        retained += batch.split('\n').size();
        last = batch;
    }
    if (retained != 256 || !last.endsWith("99999")) { return 2; }
    buffer.append(QString(1000000, 'x'), false);
    if (buffer.takeBatch().size() != 2048) { return 3; }
    QTextDocument document;
    document.setMaximumBlockCount(1000);
    QTextCursor cursor(&document);
    for (int i = 0; i < 10000; ++i) { cursor.insertText(QString::number(i) + '\n'); }
    if (document.blockCount() != 1000 || !document.toPlainText().contains("9999\n")) { return 4; }
    std::cout << "PASS: 100000 worker logs bounded to latest 256; batches of 32; oversized messages capped; document bounded to 1000 lines\n";
}
