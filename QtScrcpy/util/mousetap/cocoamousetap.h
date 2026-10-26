#ifndef COCOAMOUSETAP_H
#define COCOAMOUSETAP_H
#include <QSemaphore>
#include <QThread>

#include "mousetap.h"

struct MouseEventTapData;
class QWidget;
class CocoaMouseTap
    : public MouseTap
    , public QThread
{
public:
    CocoaMouseTap(QObject *parent = Q_NULLPTR);
    virtual ~CocoaMouseTap();

    void initMouseEventTap() override;
    void quitMouseEventTap() override;
    void enableMouseEventTap(QRect rc, bool enabled) override;
    void setRelativeMotionHandler(QWidget *owner, std::function<void(QPointF)> handler) override;
    bool hasRelativeCapture() const override;
    void releaseRelativeMotionHandler(QWidget *owner) override;

protected:
    void run() override;

private:
    MouseEventTapData *m_tapData = Q_NULLPTR;
    QSemaphore m_runloopStartedSemaphore;
};

#endif // COCOAMOUSETAP_H
