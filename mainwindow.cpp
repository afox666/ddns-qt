// mainwindow.cpp
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    setWindowTitle("DDNS Configuration");
    setMinimumSize(400, 300);

    // 创建中央窗口部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 设置IP地址显示
    setupIPAddressDisplay();

    // 创建服务提供商选择部分
    QHBoxLayout *providerLayout = new QHBoxLayout();
    QLabel *providerLabel = new QLabel("DDNS Provider:", this);
    providerCombo = new QComboBox(this);
    providerCombo->addItems({"Cloudflare", "Aliyun", "DNSPod", "DuckDNS"});
    connect(providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProviderChanged);

    providerLayout->addWidget(providerLabel);
    providerLayout->addWidget(providerCombo);
    providerLayout->addStretch();

    // 创建堆叠窗口
    stackedWidget = new QStackedWidget(this);

    // 创建各个提供商的配置界面
    createCloudFlarePage();
    createAliyunPage();
    createDNSPodPage();
    createDuckDNSPage();

    // 创建保存按钮
    QPushButton *saveButton = new QPushButton("Save Configuration", this);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveConfig);

    // 添加所有部件到主布局
    mainLayout->addLayout(providerLayout);
    mainLayout->addWidget(stackedWidget);
    mainLayout->addWidget(saveButton);
    mainLayout->addStretch();

    loadConfig();

    // 初始更新IP地址
    updateIPAddresses();

    // 设置定时器每5分钟更新一次IP地址
    ipUpdateTimer = new QTimer(this);
    connect(ipUpdateTimer, &QTimer::timeout, this, &MainWindow::updateIPAddresses);
    ipUpdateTimer->start(300000); // 5分钟 = 300000毫秒
}

QString MainWindow::getConfigFilePath()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return configPath + "/config.json";
}

void MainWindow::loadConfig()
{
    QFile file(getConfigFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open config file for reading";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Failed to parse config file";
        return;
    }

    QJsonObject config = doc.object();

    // 加载上次选择的提供商
    QString lastProvider = config["last_provider"].toString();
    int index = providerCombo->findText(lastProvider);
    if (index >= 0) {
        providerCombo->setCurrentIndex(index);
    }

    // 加载各提供商的配置
    if (config.contains("providers")) {
        QJsonObject providers = config["providers"].toObject();

        // Cloudflare
        if (providers.contains("Cloudflare")) {
            QJsonObject cf = providers["Cloudflare"].toObject();
            cfEmail->setText(cf["email"].toString());
            cfApiKey->setText(cf["api_key"].toString());
            cfZoneId->setText(cf["zone_id"].toString());
            cfDomain->setText(cf["domain"].toString());
        }

        // Aliyun
        if (providers.contains("Aliyun")) {
            QJsonObject aliyun = providers["Aliyun"].toObject();
            aliyunAccessKey->setText(aliyun["access_key"].toString());
            aliyunSecretKey->setText(aliyun["secret_key"].toString());
            aliyunDomain->setText(aliyun["domain"].toString());
        }

        // DNSPod
        if (providers.contains("DNSPod")) {
            QJsonObject dnspod = providers["DNSPod"].toObject();
            dnspodToken->setText(dnspod["token"].toString());
            dnspodDomain->setText(dnspod["domain"].toString());
        }

        // DuckDNS
        if (providers.contains("DuckDNS")) {
            QJsonObject duckdns = providers["DuckDNS"].toObject();
            duckdnsToken->setText(duckdns["token"].toString());
            duckdnsDomain->setText(duckdns["domain"].toString());
        }
    }
}

void MainWindow::setupIPAddressDisplay()
{
    QWidget *ipWidget = new QWidget(this);
    QFormLayout *ipLayout = new QFormLayout(ipWidget);

    ipv4Label = new QLabel("Checking...", this);
    ipv6Label = new QLabel("Checking...", this);

    ipLayout->addRow("IPv4 Address:", ipv4Label);
    ipLayout->addRow("IPv6 Address:", ipv6Label);

    // 将IP地址显示添加到主布局
    centralWidget()->layout()->addWidget(ipWidget);
}

