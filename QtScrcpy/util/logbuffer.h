#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QStringList>

// Producers never enqueue one GUI event per message. Keep recent diagnostics
// bounded even when the GUI cannot drain them as fast as they arrive.
class LogBuffer
{
public:
    void append(const QString &message, bool blankLine)
    {
        QMutexLocker lock(&m_mutex);
        if (m_messages.size() == 256) { m_messages.removeFirst(); }
        m_messages.append(message.left(2048) + (blankLine ? "\n" : ""));
    }

    QString takeBatch()
    {
        QMutexLocker lock(&m_mutex);
        QStringList batch;
        for (int i = 0; i < 32 && !m_messages.isEmpty(); ++i) {
            batch.append(m_messages.takeFirst());
        }
        return batch.join('\n');
    }

private:
    QMutex m_mutex;
    QStringList m_messages;
};
