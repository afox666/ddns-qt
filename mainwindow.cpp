// mainwindow.cpp
#include "mainwindow.h"

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
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QDateTime>
#include <QGroupBox>
#include <QRegExp>
#include <QJsonArray>
#include <QNetworkInterface>

MainWindow::~MainWindow() {}

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

    setupControlGroup(mainLayout);
    // 设置IP地址显示
    setupIPAddressDisplay(mainLayout);

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

    // 创建记录设置组
    QGroupBox *recordGroup = new QGroupBox("Record Settings", this);
    QGridLayout *recordLayout = new QGridLayout(recordGroup);

    // IPv4记录设置
    QLabel *ipv4RecordLabel = new QLabel("IPv4 Record Name:", this);
    ipv4RecordName = new QLineEdit(this);
    ipv4RecordName->setPlaceholderText("e.g., home");
    recordLayout->addWidget(ipv4RecordLabel, 0, 0);
    recordLayout->addWidget(ipv4RecordName, 0, 1);

    // IPv6记录设置
    QLabel *ipv6RecordLabel = new QLabel("IPv6 Record Name:", this);
    ipv6RecordName = new QLineEdit(this);
    ipv6RecordName->setPlaceholderText("e.g., home6");
    recordLayout->addWidget(ipv6RecordLabel, 1, 0);
    recordLayout->addWidget(ipv6RecordName, 1, 1);

    providerLayout->addWidget(recordGroup);

    // 创建堆叠窗口
    stackedWidget = new QStackedWidget(this);

    // 创建各个提供商的配置界面
    createCloudFlarePage();
    createAliyunPage();
    createDNSPodPage();
    createDuckDNSPage();

    // 添加所有部件到主布局
    mainLayout->addLayout(providerLayout);
    mainLayout->addWidget(stackedWidget);
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

void MainWindow::onRefreshClicked()
{
    updateIPAddresses();
}

void MainWindow::setupControlGroup(QVBoxLayout *mainLayout)
{
    // 创建顶部控制区域
    QGroupBox *controlGroup = new QGroupBox("Control Panel", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    // 添加控制按钮
    QPushButton *refreshBtn = new QPushButton("Refresh IP", this);
    // 连接信号槽
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);

    // 创建DDNS按钮
    ddnsButton = new QPushButton("Start DDNS", this);
    connect(ddnsButton, &QPushButton::clicked, this, &MainWindow::toggleDDNS);
    // 创建DDNS更新定时器
    ddnsTimer = new QTimer(this);
    connect(ddnsTimer, &QTimer::timeout, this, &MainWindow::updateDNS);

    // 创建保存按钮
    QPushButton *saveButton = new QPushButton("Save Configuration", this);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveConfig);

    // 在保存按钮之后添加DDNS按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(ddnsButton);

    controlLayout->addWidget(refreshBtn);
    controlLayout->addLayout(buttonLayout);
    controlLayout->addStretch(); // 添加弹簧，使按钮靠左对齐

    // 将分组添加到主布局
    mainLayout->addWidget(controlGroup);
}

void MainWindow::setupIPAddressDisplay(QVBoxLayout *mainLayout)
{
    QGroupBox *ipGroup = new QGroupBox("Current IP Addresses", this);
    QVBoxLayout *ipLayout = new QVBoxLayout(ipGroup);

    // IPv4 行布局
    QHBoxLayout *ipv4Layout = new QHBoxLayout();
    QLabel *ipv4Title = new QLabel("IPv4:", this);
    ipv4Label = new QLabel("Checking...", this);
    ipv4CheckBox = new QCheckBox("Enable IPv4 DDNS", this);
    ipv4CheckBox->setEnabled(false); // 初始禁用，直到检测到有效IP
    ipv4Layout->addWidget(ipv4Title);
    ipv4Layout->addWidget(ipv4Label);
    ipv4Layout->addWidget(ipv4CheckBox);
    ipv4Layout->addStretch();

    // IPv6 行布局
    QHBoxLayout *ipv6Layout = new QHBoxLayout();
    QLabel *ipv6Title = new QLabel("IPv6:", this);
    ipv6Label = new QLabel("Checking...", this);
    ipv6CheckBox = new QCheckBox("Enable IPv6 DDNS", this);
    ipv6CheckBox->setEnabled(false); // 初始禁用，直到检测到有效IP
    ipv6Layout->addWidget(ipv6Title);
    ipv6Layout->addWidget(ipv6Label);
    ipv6Layout->addWidget(ipv6CheckBox);
    ipv6Layout->addStretch();

    ipLayout->addLayout(ipv4Layout);
    ipLayout->addLayout(ipv6Layout);

    mainLayout->addWidget(ipGroup);
}

