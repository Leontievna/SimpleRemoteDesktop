#ifndef REMOTEDESKTOPUNITING_H
#define REMOTEDESKTOPUNITING_H

#include <QObject>
#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

#include "serverhttp.h"
#include "websockettransfer.h"
#include "graberclass.h"
#include "inputsimulator.h"

class RemoteDesktopUniting : public QObject
{
    Q_OBJECT
public:
    explicit RemoteDesktopUniting(QObject *parent = Q_NULLPTR);

private:

    WebSocketTransfer *m_webSocketTransfer;
    WebSocketHandler *m_webSocketHandler;
    ServerHttp *m_serverHttp;
    GraberClass *m_graberClass;
    InputSimulator *m_inputSimulator;
    QMenu *m_trayMenu;
    QSystemTrayIcon *m_trayIcon;

    QString m_currentIp;
    int m_currentPort;

signals:
    void closeSignal();

public slots:

private slots:
    void actionTriggered(QAction* action);
    void showInfoMessage();
    void updateCurrentIp();
    void loadSettings();
    void startHttpServer(quint16 port, const QString &filesPath);
    void startWebSocketTransfer(quint16 port, const QString &login, const QString &pass);
    void startWebSocketHandler(const QString &host, const QString &name, const QString &login,
                               const QString &pass, const QString &proxyLogin, const QString &proxyPass);
    void createConnectionToHandler(WebSocketHandler *webSocketHandler);
    void finishedWebSockeTransfer();
    void finishedWebSockeHandler();
};

#endif // REMOTEDESKTOPUNITING_H
