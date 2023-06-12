//
// Created by dingjing on 23-6-12.
//

#include "screen.h"

#include <glib.h>

#include <QDebug>
#include <QPainter>
#include <QGuiApplication>


Screen::Screen(const QString& str, QScreen* screen, QWidget *parent)
    : QWidget (parent), mScreen(screen)
{
    setWindowFlags (Qt::Dialog
        | Qt::WindowStaysOnTopHint
        | Qt::WindowTransparentForInput
        | Qt::FramelessWindowHint
        | Qt::WindowDoesNotAcceptFocus);
    setAttribute (Qt::WA_TranslucentBackground);

    mSize = mScreen->size();
    mSize.setWidth (mSize.width() * 2);
    mSize.setHeight (mSize.height() * 2);
    setString (str);
}

void Screen::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    p.save();
    p.setPen (Qt::black);
    p.setViewTransformEnabled (false);
    p.setRenderHint (QPainter::Antialiasing, true);

    p.rotate (-45);
    p.translate (-mSize.height(), 0);

    QFontMetrics fm(qApp->font());

    for (auto x = mMargin; (x + mJGW + mWidgetSize.width()) < mSize.width(); x += (mJGW + mWidgetSize.width())) {
        for (auto y = mMargin; (y + mJGH + mWidgetSize.height()) < mSize.height(); y += (mJGH + mWidgetSize.height())) {
            p.save();
            QStringList sl = mString.split ("\n");
            auto c = sl.count();
            for (auto l = 0; l < c; ++l) {
                auto ww = fm.horizontalAdvance (sl.at (l));
                auto xS = x + (mWidgetSize.width() - ww) / 2;
                auto yS = y + l * fm.height();
                QRectF rec(xS, yS, ww, fm.height());
                p.drawText (rec, Qt::AlignCenter, sl.at (l));
            }
            p.restore();
        }
    }

    p.restore();
}

void Screen::showWindow()
{
    g_return_if_fail(mScreen);

    setGeometry (mScreen->geometry());
    resize (mScreen->size());

    show();
}

void Screen::setString(const QString &str)
{
    mString = str;

    QFontMetrics fm(qApp->font());

    int w = 0;
    int h = 0;
    auto sl = mString.split ("\n");
    for (const auto& s : sl) {
        auto ww = fm.horizontalAdvance (s);
        w = (w < ww) ? ww : w;
        h += fm.height();
    }

    mWidgetSize.setWidth (w);
    mWidgetSize.setHeight (h);
}
