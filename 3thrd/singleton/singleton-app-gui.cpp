#include "singleton-app-gui.h"

#include <pwd.h>
#include <glib.h>
#include <unistd.h>

#include <QTime>
#include <QThread>
#include <QDataStream>
#include <QCryptographicHash>

#define SHARED_MEMORY_SIZE 4096

SingletonApp::SingletonApp(int &argc, char *argv[], const char *appName, bool allowSecondary, Options options, int timeout)
        : QApplication(argc, argv)
{
    bool            ret = false;
    unsigned long   blockSize = 0;

    mServer = nullptr;
    mSocket = nullptr;
    mMemory = nullptr;
    mInstanceNumber = -1;

    mOptions = options;

    parseCommandLine();
    genBlockServerName(appName);

#ifdef Q_OS_UNIX
    // 防止异常退出后, 没有销毁共享内存
    mMemory = new QSharedMemory(mBlockServerName);
    mMemory->attach();
    delete mMemory;
#endif

    // 初始化共享内存
    mMemory = new QSharedMemory(mBlockServerName);
    while (++blockSize) {
        if (blockSize * SHARED_MEMORY_SIZE >= sizeof (InstancesInfo)) {
            ret = mMemory->create(sizeof (InstancesInfo));
            break;
        }
    }

    if(ret) {
        mMemory->lock();
        initializeMemoryBlock();
        mMemory->unlock();
    } else {
        if (!mMemory->attach()) {
            qWarning() << "SingletonApplication: Unable to attach to shared memory block. error:" << mMemory->errorString();
            ::exit( EXIT_FAILURE );
        }
    }

    mMemory->lock();
    auto* inst = static_cast<InstancesInfo*>(mMemory->data());
    QTime time;
    time.start();

    while(true) {
        if(blockChecksum() == inst->checksum) break;
        if(time.elapsed() > 5000) {
            qWarning() << "SingletonApplication: Shared memory block has been in an inconsistent state from more than 5s. Assuming primary instance failure.";
            initializeMemoryBlock();
        }

        qsrand(QDateTime::currentMSecsSinceEpoch() % std::numeric_limits<uint>::max());
        QThread::sleep(8 + static_cast<unsigned long>(static_cast<float>(qrand()) / RAND_MAX * 10));
    }

    if (!inst->primary) {
        startPrimary();
        mMemory->unlock();
        return;
    }

    if(allowSecondary) {
        inst->secondary += 1;
        inst->checksum = blockChecksum();
        mInstanceNumber = inst->secondary;
        startSecondary();
        if (mOptions & Mode::SecondaryNotification) {
            connectToPrimary (timeout, SecondaryInstance);
        }
        mMemory->unlock();
        return;
    }

    mMemory->unlock();

    connectToPrimary (timeout, NewInstance);

    qDebug() << appName << " already running...";
    //::exit(EXIT_FAILURE);
}

SingletonApp::~SingletonApp()
{
    if (nullptr != mSocket) {
        mSocket->close();
        delete mSocket;
    }

    mMemory->lock();
    auto inst = static_cast<InstancesInfo*>(mMemory->data());
    if(mServer != nullptr) {
        mServer->close();
        delete mServer;
        inst->primary = false;
        inst->primaryPid = -1;
        inst->checksum = blockChecksum();
    }
    mMemory->unlock();

    delete mMemory;
}

bool SingletonApp::isPrimary()
{
    return nullptr != mServer;
}

bool SingletonApp::isSecondary()
{
    return mServer == nullptr;
}

qint64 SingletonApp::primaryPid()
{
    qint64 pid;

    mMemory->lock();
    auto* inst = static_cast<InstancesInfo*>(mMemory->data());
    pid = inst->primaryPid;
    mMemory->unlock();

    return pid;
}

quint32 SingletonApp::instanceId() const
{
    return mInstanceNumber;
}

bool SingletonApp::sendMessage(const QByteArray& message, int timeout)
{
    if(isPrimary()) {
        qDebug() << "is primary, cannot send message!";
        return false;
    }

    connectToPrimary(timeout,  Reconnect);

    mSocket->write(message);
    bool dataWritten = mSocket->waitForBytesWritten(timeout);
    mSocket->flush();
    return dataWritten;
}

