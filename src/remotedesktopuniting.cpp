#include "remotedesktopuniting.h"
#include <QApplication>
#include <QCommonStyle>
#include <QNetworkInterface>
#include <QSettings>
#include <QDebug>
#include <QUuid>
#include <QApplication>
#include <QThread>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RemoteDesktopUniting u;
    return a.exec();
}

RemoteDesktopUniting::RemoteDesktopUniting(QObject *parent) : QObject(parent),
    m_webSocketTransfer(Q_NULLPTR),
    m_serverHttp(new ServerHttp(this)),
    m_graberClass(new GraberClass(this)),
    m_inputSimulator(new InputSimulator(this)),
    m_trayMenu(new QMenu),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_currentIp("127.0.0.1"),
    m_currentPort(8080)
{
    QCommonStyle style;
    m_trayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_MessageBoxInformation)),"Help");
    m_trayMenu->addAction(QIcon(style.standardPixmap(QStyle::SP_DialogCancelButton)),"Exit");

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/res/favicon.ico"));
    m_trayIcon->setToolTip("SimpleRemoteDesktop");
    m_trayIcon->show();

    connect(m_trayMenu,SIGNAL(triggered(QAction*)),this,SLOT(actionTriggered(QAction*)));

    updateCurrentIp();
    loadSettings();

    m_graberClass->start();
}

void RemoteDesktopUniting::actionTriggered(QAction *action)
{
    if(action->text() == "Help")
        showInfoMessage();
    else if(action->text() == "Exit")
        emit closeSignal();
}

void RemoteDesktopUniting::showInfoMessage()
{
    m_trayIcon->showMessage("SimpleRemoteDesktop",
                            "To connect, enter in browser:\n" +
                            m_currentIp + ":" + QString::number(m_currentPort),
                            QSystemTrayIcon::Information);
}

void RemoteDesktopUniting::updateCurrentIp()
{
    QList<QHostAddress> my_addr_list;
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    for(int i=0;i<addresses.count();++i)
    {
        if(addresses.at(i).protocol() == QAbstractSocket::IPv4Protocol &&
                !addresses.at(i).toString().contains("127") &&
                !addresses.at(i).toString().contains("172"))
        {
            my_addr_list.append(addresses.at(i));
        }
    }

    if(my_addr_list.count() > 0)
        m_currentIp = my_addr_list.at(0).toString();
}

void RemoteDesktopUniting::loadSettings()
{
    QSettings settings("config.ini",QSettings::IniFormat);
    settings.beginGroup("REMOTE_DESKTOP");

    int portHttp = settings.value("portHttp",0).toInt();
    if(portHttp == 0)
    {
        portHttp = 8080;
        settings.setValue("portHttp",portHttp);
    }

    QString filesPath = settings.value("filesPath").toString();
    if(filesPath.isEmpty())
    {
        filesPath = ":/res/";
        settings.setValue("filesPath",filesPath);
    }

    int portWeb = settings.value("portWeb",0).toInt();
    if(portWeb == 0)
    {
        portWeb = 8081;
        settings.setValue("portWeb",portWeb);
    }

    QString login = settings.value("login").toString();
    if(login.isEmpty())
    {
        login = "login";
        settings.setValue("login",login);
    }

    QString pass = settings.value("pass").toString();
    if(pass.isEmpty())
    {
        pass = "pass";
        settings.setValue("pass",pass);
    }

    QString name = settings.value("name").toString();
    if(name.isEmpty())
    {
        name = QUuid::createUuid().toString().mid(1,8);
        settings.setValue("name",name);
    }

    QString remoteHost = settings.value("remoteHost").toString();
    if(remoteHost.isEmpty())
    {
        remoteHost = "remoteHost";
        settings.setValue("remoteHost",remoteHost);
    }

    settings.endGroup();
    settings.sync();

    startHttpServer(portHttp,filesPath);
    startWebSocketTransfer(portWeb,login,pass);
}

void RemoteDesktopUniting::startHttpServer(quint16 port, const QString &filesPath)
{
    m_serverHttp->setPort(static_cast<quint16>(port));
    m_serverHttp->setPath(filesPath);
    m_currentPort = port;

    if(m_serverHttp->start())
    {
        showInfoMessage();
    }
    else
    {
        m_trayIcon->showMessage("SimpleRemoteDesktop","Failed to start on port: " +
                                QString::number(port) + "!",QSystemTrayIcon::Critical,5000);
    }
}

void RemoteDesktopUniting::startWebSocketTransfer(quint16 port, const QString &login, const QString &pass)
{
    QThread *thread = new QThread;
    m_webSocketTransfer = new WebSocketTransfer;

    m_webSocketTransfer->setPort(port);
    m_webSocketTransfer->setLoginPass(login, pass);

    connect(thread, &QThread::started, m_webSocketTransfer, &WebSocketTransfer::start);
    connect(this, &RemoteDesktopUniting::closeSignal, m_webSocketTransfer, &WebSocketTransfer::stop);
    connect(m_webSocketTransfer, &WebSocketTransfer::finished, this, &RemoteDesktopUniting::finishedWebSocketransfer);
    connect(m_webSocketTransfer, &WebSocketTransfer::finished, thread, &QThread::quit);
    connect(thread, &QThread::finished, m_webSocketTransfer, &WebSocketTransfer::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(m_webSocketTransfer, &WebSocketTransfer::newSocketConnected, this, &RemoteDesktopUniting::createConnectionToHandler);
    connect(m_webSocketTransfer, &WebSocketTransfer::disconnectedAll, m_graberClass, &GraberClass::stopSending);

    m_webSocketTransfer->moveToThread(thread);
    thread->start();
}

void RemoteDesktopUniting::createConnectionToHandler(WebSocketHandler *webSocketHandler)
{
    connect(m_graberClass, &GraberClass::imageParameters, webSocketHandler, &WebSocketHandler::sendImageParameters);
    connect(m_graberClass, &GraberClass::imageTile, webSocketHandler, &WebSocketHandler::sendImageTile);
    connect(m_graberClass, &GraberClass::screenPositionChanged, m_inputSimulator, &InputSimulator::setScreenPosition);

    connect(webSocketHandler, &WebSocketHandler::getDesktop, m_graberClass, &GraberClass::startSending);
    connect(webSocketHandler, &WebSocketHandler::changeDisplayNum, m_graberClass, &GraberClass::changeScreenNum);
    connect(webSocketHandler, &WebSocketHandler::receivedTileNum, m_graberClass, &GraberClass::setReceivedTileNum);

    connect(webSocketHandler, &WebSocketHandler::setKeyPressed, m_inputSimulator, &InputSimulator::simulateKeyboard);
    connect(webSocketHandler, &WebSocketHandler::setMousePressed, m_inputSimulator, &InputSimulator::simulateMouseKeys);
    connect(webSocketHandler, &WebSocketHandler::setMouseMove, m_inputSimulator, &InputSimulator::simulateMouseMove);
    connect(webSocketHandler, &WebSocketHandler::setWheelChanged, m_inputSimulator, &InputSimulator::simulateWheelEvent);
    connect(webSocketHandler, &WebSocketHandler::setMouseDelta, m_inputSimulator, &InputSimulator::setMouseDelta);
}

void RemoteDesktopUniting::finishedWebSocketransfer()
{
    QApplication::quit();
}