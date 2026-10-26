#include "../util/mousetap/relativecursor.h"
#include <iostream>
#include <stdexcept>
static void require(bool ok) { if (!ok) { throw std::runtime_error("relative cursor recenter failed"); } }
int main()
{
    try {
        const QRect bounds(100, 200, 800, 500);
        const QPointF center(500, 450);
        for (QPointF edge : {QPointF(110, 450), QPointF(890, 450), QPointF(500, 210), QPointF(500, 690), QPointF(1000, 800)}) {
            require(relativeCursorTarget(bounds, edge) == center);
        }
        require(relativeCursorTarget(bounds, center) == center);
        QPointF position = center;
        int recenters = 0;
        for (int i = 0; i < 10000; ++i) {
            position += QPointF(11, 0);
            const auto target = relativeCursorTarget(bounds, position);
            recenters += target != position;
            position = target;
            require(position.x() < 900 && position.x() > 100);
        }
        require(recenters > 100);
        require(relativeCursorTarget(QRect(0, 0, 40, 40), QPointF(1, 1)) == QPointF(20, 20));
        std::cout << "PASS: four edges, overshoot, small window, 10000 continuous steps recenter within bounds\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
