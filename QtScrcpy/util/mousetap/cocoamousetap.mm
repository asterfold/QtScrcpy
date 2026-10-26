#import <Cocoa/Cocoa.h>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include "cocoamousetap.h"
#include "../../QtScrcpyCore/src/common/mousediagnostics.h"

// App-local relative capture works without a system-wide CGEventTap. The
// hardware cursor stays centered, so movement cannot reach Dock/Hot Corners.
struct MouseEventTapData {
    QPointer<QWidget> owner;
    std::function<void(QPointF)> handler;
    QRect bounds;
    bool requested = false;
    bool captured = false;
    quint64 moves = 0;
    id monitor = nil;
    id becameKey = nil;
    id resignedKey = nil;
    NSWindow *window = nil;
};

static void releaseCapture(MouseEventTapData *data)
{
    if (data->captured) {
        CGAssociateMouseAndMouseCursorPosition(true);
        data->captured = false;
    }
}

static void beginCapture(MouseEventTapData *data)
{
    if (!data->requested || !data->owner || !data->handler || data->bounds.isEmpty()
        || ![NSApp isActive] || [NSApp keyWindow] != data->window) { return; }
    const QPoint center = data->bounds.center();
    CGWarpMouseCursorPosition(CGPointMake(center.x(), center.y()));
    data->captured = CGAssociateMouseAndMouseCursorPosition(false) == kCGErrorSuccess;
}

CocoaMouseTap::CocoaMouseTap(QObject *parent) : QThread(parent), m_tapData(new MouseEventTapData)
{
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        mouseDiagnostic(QString("native relative_moves=%1 requested=%2 captured=%3 owner=%4")
            .arg(m_tapData->moves).arg(m_tapData->requested).arg(m_tapData->captured).arg(bool(m_tapData->owner)));
        m_tapData->moves = 0;
    });
    timer->start(2000);
}

CocoaMouseTap::~CocoaMouseTap()
{
    quitMouseEventTap();
    delete m_tapData;
}

void CocoaMouseTap::initMouseEventTap()
{
    if (m_tapData->monitor) { return; }
    MouseEventTapData *data = m_tapData;
    const NSEventMask mask = NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged
                          | NSEventMaskRightMouseDragged | NSEventMaskOtherMouseDragged;
    data->monitor = [NSEvent addLocalMonitorForEventsMatchingMask:mask handler:^NSEvent *(NSEvent *event) {
        if (!data->owner || !data->requested || ![NSApp isActive] || [NSApp keyWindow] != data->window) {
            releaseCapture(data);
            return event;
        }
        if (!data->captured) { beginCapture(data); }
        if (!data->captured) { return event; }
        const QPointF delta([event deltaX], [event deltaY]);
        if (!delta.isNull() && data->handler) {
            ++data->moves;
            data->handler(delta);
        }
        // No duplicate absolute Qt movement or synthetic edge-warp feedback.
        return nil;
    }];
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    data->becameKey = [center addObserverForName:NSWindowDidBecomeKeyNotification object:nil queue:nil
                                     usingBlock:^(NSNotification *note) {
        if ([note object] == data->window) { beginCapture(data); }
    }];
    data->resignedKey = [center addObserverForName:NSWindowDidResignKeyNotification object:nil queue:nil
                                      usingBlock:^(NSNotification *note) {
        if ([note object] == data->window) { releaseCapture(data); }
    }];
}

void CocoaMouseTap::quitMouseEventTap()
{
    if (!m_tapData) { return; }
    releaseCapture(m_tapData);
    m_tapData->requested = false;
    if (m_tapData->monitor) { [NSEvent removeMonitor:m_tapData->monitor]; m_tapData->monitor = nil; }
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    if (m_tapData->becameKey) { [center removeObserver:m_tapData->becameKey]; m_tapData->becameKey = nil; }
    if (m_tapData->resignedKey) { [center removeObserver:m_tapData->resignedKey]; m_tapData->resignedKey = nil; }
    m_tapData->handler = nullptr;
    m_tapData->owner.clear();
    m_tapData->window = nil;
}

void CocoaMouseTap::setRelativeMotionHandler(QWidget *owner, std::function<void(QPointF)> handler)
{
    if (m_tapData->owner != owner) { releaseCapture(m_tapData); }
    m_tapData->owner = owner;
    m_tapData->handler = std::move(handler);
    m_tapData->window = [(NSView *)owner->winId() window];
}

void CocoaMouseTap::enableMouseEventTap(QRect bounds, bool enabled)
{
    m_tapData->requested = enabled;
    m_tapData->bounds = bounds;
    if (enabled) { beginCapture(m_tapData); }
    else { releaseCapture(m_tapData); }
}

bool CocoaMouseTap::hasRelativeCapture() const { return m_tapData->captured; }

void CocoaMouseTap::releaseRelativeMotionHandler(QWidget *owner)
{
    if (m_tapData->owner != owner) { return; }
    releaseCapture(m_tapData);
    m_tapData->requested = false;
    m_tapData->owner.clear();
    m_tapData->handler = nullptr;
    m_tapData->window = nil;
}

// Retained for the existing MouseTap/QThread type; no worker or global tap is needed.
void CocoaMouseTap::run() {}
