#ifndef IMAGERECOGNITION_H
#define IMAGERECOGNITION_H

#include <QWidget>
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QJsonArray>
#include <QPainter>
#include <QStringList>
#include <QCameraInfo>

QT_BEGIN_NAMESPACE
namespace Ui {
class ImageRecognition;
}
QT_END_NAMESPACE

class ImageRecognition : public QWidget
{
    Q_OBJECT

public:
    ImageRecognition(QWidget *parent = nullptr);
    ~ImageRecognition();

public slots:
    void showCamera(int id, QImage preview);
    void takePicture();
    void tokenReply(QNetworkReply *reply);
    void beginFaceDetect(QByteArray postData);
    void imgReply(QNetworkReply *reply);
    void controlWorker();
    void pickCamera(int idx);

private:
    Ui::ImageRecognition *ui;

    QCamera* camera;
    QCameraViewfinder* finder;
    QCameraImageCapture* imageCapture;
    QTimer* refreshTimer;
    QTimer* netTimer;
    QNetworkAccessManager* tokenManager;
    QNetworkAccessManager* imgManager;
    QSslConfiguration sslConfig;
    QString accessToken;
    QImage img;

    // 人脸数据与识别状态
    double faceLeft;
    double faceTop;
    double faceWidth;
    double faceHeight;
    double liveness;
    double age;
    QString gender;
    QString emotion;
    double beauty;
    int mask;
    QString glasses;
    int latestTime;

    QStringList cachedFaceLabels; // 用于缓存渲染文本，提升帧率
    QList<QCameraInfo> camerasInfo;
};
#endif // IMAGERECOGNITION_H