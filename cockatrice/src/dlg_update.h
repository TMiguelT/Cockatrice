#ifndef DLG_UPDATE_H
#define DLG_UPDATE_H

#include <QtNetwork>
#include <QProgressDialog>

class DlgUpdate : public QProgressDialog {
Q_OBJECT
public:
    DlgUpdate(QWidget *parent, QString downloadUrlStr, int downloadSize);

private slots:
    void beginDownload(QUrl downloadUrl);
    void fileFinished();
    void fileReadyRead();
    void fileProgress(qint64 bytesRead, qint64 totalBytes);
    void downloadError(QNetworkReply::NetworkError);

private:
    QFile *file;
    QNetworkAccessManager netMan;
    QNetworkReply *response;
};

#endif
