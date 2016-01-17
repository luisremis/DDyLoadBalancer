/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtNetwork>
#include <iostream>
#include "dialog.h"

struct gui_packet
{
    char init;
    unsigned int total;
    unsigned int completed;
    unsigned int throtling;
};

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
{
    //clientProgressBar = new QProgressBar;
    clientStatusLabel = new QLabel(tr("Client ready"));
    serverProgressBar = new QProgressBar;
    serverStatusLabel = new QLabel(tr("Server ready"));

    startButton = new QPushButton(tr("&Start"));
    quitButton = new QPushButton(tr("&Quit"));

    buttonBox = new QDialogButtonBox;
    //buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    //connect(startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(&tcpServer,  SIGNAL(newConnection()),this, SLOT(acceptConnection()));
    connect(&tcpServer2, SIGNAL(newConnection()),this, SLOT(acceptConnection2()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    //mainLayout->addWidget(clientProgressBar);
    //
    mainLayout->addWidget(serverProgressBar);
    mainLayout->addWidget(serverStatusLabel);
    mainLayout->addWidget(clientStatusLabel);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(tr("MP4 GUI"));
    std::cout << tcpServer.serverAddress().toString().toStdString() << std::endl;
    start();
}

void Dialog::start()
{
    startButton->setEnabled(false);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif

   // bytesWritten = 0;
   // bytesReceived = 0;

    while (!tcpServer.isListening() && !tcpServer.listen()) {
        QMessageBox::StandardButton ret = QMessageBox::critical(this,
                                        tr("Loopback"),
                                        tr("Unable to start the test: %1.")
					.arg(tcpServer.errorString()),
                                        QMessageBox::Retry
					| QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel)
            return;
    }
    while (!tcpServer2.isListening() && !tcpServer2.listen()) {
        QMessageBox::StandardButton ret = QMessageBox::critical(this,
                                        tr("Loopback"),
                                        tr("Unable to start the test: %1.")
                    .arg(tcpServer2.errorString()),
                                        QMessageBox::Retry
                    | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel)
            return;
    }

    serverStatusLabel->setText(tr("Listening at %1").arg(tcpServer.serverPort()));
    clientStatusLabel->setText(tr("Listening at %1").arg(tcpServer2.serverPort()));
    //clientStatusLabel->setText(tcpServer.serverAddress().toString());
    //tcpClient.connectToHost(QHostAddress::LocalHost, tcpServer.serverPort());


    //std::cout << "About to open socket port: " <<tcpServer.serverPort() <<  std::endl;
    //std::cout << "About to open socket 2port: " <<tcpServer2.serverPort() <<  std::endl;


    //tcpCloud.connectToHost("172.22.156.33", 50006);
    //tcpMobile.connectToHost("172.22.156.93", 50005);

    //char data = 'm';
    //tcpCloud.bytesAvailable();
    //while ( !tcpCloud.bytesAvailable() );

    //std::cout << tcpCloud.bytesAvailable() << std::endl;


}

void Dialog::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection();
    connect(tcpServerConnection, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress()));
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    serverStatusLabel->setText(tr("Accepted connection"));
    tcpServer.close();
    std::cout << "Received connection" << std::endl;
}

void Dialog::acceptConnection2()
{
    tcpServerConnection2 = tcpServer2.nextPendingConnection();
    connect(tcpServerConnection2, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress2()));
    connect(tcpServerConnection2, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));

    serverStatusLabel->setText(tr("Accepted connection"));
    tcpServer2.close();
    std::cout << "Received connection2" << std::endl;
}

void Dialog::startTransfer()
{
    // called when the TCP client connected to the loopback server
    //bytesToWrite = TotalBytes - (int)tcpClient.write(QByteArray(PayloadSize, '@'));
    //clientStatusLabel->setText(tr("Connected"));
}

void Dialog::updateServerProgress()
{
    int bytesReceived = (int)tcpServerConnection->bytesAvailable();
    //tcpServerConnection->readAll();

    struct gui_packet packet;
    tcpServerConnection->read((char*)&packet, sizeof(struct gui_packet));
    //std::cout << "Recevied: " << packet.total << " : " << packet.completed<< std::endl;
    client_completed = packet.completed;

    serverProgressBar->setMaximum(packet.total);
    serverProgressBar->setValue(client_completed + server_completed);
    clientStatusLabel->setText(tr("Server: %1\%").arg(packet.throtling));

    //std::cout << "\% Completed: " << (client_completed + server_completed) << "/" << packet.total << std::endl;

    if (client_completed + server_completed == packet.total)
    {
        tcpServerConnection->close();
        startButton->setEnabled(true);
#ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
#endif
    }
}

void Dialog::updateServerProgress2()
{
    int bytesReceived = (int)tcpServerConnection2->bytesAvailable();
    //tcpServerConnection->readAll();

    struct gui_packet packet;
    tcpServerConnection2->read((char*)&packet, sizeof(struct gui_packet));
    //std::cout << "Recevied: " << packet.total << " : " << packet.completed<< std::endl;
    server_completed = packet.completed;

    //std::cout << "\% Completed: " << (client_completed + server_completed) << "/" << packet.total << std::endl;

    serverProgressBar->setMaximum(packet.total);
    serverProgressBar->setValue(client_completed + server_completed);
    serverStatusLabel->setText(tr("Mobile: %1\%").arg(packet.throtling));

    if (client_completed + server_completed == packet.total)
    {
        tcpServerConnection2->close();
        startButton->setEnabled(true);
#ifndef QT_NO_CURSOR
        QApplication::restoreOverrideCursor();
#endif
    }
}
/*
void Dialog::updateClientProgress(qint64 numBytes)
{

    // callen when the TCP client has written some bytes
    bytesWritten += (int)numBytes;

    // only write more if not finished and when the Qt write buffer is below a certain size.
    if (bytesToWrite > 0 && tcpClient.bytesToWrite() <= 4*PayloadSize)
        bytesToWrite -= (int)tcpClient.write(QByteArray(qMin(bytesToWrite, PayloadSize), '@'));

    clientProgressBar->setMaximum(TotalBytes);
    clientProgressBar->setValue(bytesWritten);
    clientStatusLabel->setText(tr("Sent %1MB")
                               .arg(bytesWritten / (1024 * 1024)));

}*/

void Dialog::displayError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QTcpSocket::RemoteHostClosedError)
        return;
/*
    QMessageBox::information(this, tr("Network error"),
                             tr("The following error occurred: %1.")
                             .arg(tcpClient.errorString()));
*/
    //tcpClient.close();
    tcpServer.close();
    //clientProgressBar->reset();
    //serverProgressBar->reset();
    clientStatusLabel->setText(tr("Client ready"));
    serverStatusLabel->setText(tr("Server ready"));
    startButton->setEnabled(true);
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
}
