#include "imagerecognition.h"
#include "ui_imagerecognition.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QSettings>
#include <QCoreApplication>

ImageRecognition::ImageRecognition(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageRecognition)
    , camera(nullptr)
    , imageCapture(nullptr)
    , latestTime(0)
{
    ui->setupUi(this);

    // ==========================================
    // 1. 设备与定时器初始化
    // ==========================================
    camerasInfo = QCameraInfo::availableCameras();
    for(const QCameraInfo &cameraInfo : camerasInfo) {
        ui->comboBox->addItem(cameraInfo.description());
    }
    connect(ui->comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageRecognition::pickCamera);

    camera = new QCamera(this);
    finder = new QCameraViewfinder(this);
    imageCapture = new QCameraImageCapture(camera, this);

    camera->setViewfinder(finder);
    camera->setCaptureMode(QCamera::CaptureStillImage);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

    connect(imageCapture, &QCameraImageCapture::imageCaptured, this, &ImageRecognition::showCamera);
    connect(ui->pushButton, &QPushButton::clicked, this, &ImageRecognition::controlWorker);
    camera->start();

    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ImageRecognition::takePicture);
    refreshTimer->start(33); // 约 30 帧，兼顾流畅度与性能

    netTimer = new QTimer(this);
    connect(netTimer, &QTimer::timeout, this, &ImageRecognition::controlWorker);

    // ==========================================
    // 2. 界面布局与样式
    // ==========================================
    this->resize(1280, 720);
    this->setWindowTitle("AI 智能人脸识别监控系统");

    finder->hide(); // 隐藏原生 finder，接管画面绘制

    this->setStyleSheet(R"(
        QWidget { background-color: #1E1E2E; color: #CDD6F4; font-family: "Microsoft YaHei"; font-size: 14px; }
        QLabel#label { background-color: #11111B; border: 2px solid #313244; border-radius: 12px; padding: 5px; }
        QPushButton { background-color: #89B4FA; color: #11111B; border: none; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 16px; }
        QPushButton:hover { background-color: #74C7EC; }
        QPushButton:pressed { background-color: #89DCEB; padding-top: 12px; padding-bottom: 8px; }
        QComboBox { background-color: #313244; border: 1px solid #45475A; border-radius: 6px; padding: 8px 12px; color: #CDD6F4; }
        QComboBox:hover { border: 1px solid #89B4FA; }
        QTextBrowser { background-color: #181825; border: 1px solid #313244; border-radius: 8px; padding: 10px; color: #A6E3A1; font-family: "Consolas", monospace; line-height: 1.5; }
        QScrollBar:vertical { border: none; background: #1E1E2E; width: 8px; border-radius: 4px; }
        QScrollBar::handle:vertical { background: #45475A; min-height: 20px; border-radius: 4px; }
        QScrollBar::handle:vertical:hover { background: #585B70; }
    )");

    // 【布局调整】：增加底部边距，增大底部空间
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 50); // 底部 margin 设为 50
    mainLayout->setSpacing(25);

    // 左侧：监控核心区
    QVBoxLayout* leftLayout = new QVBoxLayout();

    
    ui->label->setMinimumSize(800, 450);
    ui->label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->label->setAlignment(Qt::AlignCenter);

    QLabel* videoTitle = new QLabel("实时视频监控", this);
    videoTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #89B4FA; background: transparent; margin-bottom: 8px;");

    leftLayout->addWidget(videoTitle, 0);
    leftLayout->addWidget(ui->label, 1);

    // 右侧：控制与数据区
    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    QLabel* controlTitle = new QLabel("设备与控制", this);
    controlTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #89B4FA; background: transparent;");

    QLabel* dataTitle = new QLabel("AI 结构化数据分析", this);
    dataTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #89B4FA; background: transparent; margin-top: 15px;");

    rightLayout->addWidget(controlTitle, 0);
    rightLayout->addWidget(ui->comboBox, 0);
    rightLayout->addWidget(ui->pushButton, 0);
    rightLayout->addWidget(dataTitle, 0);
    rightLayout->addWidget(ui->textBrowser, 1);

    mainLayout->addLayout(leftLayout, 7);
    mainLayout->addLayout(rightLayout, 3);
    this->setLayout(mainLayout);

    // ==========================================
    // 3. 网络与配置初始化
    // ==========================================
    tokenManager = new QNetworkAccessManager(this);
    connect(tokenManager, &QNetworkAccessManager::finished, this, &ImageRecognition::tokenReply);

    imgManager = new QNetworkAccessManager(this);
    connect(imgManager, &QNetworkAccessManager::finished, this, &ImageRecognition::imgReply);

    sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2);

    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(configPath, QSettings::IniFormat);
    QString clientId = settings.value("BaiduAI/client_id", "").toString();
    QString clientSecret = settings.value("BaiduAI/client_secret", "").toString();

    if (clientId.isEmpty() || clientSecret.isEmpty() || clientId.contains("API_KEY")) {
        settings.setValue("BaiduAI/client_id", "请在此处填入你的_API_KEY");
        settings.setValue("BaiduAI/client_secret", "请在此处填入你的_SECRET_KEY");
        ui->textBrowser->setText(QString("【系统提示：缺少配置文件】\n\n请在以下路径打开 config.ini 文件并填入您的百度 AI 密钥：\n\n%1\n\n填写完毕后请重启本软件。").arg(configPath));
        return;
    }

    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", clientId);
    query.addQueryItem("client_secret", clientSecret);
    url.setQuery(query);

    QNetworkRequest req;
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);
    tokenManager->get(req);
}

ImageRecognition::~ImageRecognition()
{
    delete ui;
}

void ImageRecognition::showCamera(int id, QImage preview)
{
    Q_UNUSED(id);
    img = preview;

    QPainter p(&preview);
    QPen pen(QColor("#89B4FA")); 
    pen.setWidth(4);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);

    if (!cachedFaceLabels.isEmpty()) {
        p.drawRect(faceLeft, faceTop, faceWidth, faceHeight);

        QFont font;
        font.setPixelSize(28);
        p.setFont(font);

        p.setPen(QColor("#FFFFFF")); 
        int textY = faceTop;
        for(const QString& text : qAsConst(cachedFaceLabels)) {
            p.drawText(faceLeft + faceWidth + 15, textY, text);
            textY += 35;
        }
    }

    // 【比例修复】：保持宽高比缩放，防止人脸和画面变形
    ui->label->setPixmap(QPixmap::fromImage(preview).scaled(ui->label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ImageRecognition::takePicture()
{
    if (imageCapture && imageCapture->isReadyForCapture()) {
        imageCapture->capture();
    }
}

void ImageRecognition::tokenReply(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        ui->textBrowser->setText("【网络错误】\n" + reply->errorString());
        reply->deleteLater();
        return;
    }

    const QByteArray reply_data = reply->readAll();
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply_data, &jsonErr);

    if(jsonErr.error == QJsonParseError::NoError) {
        QJsonObject obj = doc.object();
        if(obj.contains("access_token")) {
            accessToken = obj.take("access_token").toString();
            ui->textBrowser->setText("【系统就绪】\nToken 鉴权成功，正在进行实时识别...");
            netTimer->start(1000);
        } else if (obj.contains("error_description")) {
            ui->textBrowser->setText("【鉴权失败】\n请检查 config.ini 中的密钥是否正确！\n错误详情: " + obj.value("error_description").toString());
        }
    }
    reply->deleteLater();
}

void ImageRecognition::controlWorker()
{
    QImage processImg = this->img;
    if(processImg.isNull() || accessToken.isEmpty()) return;

    QFuture<QByteArray> future = QtConcurrent::run([processImg]() {
        QByteArray ba;
        QBuffer buff(&ba);
        buff.open(QIODevice::WriteOnly);
        processImg.save(&buff, "png");

        QJsonObject postJson;
        postJson.insert("image", ba.toBase64().data());
        postJson.insert("image_type", "BASE64");
        postJson.insert("face_field", "age,expression,face_shape,gender,glasses,quality,eye_status,emotion,face_type,mask,beauty");
        postJson.insert("liveness_control", "NORMAL");

        return QJsonDocument(postJson).toJson(QJsonDocument::Compact);
    });

    auto watcher = new QFutureWatcher<QByteArray>(this);
    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, watcher]() {
        this->beginFaceDetect(watcher->result());
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void ImageRecognition::pickCamera(int idx)
{
    refreshTimer->stop();
    if (camera) { camera->stop(); camera->deleteLater(); }
    if (imageCapture) { imageCapture->deleteLater(); }

    camera = new QCamera(camerasInfo.at(idx), this);
    imageCapture = new QCameraImageCapture(camera, this);

    connect(imageCapture, &QCameraImageCapture::imageCaptured, this, &ImageRecognition::showCamera);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    camera->setViewfinder(finder);

    camera->start();
    refreshTimer->start(33);
}

void ImageRecognition::beginFaceDetect(QByteArray postData)
{
    QUrl url("https://aip.baidubce.com/rest/2.0/face/v3/detect");
    QUrlQuery query;
    query.addQueryItem("access_token", accessToken);
    url.setQuery(query);

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    req.setUrl(url);
    req.setSslConfiguration(sslConfig);

    imgManager->post(req, postData);
}

void ImageRecognition::imgReply(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &jsonErr);

    if(jsonErr.error == QJsonParseError::NoError) {
        QJsonObject obj = doc.object();

        if(obj.contains("timestamp")) {
            int tmpTime = obj.take("timestamp").toInt();
            if(tmpTime < latestTime) { reply->deleteLater(); return; }
            else { latestTime = tmpTime; }
        }

        if(obj.contains("result") && !obj["result"].isNull()) {
            QJsonObject resultObj = obj.take("result").toObject();
            if(resultObj.contains("face_list")) {
                QJsonArray faceList = resultObj.take("face_list").toArray();
                if (faceList.isEmpty()) {
                    cachedFaceLabels.clear();
                    reply->deleteLater();
                    return;
                }

                QJsonObject faceObject = faceList.at(0).toObject();
                QString faceInfo = "【最新分析结果】\r\n";

                if(faceObject.contains("location")) {
                    QJsonObject locObj = faceObject.take("location").toObject();
                    faceLeft = locObj.value("left").toDouble();
                    faceTop = locObj.value("top").toDouble();
                    faceWidth = locObj.value("width").toDouble();
                    faceHeight = locObj.value("height").toDouble();
                }

                cachedFaceLabels.clear();

                auto addField = [&](const QString& label, const QString& value) {
                    QString str = label + "：" + value;
                    faceInfo += " - " + str + "\r\n";
                    cachedFaceLabels << str;
                };

                if(faceObject.contains("age"))
                    addField("年龄", QString::number(faceObject.value("age").toDouble()));

                if(faceObject.contains("gender"))
                    addField("性别", faceObject.value("gender").toObject().value("type").toString() == "male" ? "男" : "女");

                if(faceObject.contains("emotion"))
                    addField("情绪", faceObject.value("emotion").toObject().value("type").toString());

                if(faceObject.contains("beauty"))
                    addField("颜值", QString::number(faceObject.value("beauty").toDouble()));

                if(faceObject.contains("mask"))
                    addField("口罩", faceObject.value("mask").toObject().value("type").toInt() ? "已佩戴" : "未佩戴");

                if(faceObject.contains("glasses")) {
                    QString gls = faceObject.value("glasses").toObject().value("type").toString();
                    addField("眼镜", gls == "none" ? "未佩戴" : (gls == "common" ? "普通眼镜" : "墨镜"));
                }

                if(faceObject.contains("liveness"))
                    addField("活体分数", QString::number(faceObject.value("liveness").toObject().value("livemapscore").toDouble(), 'f', 4));

                ui->textBrowser->setText(faceInfo);
            }
        } else {
            cachedFaceLabels.clear();
        }
    }
    reply->deleteLater();
}