void MainWindow::updateIPAddresses()
{
    qDebug() << "update ip address";

    ipv4Label->setText("...");
    ipv6Label->setText("...");

    // 获取IPv4地址
    QNetworkRequest requestv4(QUrl("https://api.ipify.org?format=json"));
    QNetworkReply *replyv4 = networkManager->get(requestv4);
    connect(replyv4, &QNetworkReply::finished, [this, replyv4]() {
        handleIPv4Reply(replyv4);
    });

    // 获取IPv6地址
    QNetworkRequest requestv6(QUrl("https://api6.ipify.org?format=json"));
    QNetworkReply *replyv6 = networkManager->get(requestv6);
    connect(replyv6, &QNetworkReply::finished, [this, replyv6]() {
        handleIPv6Reply(replyv6);
    });
}

void MainWindow::handleIPv4Reply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString ipv4 = QString::fromUtf8(reply->readAll()).trimmed();
        QHostAddress address(ipv4);
        if (!ipv4.isEmpty() && address.protocol() == QAbstractSocket::IPv4Protocol) {
            ipv4Label->setText(ipv4);
            ipv4CheckBox->setEnabled(true);
            ipv4CheckBox->setChecked(true);
        } else {
            ipv4Label->setText("No public IPv4");
            ipv4CheckBox->setEnabled(false);
            ipv4CheckBox->setChecked(false);
        }
    } else {
        ipv4Label->setText("Failed to get IPv4");
        ipv4CheckBox->setEnabled(false);
        ipv4CheckBox->setChecked(false);
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
            ipv6CheckBox->setEnabled(true);
            ipv6CheckBox->setChecked(true);
        } else {
            ipv6Label->setText("No public IPv6");
            ipv6CheckBox->setEnabled(false);
            ipv6CheckBox->setChecked(false);
        }
    } else {
        ipv6Label->setText("Failed to get IPv6");
        ipv6CheckBox->setEnabled(false);
        ipv6CheckBox->setChecked(false);
    }
    reply->deleteLater();
}

void MainWindow::toggleDDNS()
{
    if (!ddnsRunning) {
        // 检查是否有选中的IP地址
        if (!ipv4CheckBox->isChecked() && !ipv6CheckBox->isChecked()) {
            QMessageBox::warning(this, "DDNS Error",
                                 "Please select at least one IP address type for DDNS update.");
            return;
        }

        saveConfig();

        // 开始DDNS服务
        ddnsRunning = true;
        ddnsButton->setText("Stop DDNS");
        updateDNS(); // 立即执行一次更新
        ddnsTimer->start(300000); // 每5分钟更新一次
        QMessageBox::information(this, "DDNS Service",
                                 "DDNS service started for " +
                                     QString(ipv4CheckBox->isChecked() ? "IPv4" : "") +
                                     QString((ipv4CheckBox->isChecked() && ipv6CheckBox->isChecked()) ? " and " : "") +
                                     QString(ipv6CheckBox->isChecked() ? "IPv6" : ""));
    } else {
        // 停止DDNS服务
        ddnsRunning = false;
        ddnsButton->setText("Start DDNS");
        ddnsTimer->stop();
        QMessageBox::information(this, "DDNS Service", "DDNS service stopped");
    }
}

