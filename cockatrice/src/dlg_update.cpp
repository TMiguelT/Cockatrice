#define HUMAN_DOWNLOAD_URL "https://bintray.com/cockatrice/Cockatrice/Cockatrice/_latestVersion"

#include <QtNetwork>
#include <QProgressDialog>
#include <QtGui/qdesktopservices.h>
#include <QtWidgets/qmessagebox.h>

#include "dlg_update.h"

DlgUpdate::DlgUpdate(QWidget *parent, QString downloadUrlStr, int downloadSize)
        : QProgressDialog(parent) {

    QUrl downloadUrl = QUrl(downloadUrlStr);

    //Check for SSL (this probably isn't necessary)
    if (!QSslSocket::supportsSsl()) {
        QMessageBox::critical(
                this,
                tr("Error"),
                tr("Cockatrice was not built with SSL support, so cannot download update! "
                           "Please visit download the new version manually at the following URL: " HUMAN_DOWNLOAD_URL));
    }

    //Update the maximum progress
    setMaximum(downloadSize);

    //Work out the file name of the download
    QString fileName = downloadUrlStr.section('/', -1);

    //Create a temporary directory to save the file in
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(false);

    //Open the file for writing and begin the download
    file = new QFile(tempDir.path() + '/' + fileName);
    if (file->open(QIODevice::WriteOnly))
        beginDownload(downloadUrl);
    else
        QMessageBox::critical(
                this,
                tr("Error"),
                tr("A temporary file could not be opened for downloading. Please update manually from " HUMAN_DOWNLOAD_URL));
}

void DlgUpdate::beginDownload(QUrl downloadUrl) {

    //Request the file and handle success failure, progress etc.
    response = netMan.get(QNetworkRequest(downloadUrl));
    connect(response, SIGNAL(finished()),
            this, SLOT(fileFinished()));
    connect(response, SIGNAL(readyRead()),
            this, SLOT(fileReadyRead()));
    connect(response, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(fileProgress(qint64, qint64)));
    connect(response, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void DlgUpdate::downloadError(QNetworkReply::NetworkError) {
    //Alert the user to the error then close
    QMessageBox::critical(this, tr("Error"), tr(response->errorString().toUtf8()));
    accept();
}

void DlgUpdate::fileFinished() {

    //If we finished but there's a redirect, follow it
    QVariant redirect = response->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirect.isNull())
        beginDownload(redirect.toUrl());
    else
    {
        //Write out the rest of the file
        file->flush();
        file->close();

        //Handle any errors we had
        if (response->error())
            downloadError(response->error());

        //Tell the user it worked
        QMessageBox::information(this, tr("HTTP"),
                                 tr("Download successful!"));

        //Open the installer for the user
        QDesktopServices::openUrl(QUrl::fromLocalFile(file->fileName()));

        //Delete the file handle
        delete file;
        file = NULL;

        //Close the dialogue
        accept();
    }
}

void DlgUpdate::fileReadyRead() {
    //Write the HTTP result straight to the file
    if (file->isOpen())
        file->write(response->readAll());
    else
        qDebug() << "Cockatrice update file isn't open.";
}

void DlgUpdate::fileProgress(qint64 bytesRead, qint64 totalBytes) {

    //Update the progress bar
    setValue(bytesRead);
    setMaximum(totalBytes);

    //Alert the user if there are errors
    if (response->error())
        downloadError(response->error());
}