void MainWindow::updateIPAddresses()
{
    // 获取IPv4地址
    QNetworkRequest requestv4(QUrl("https://api.ipify.org?format=json"));
    QNetworkReply *replyv4 = networkManager->get(requestv4);
    connect(replyv4, &QNetworkReply::finished, [this, replyv4]() {
        handleIPv4Reply(replyv4);
    });

    // 获取IPv6地址
    QNetworkRequest requestv6(QUrl("https://api64.ipify.org?format=json"));
    QNetworkReply *replyv6 = networkManager->get(requestv6);
    connect(replyv6, &QNetworkReply::finished, [this, replyv6]() {
        handleIPv6Reply(replyv6);
    });
}

void MainWindow::handleIPv4Reply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            QString ip = obj["ip"].toString();
            ipv4Label->setText(ip);
        }
    } else {
        ipv4Label->setText("Failed to get IPv4");
    }
    reply->deleteLater();
}

void MainWindow::handleIPv6Reply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            QString ip = obj["ip"].toString();
            ipv6Label->setText(ip);
        }
    } else {
        ipv6Label->setText("Failed to get IPv6");
    }
    reply->deleteLater();
}

MainWindow::~MainWindow()
{
}

void MainWindow::createCloudFlarePage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);

    cfEmail = new QLineEdit(page);
    cfApiKey = new QLineEdit(page);
    cfZoneId = new QLineEdit(page);
    cfDomain = new QLineEdit(page);

    layout->addRow("Email:", cfEmail);
    layout->addRow("API Key:", cfApiKey);
    layout->addRow("Zone ID:", cfZoneId);
    layout->addRow("Domain:", cfDomain);

    stackedWidget->addWidget(page);
}

void MainWindow::createAliyunPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);

    aliyunAccessKey = new QLineEdit(page);
    aliyunSecretKey = new QLineEdit(page);
    aliyunDomain = new QLineEdit(page);

    layout->addRow("Access Key:", aliyunAccessKey);
    layout->addRow("Secret Key:", aliyunSecretKey);
    layout->addRow("Domain:", aliyunDomain);

    stackedWidget->addWidget(page);
}

void MainWindow::createDNSPodPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);

    dnspodToken = new QLineEdit(page);
    dnspodDomain = new QLineEdit(page);

    layout->addRow("API Token:", dnspodToken);
    layout->addRow("Domain:", dnspodDomain);

    stackedWidget->addWidget(page);
}

void MainWindow::createDuckDNSPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);

    duckdnsToken = new QLineEdit(page);
    duckdnsDomain = new QLineEdit(page);

    layout->addRow("Token:", duckdnsToken);
    layout->addRow("Domain:", duckdnsDomain);

    stackedWidget->addWidget(page);
}

void MainWindow::onProviderChanged(int index)
{
    stackedWidget->setCurrentIndex(index);
}

void MainWindow::saveConfig()
{
    QJsonObject config;
    QJsonObject providers;

    // 保存当前选择的提供商
    config["last_provider"] = providerCombo->currentText();

    // Cloudflare配置
    QJsonObject cloudflare;
    cloudflare["email"] = cfEmail->text();
    cloudflare["api_key"] = cfApiKey->text();
    cloudflare["zone_id"] = cfZoneId->text();
    cloudflare["domain"] = cfDomain->text();
    providers["Cloudflare"] = cloudflare;

    // Aliyun配置
    QJsonObject aliyun;
    aliyun["access_key"] = aliyunAccessKey->text();
    aliyun["secret_key"] = aliyunSecretKey->text();
    aliyun["domain"] = aliyunDomain->text();
    providers["Aliyun"] = aliyun;

    // DNSPod配置
    QJsonObject dnspod;
    dnspod["token"] = dnspodToken->text();
    dnspod["domain"] = dnspodDomain->text();
    providers["DNSPod"] = dnspod;

    // DuckDNS配置
    QJsonObject duckdns;
    duckdns["token"] = duckdnsToken->text();
    duckdns["domain"] = duckdnsDomain->text();
    providers["DuckDNS"] = duckdns;

    config["providers"] = providers;

    // 保存到文件
    QJsonDocument doc(config);
    QFile file(getConfigFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Error", "Could not save configuration file.");
        return;
    }

    file.write(doc.toJson());
    QMessageBox::information(this, "Success", "Configuration saved successfully.");
}