void SingletonApp::genBlockServerName(const char *appName)
{
    QCryptographicHash appData(QCryptographicHash::Sha256);
    appData.addData (appName, 18);
    appData.addData (SingletonApp::QCoreApplication::applicationName().toUtf8());
    appData.addData (SingletonApp::QCoreApplication::organizationName().toUtf8());
    appData.addData (SingletonApp::QCoreApplication::organizationDomain().toUtf8());

    if(!(mOptions & SingletonApp::Mode::ExcludeAppVersion)) {
        appData.addData( SingletonApp::QCoreApplication::applicationVersion().toUtf8());
    }

    if(!(mOptions & SingletonApp::Mode::ExcludeAppPath)) {
        appData.addData(SingletonApp::QCoreApplication::applicationFilePath().toUtf8());
    }

    if(mOptions & SingletonApp::Mode::User) {
        QByteArray username;
        uid_t uid = geteuid();
        struct passwd *pw = getpwuid(uid);
        if(pw) {
            username = pw->pw_name;
        }
        if(username.isEmpty()) {
            username = qgetenv("USER");
        }
        appData.addData(username);
    }

    mBlockServerName = appData.result().toBase64().replace("/", "_");
}

void SingletonApp::initializeMemoryBlock()
{
    auto* inst = static_cast<InstancesInfo*>(mMemory->data());
    inst->primary = false;
    inst->secondary = 0;
    inst->primaryPid = -1;
    inst->checksum = blockChecksum();
}

void SingletonApp::startPrimary()
{
    QLocalServer::removeServer(mBlockServerName);
    mServer = new QLocalServer();

    if(mOptions & SingletonApp::Mode::User) {
        mServer->setSocketOptions(QLocalServer::UserAccessOption);
    } else {
        mServer->setSocketOptions(QLocalServer::WorldAccessOption);
    }

    mServer->listen(mBlockServerName);
    QObject::connect(mServer, &QLocalServer::newConnection, this, &SingletonApp::slotConnectionEstablished);

    auto* inst = static_cast <InstancesInfo*>(mMemory->data());

    inst->primary = true;
    inst->primaryPid = applicationPid();
    inst->checksum = blockChecksum();

    mInstanceNumber = 0;
}

void SingletonApp::startSecondary()
{

}

void SingletonApp::connectToPrimary(int msecs, SingletonApp::ConnectionType connectionType)
{
    if(mSocket == nullptr) {
        mSocket = new QLocalSocket();
    }

    if(mSocket->state() == QLocalSocket::ConnectedState) {
        return;
    }

    if(mSocket->state() == QLocalSocket::UnconnectedState || mSocket->state() == QLocalSocket::ClosingState) {
        mSocket->connectToServer(mBlockServerName);
    }

    if(mSocket->state() == QLocalSocket::ConnectingState) {
        mSocket->waitForConnected(msecs);
    }

    if(mSocket->state() == QLocalSocket::ConnectedState) {
        QByteArray initMsg;
        QDataStream writeStream(&initMsg, QIODevice::WriteOnly);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        writeStream.setVersion(QDataStream::Qt_5_6);
#endif

        writeStream << mBlockServerName.toLatin1();
        writeStream << static_cast<quint8>(connectionType);
        writeStream << mInstanceNumber;
        quint16 checksum = qChecksum(initMsg.constData(), static_cast<quint32>(initMsg.length()));
        writeStream << checksum;

        QByteArray header;
        QDataStream headerStream(&header, QIODevice::WriteOnly);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
        headerStream.setVersion(QDataStream::Qt_5_6);
#endif
        headerStream << static_cast <quint64>(initMsg.length());

        mSocket->write(header);
        mSocket->write(initMsg);
        mSocket->flush();
        mSocket->waitForBytesWritten(msecs);
    }
}

quint16 SingletonApp::blockChecksum()
{
    return qChecksum(static_cast <const char *>(mMemory->data()), offsetof(InstancesInfo, checksum));
}

void SingletonApp::readInitMessageHeader(QLocalSocket *sock)
{
    if (!mConnectionMap.contains(sock)) {
        return;
    }

    if(sock->bytesAvailable() < (qint64)sizeof(quint64)) {
        return;
    }

    QDataStream headerStream(sock);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    headerStream.setVersion(QDataStream::Qt_5_6);
#endif

    quint64 msgLen = 0;
    headerStream >> msgLen;
    ConnectionInfo &info = mConnectionMap[sock];
    info.stage = StageBody;
    info.msgLen = msgLen;

    if (sock->bytesAvailable() >= (qint64) msgLen) {
        readInitMessageBody(sock);
    }
}

