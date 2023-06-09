/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Oliver Grossmann.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include "dxc_options.h"
#include "ui_dxc_options.h"
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include "dxc_spots.h"

DXCOptions::DXCOptions(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DXCOptions)
{
    ui->setupUi(this);

    /* select font for text viewer */
    auto fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixedFont.setPointSize(10);
    ui->plainTextEdit_DXCMonitor->setFont(fixedFont);

    m_socket = new QTcpSocket(this);
    connect(m_socket, SIGNAL(connected()),this, SLOT(connected()));
    connect(m_socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(m_socket, SIGNAL(readyRead()),this, SLOT(readyToRead()));
}

DXCOptions::~DXCOptions()
{
    delete ui;
}

/*! \brief Catch window close events.
 *
 * This method is called when the user closes the audio options dialog
 * window using the window close icon. We catch the event and hide the
 * dialog but keep it around for later use.
 */
void DXCOptions::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

/*! \brief Catch window show events. */
void DXCOptions::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
}

void DXCOptions::on_pushButton_DXCConnect_clicked()
{
    DXCSpots::Get().setSpotTimeout(ui->lineEdit_DXCSpottimeout->text().toInt());
    m_socket->connectToHost(ui->lineEdit_DXCAddress->text(),ui->lineEdit_DXCPort->text().toInt());
    if(!m_socket->waitForConnected(5000))
    {
        ui->plainTextEdit_DXCMonitor->appendPlainText(m_socket->errorString());
    }
}

void DXCOptions::on_pushButton_DXCDisconnect_clicked()
{
    m_socket->close();
}

void DXCOptions::connected()
{
    ui->plainTextEdit_DXCMonitor->appendPlainText("Connected");
    ui->pushButton_DXCConnect->setDisabled(true);
    ui->pushButton_DXCDisconnect->setEnabled(true);
}

void DXCOptions::disconnected()
{
    ui->plainTextEdit_DXCMonitor->appendPlainText("Disconnected");
    ui->pushButton_DXCDisconnect->setDisabled(true);
    ui->pushButton_DXCConnect->setEnabled(true);
}

void DXCOptions::readyToRead()
{
    DXCSpotInfo info;
    QStringList spot;
    QString incomingMessage;

    incomingMessage = m_socket->readLine();
    while (incomingMessage.length() > 0)
    {
        ui->plainTextEdit_DXCMonitor->appendPlainText(incomingMessage.remove('\a').trimmed());
        if(incomingMessage.contains("enter your call", Qt::CaseInsensitive)
                || incomingMessage.contains("login:", Qt::CaseInsensitive))
        {
            m_socket->write(ui->lineEdit_DXCUSername->text().append("\n").toUtf8());
            ui->plainTextEdit_DXCMonitor->appendPlainText(ui->lineEdit_DXCUSername->text());
        }
        else if(incomingMessage.contains("DX de", Qt::CaseInsensitive) &&
                incomingMessage.contains(ui->lineEdit_DXCFilter->text()))
        {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            spot = incomingMessage.split(" ", QString::SkipEmptyParts);
#else
            spot = incomingMessage.split(" ", Qt::SkipEmptyParts);
#endif
            if (spot.length() >= 5)
            {
                info.name = spot[4].trimmed();
                info.frequency = spot[3].toDouble() * 1000;
                DXCSpots::Get().add(info);
            }
        }

        incomingMessage = m_socket->readLine();
    }
}

void DXCOptions::saveSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("dxcluster");

    settings->setValue("DXCAddress", ui->lineEdit_DXCAddress->text());
    settings->setValue("DXCPort", ui->lineEdit_DXCPort->text());
    settings->setValue("DXCUsername", ui->lineEdit_DXCUSername->text());
    settings->setValue("DXCSpotTimeout", ui->lineEdit_DXCSpottimeout->text());
    settings->setValue("DXCFilter", ui->lineEdit_DXCFilter->text());

    settings->endGroup();
}

void DXCOptions::readSettings(QSettings *settings)
{
    if (!settings)
        return;

    settings->beginGroup("dxcluster");
    ui->lineEdit_DXCAddress->setText(settings->value("DXCAddress", "localhost").toString());
    ui->lineEdit_DXCPort->setText(settings->value("DXCPort", "7300").toString());
    ui->lineEdit_DXCUSername->setText(settings->value("DXCUsername", "nocall").toString());
    ui->lineEdit_DXCSpottimeout->setText(settings->value("DXCSpotTimeout", "10").toString());
    ui->lineEdit_DXCFilter->setText(settings->value("DXCFilter", "").toString());

    settings->endGroup();
}
