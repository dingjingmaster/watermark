//
// Created by dingjing on 23-6-12.
//

#ifndef WATERMARK_SCREEN_H
#define WATERMARK_SCREEN_H
#include <QWidget>
#include <QScreen>

class Screen : public QWidget
{
    Q_OBJECT
public:
    explicit Screen (const QString& str, QScreen* screen, QWidget* parent = nullptr);

    void showWindow();
    void setString(const QString& str);

protected:
    void paintEvent (QPaintEvent* event) override;

private:

private:
    const int                   mMargin = 3;
    const int                   mJGH = 18;
    const int                   mJGW = 18;

    QSize                       mSize;
    QSize                       mWidgetSize;

    QString                     mString;
    QScreen*                    mScreen = nullptr;
};


#endif //WATERMARK_SCREEN_H