void SingletonApp::readInitMessageBody(QLocalSocket *sock)
{
    if (!mConnectionMap.contains(sock)) {
        return;
    }

    ConnectionInfo &info = mConnectionMap[sock];
    if(sock->bytesAvailable() < (qint64)info.msgLen) {
        return;
    }

    // Read the message body
    QByteArray msgBytes = sock->read(info.msgLen);
    QDataStream readStream(msgBytes);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    readStream.setVersion(QDataStream::Qt_5_6);
#endif

    QByteArray latin1Name;
    readStream >> latin1Name;

    ConnectionType connectionType = InvalidConnection;
    quint8 connTypeVal = InvalidConnection;
    readStream >> connTypeVal;
    connectionType = static_cast <ConnectionType>(connTypeVal);

    quint32 instanceId = 0;
    readStream >> instanceId;

    quint16 msgChecksum = 0;
    readStream >> msgChecksum;

    const quint16 actualChecksum = qChecksum(msgBytes.constData(), static_cast<quint32>(msgBytes.length() - sizeof(quint16)));

    bool isValid = readStream.status() == QDataStream::Ok && QLatin1String(latin1Name) == mBlockServerName && msgChecksum == actualChecksum;
    if(!isValid) {
        sock->close();
        return;
    }

    info.instanceId = instanceId;
    info.stage = StageConnected;

    if(connectionType == NewInstance || ( connectionType == SecondaryInstance && mOptions & SingletonApp::Mode::SecondaryNotification)) {
        Q_EMIT instanceStarted();
    }

    if (sock->bytesAvailable() > 0) {
        Q_EMIT this->slotDataAvailable(sock, instanceId);
    }
}

void SingletonApp::slotConnectionEstablished()
{
    QLocalSocket *nextConnSocket = mServer->nextPendingConnection();
    mConnectionMap.insert(nextConnSocket, ConnectionInfo());

    QObject::connect(nextConnSocket, &QLocalSocket::aboutToClose,
        [nextConnSocket, this]() {
            auto &info = mConnectionMap[nextConnSocket];
            Q_EMIT this->slotClientConnectionClosed(nextConnSocket, info.instanceId);
        }
    );

    QObject::connect(nextConnSocket, &QLocalSocket::disconnected,
        [nextConnSocket, this]() {
            mConnectionMap.remove(nextConnSocket);
            nextConnSocket->deleteLater();
        }
    );

    QObject::connect(nextConnSocket, &QLocalSocket::readyRead,
        [nextConnSocket, this]() {
            auto &info = mConnectionMap[nextConnSocket];
            switch(info.stage) {
            case StageHeader:
                readInitMessageHeader(nextConnSocket);
                break;
            case StageBody:
                readInitMessageBody(nextConnSocket);
                break;
            case StageConnected:
                Q_EMIT this->slotDataAvailable(nextConnSocket, info.instanceId);
                break;
            default:
                break;
            };
        }
    );
}

void SingletonApp::slotDataAvailable(QLocalSocket *dataSocket, quint32 instanceId)
{
    Q_EMIT receivedMessage(instanceId, dataSocket->readAll());
}

void SingletonApp::slotClientConnectionClosed(QLocalSocket *closedSocket, quint32 instanceId)
{
    if (closedSocket->bytesAvailable() > 0) {
        Q_EMIT slotDataAvailable(closedSocket, instanceId);
    }
}

void SingletonApp::parseCommandLine()
{
    mParser.setApplicationDescription (tr("Watermark 水印功能"));
    mParser.addHelpOption();
    mParser.addVersionOption();

    QCommandLineOption screenOpt(QStringList()     << "s" << "screen", QCoreApplication::translate ("main-app-c", "添加水印"));

    mParser.addOption (screenOpt);

    mParser.addPositionalArgument ("String", QCoreApplication::translate ("main-app-c", "要显示的字符串"));

    mParser.process (*this);

    if (mParser.isSet (screenOpt)) {
        mString = mParser.positionalArguments().at (0);
        mString = mString.replace ("\\n", "\n");
    }
}

void SingletonApp::showHelp()
{
    mParser.showHelp (0);
}

QString SingletonApp::getString()
{
    return mString;
}
