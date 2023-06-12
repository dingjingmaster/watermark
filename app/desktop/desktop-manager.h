//
// Created by dingjing on 23-6-12.
//

#ifndef WATERMARK_DESKTOP_MANAGER_H
#define WATERMARK_DESKTOP_MANAGER_H
#include <QMap>
#include <QObject>

class Screen;
class DesktopManager : public QObject
{
    Q_OBJECT
public:
    static DesktopManager* getInstance();

    void setString (const QString& str);

    void run ();


private:
    explicit DesktopManager(QObject* parent = nullptr);

private:
    static DesktopManager*              gInstance;

    QString                             mString;

    QList<Screen*>                      mScreens;
};


#endif //WATERMARK_DESKTOP_MANAGER_H
