#include <QUrl>
#include <QMessageBox>

#include "update_downloader.h"

UpdateDownloader::UpdateDownloader(QObject *parent) : QObject(parent) {
    netMan = new QNetworkAccessManager(this);
}

void UpdateDownloader::beginDownload(QUrl downloadUrl) {

    //Save the original URL because we need it for the filename
    if (originalUrl.isEmpty())
        originalUrl = downloadUrl;

    //Work out the file name of the download
    QString fileName = originalUrl.toString().section('/', -1);

    //Save the build in a temporary directory
    file = new QFile(QDir::temp().path() + QDir::separator() + fileName, this);
    if (file->open(QIODevice::WriteOnly)) {
        response = netMan->get(QNetworkRequest(downloadUrl));
        connect(response, SIGNAL(finished()),
                this, SLOT(fileFinished()));
        connect(response, SIGNAL(readyRead()),
                this, SLOT(fileReadyRead()));
        connect(response, SIGNAL(downloadProgress(qint64, qint64)),
                this, SLOT(downloadProgress(qint64, qint64)));
        connect(response, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(downloadError(QNetworkReply::NetworkError)));
    }
    else
            emit error("Could not open the file for reading.");

}

void UpdateDownloader::downloadError(QNetworkReply::NetworkError) {
    emit error(response->errorString().toUtf8());
}

void UpdateDownloader::fileFinished() {
    //If we finished but there's a redirect, follow it
    QVariant redirect = response->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirect.isNull())
        beginDownload(redirect.toUrl());
    else {
        QMessageBox::information((QWidget*)this->parent(), "Progress", "Finished downloading");

        //Write out the rest of the file
        file->flush();
        file->close();

        QMessageBox::information((QWidget*)this->parent(), "Progress", "File closed");

        //Handle any errors we had
        if (response->error()) {
            QMessageBox::information((QWidget*)this->parent(), "Error", response->errorString());
            emit error(response->errorString());
        }

        QMessageBox::information((QWidget*)this->parent(), "Progress", "No error. Emitting success...");

        //Emit the success signal with a URL to the download file
        emit downloadSuccessful(QUrl::fromLocalFile(file->fileName()));
    }
}

void UpdateDownloader::fileReadyRead() {
    //Write the HTTP result straight to the file
    if (file->isOpen())
        file->write(response->readAll());
    else
        emit error("File closed during writing!");
}

void UpdateDownloader::downloadProgress(qint64 bytesRead, qint64 totalBytes) {
    emit progressMade(bytesRead, totalBytes);
}

