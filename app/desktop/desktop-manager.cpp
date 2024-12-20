//
// Created by dingjing on 23-6-12.
//

#include "desktop-manager.h"

#include <qdatetime.h>

#include <QTimer>
#include <QHostInfo>
#include <QApplication>
#include <QNetworkInterface>

#include "screen.h"


DesktopManager* DesktopManager::gInstance = nullptr;

DesktopManager *DesktopManager::getInstance()
{
    static QMutex mMutex;
    QMutexLocker locker(&mMutex);

    if (!gInstance) {
        gInstance = new DesktopManager;
    }

    return gInstance;
}

DesktopManager::DesktopManager(QObject *parent)
    : QObject (parent), mTimer(new QTimer)
{
    auto screens = qApp->screens();

    auto font = qApp->font();
    // font.setFamily("宋体");
    font.setBold (true);
    font.setPointSize (18);
    font.setStyleStrategy (QFont::ForceOutline);
    qApp->setFont (font);

    for (auto& s : screens) {
        mScreens.append (new Screen(mFinName, s));
    }

    mTimer->setInterval(1000 * 60 * 60);
    connect(mTimer, &QTimer::timeout, [this]() ->void {
        updateOtherInfos();
    });
    mTimer->start();
}

void DesktopManager::setString(const QString &str, bool otherInfo)
{
    if (str.isNull() || str.isEmpty()) {
        return;
    }

    mString = str;
    mOtherInfo = otherInfo;

    if (mOtherInfo) {
        updateOtherInfos();
    }

    for (auto s : mScreens) {
        s->setString (mFinName);
    }
}

void DesktopManager::run()
{
    for (auto s : mScreens) {
        s->showWindow();
    }
}

void DesktopManager::updateOtherInfos()
{
    if (mOtherInfo) {

        // hostname
        mHostName = QHostInfo::localHostName();

        // mac
        for (QNetworkInterface netInterface : QNetworkInterface::allInterfaces()) {
            if (netInterface.flags().testFlag(QNetworkInterface::IsUp) && netInterface.flags().testFlag(QNetworkInterface::IsRunning) && !netInterface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
                mMac= netInterface.hardwareAddress();
                break;
            }
        }

        // date time
        mDateTimeStr = QDateTime::currentDateTime().toLocalTime().toString("yyyy-mm-dd");
        mFinName = QString("%1\n%2  %3\n%4").arg(mString, mMac, mHostName, mDateTimeStr);
    }
    else {
        mFinName = mString;
    }
}