void MainWindow::updateDNS()
{
    // 检查是否有选中的IP地址
    bool hasIpv4 = ipv4CheckBox->isEnabled() && ipv4CheckBox->isChecked();
    bool hasIpv6 = ipv6CheckBox->isEnabled() && ipv6CheckBox->isChecked();

    if (!hasIpv4 && !hasIpv6) {
        QMessageBox::warning(this, "DDNS Error",
                             "No public IP address selected for DDNS update. Please check your network connection and IP selection.");
        return;
    }

    QString ipv4 = hasIpv4 ? ipv4Label->text() : QString();
    QString ipv6 = hasIpv6 ? ipv6Label->text() : QString();

    // 确保选中的IP地址是有效的
    if (hasIpv4 && (ipv4 == "Checking..." || ipv4 == "Failed to get IPv4" || ipv4 == "No public IPv4")) {
        QMessageBox::warning(this, "DDNS Error", "Invalid IPv4 address");
        return;
    }

    if (hasIpv6 && (ipv6 == "Checking..." || ipv6 == "Failed to get IPv6" || ipv6 == "No public IPv6")) {
        QMessageBox::warning(this, "DDNS Error", "Invalid IPv6 address");
        return;
    }

    // 执行DDNS更新
    QString provider = providerCombo->currentText();
    if (provider == "Cloudflare") {
        cloudflare_ = std::make_shared<Cloudflare>(networkManager);
        cloudflare_->updateCloudflare(cfApiKey, cfZoneId, cfDomain, ipv4, ipv6, ipv4RecordName, ipv6RecordName);
    } else if (provider == "Aliyun") {
        updateAliyun(ipv4, ipv6);
    } else if (provider == "DNSPod") {
        updateDNSPod(ipv4, ipv6);
    } else if (provider == "DuckDNS") {
        updateDuckDNS(ipv4, ipv6);
    }
}

void MainWindow::updateAliyun(const QString &ipv4, const QString &ipv6)
{
    // 获取配置
    QString accessKey = aliyunAccessKey->text();
    QString secretKey = aliyunSecretKey->text();
    QString domain = aliyunDomain->text();

    if (accessKey.isEmpty() || secretKey.isEmpty() || domain.isEmpty()) {
        QMessageBox::warning(this, "Configuration Error", "Please fill in all Aliyun settings");
        return;
    }

    // 构建阿里云API请求
    // 这里需要实现阿里云的API调用逻辑
    // 参考：https://help.aliyun.com/document_detail/29776.html
}

void MainWindow::updateDNSPod(const QString &ipv4, const QString &ipv6)
{
    // 获取配置
    QString token = dnspodToken->text();
    QString domain = dnspodDomain->text();

    if (token.isEmpty() || domain.isEmpty()) {
        QMessageBox::warning(this, "Configuration Error", "Please fill in all DNSPod settings");
        return;
    }

    // 构建DNSPod API请求
    // 这里需要实现DNSPod的API调用逻辑
    // 参考：https://docs.dnspod.cn/api/
}

void MainWindow::updateDuckDNS(const QString &ipv4, const QString &ipv6)
{
    // 获取配置
    QString token = duckdnsToken->text();
    QString domain = duckdnsDomain->text();

    if (token.isEmpty() || domain.isEmpty()) {
        QMessageBox::warning(this, "Configuration Error", "Please fill in all DuckDNS settings");
        return;
    }

    // 构建DuckDNS API请求
    QString subdomain = domain.split(".").first();
    QUrl url(QString("https://www.duckdns.org/update?domains=%1&token=%2&ip=%3&ipv6=%4")
                 .arg(subdomain)
                 .arg(token)
                 .arg(ipv4)
                 .arg(ipv6));

    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        handleDDNSReply(reply);
    });
}

void MainWindow::handleDDNSReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QString response = reply->readAll();
        qDebug() << "DDNS update response:" << response;
        // 可以在这里添加更详细的响应处理逻辑
    } else {
        qDebug() << "DDNS update error:" << reply->errorString();
        QMessageBox::warning(this, "DDNS Update Error",
                             "Failed to update DNS records: " + reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::createCloudFlarePage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);

    cfApiKey = new QLineEdit(page);
    cfZoneId = new QLineEdit(page);
    cfDomain = new QLineEdit(page);

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

