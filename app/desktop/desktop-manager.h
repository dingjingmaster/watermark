//
// Created by dingjing on 23-6-12.
//

#ifndef WATERMARK_DESKTOP_MANAGER_H
#define WATERMARK_DESKTOP_MANAGER_H
#include <QMap>
#include <QMutex>
#include <QObject>

class QTimer;
class Screen;
class DesktopManager : public QObject
{
    Q_OBJECT
public:
    static DesktopManager* getInstance();

    void setString (const QString& str, bool otherInfo);

    void run ();


private:
    void updateOtherInfos();
    explicit DesktopManager(QObject* parent = nullptr);

private:
    static DesktopManager*              gInstance;

    bool                                mOtherInfo{};
    QString                             mMac;
    QString                             mString;
    QString                             mHostName;
    QString                             mDateTimeStr;

    QString                             mFinName;

    QTimer*                             mTimer;
    QList<Screen*>                      mScreens;
};


#endif //WATERMARK_DESKTOP_MANAGER_H
