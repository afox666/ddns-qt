// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QSettings>
#include <QPushButton>
#include <QCheckBox>
#include <QHostAddress>
#include <QVBoxLayout>

#include "cloudflare.h"
#include "networkwidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onProviderChanged(int index);
    void saveConfig();
    void updateIPAddresses();
    void handleIPv4Reply(QNetworkReply *reply);
    void handleIPv6Reply(QNetworkReply *reply);
    void toggleDDNS();
    void updateDNS();
    void handleDDNSReply(QNetworkReply *reply);

private:
    void createCloudFlarePage();
    void createAliyunPage();
    void createDNSPodPage();
    void createDuckDNSPage();

    void onRefreshClicked();
    void setupControlGroup(QVBoxLayout *mainLayout);
    void setupIPAddressDisplay(QVBoxLayout *mainLayout);

    void loadConfig();
    void loadProviderConfig(const QString &provider);
    QString getConfigFilePath();

    void updateAliyun(const QString &ipv4, const QString &ipv6);
    void updateDNSPod(const QString &ipv4, const QString &ipv6);
    void updateDuckDNS(const QString &ipv4, const QString &ipv6);

    NetworkWidget *networkWidget;

    std::shared_ptr<Cloudflare> cloudflare_;

    QComboBox *providerCombo;
    QStackedWidget *stackedWidget;

    // IP address labels
    QLabel *ipv4Label;
    QLabel *ipv6Label;
    QCheckBox *ipv4CheckBox;
    QCheckBox *ipv6CheckBox;
    QLineEdit *ipv4RecordName;    // IPv4记录名
    QLineEdit *ipv6RecordName;    // IPv6记录名
    // 用于存储记录ID的变量
    QString ipv4RecordId;
    QString ipv6RecordId;

    QTimer *ipUpdateTimer;
    QNetworkAccessManager *networkManager;

    QPushButton *ddnsButton;
    QTimer *ddnsTimer;
    bool ddnsRunning;
    QString currentIPv4;
    QString currentIPv6;

    // Cloudflare inputs
    QLineEdit *cfEmail;
    QLineEdit *cfApiKey;
    QLineEdit *cfZoneId;
    QLineEdit *cfDomain;

    // Aliyun inputs
    QLineEdit *aliyunAccessKey;
    QLineEdit *aliyunSecretKey;
    QLineEdit *aliyunDomain;

    // DNSPod inputs
    QLineEdit *dnspodToken;
    QLineEdit *dnspodDomain;

    // DuckDNS inputs
    QLineEdit *duckdnsToken;
    QLineEdit *duckdnsDomain;
};

#endif // MAINWINDOW_H
