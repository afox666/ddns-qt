// networkwidget.cpp
#include "networkwidget.h"

NetworkWidget::NetworkWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refreshNetworkInterfaces();
}

void NetworkWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 创建网卡选择下拉框
    interfaceComboBox = new QComboBox(this);
    mainLayout->addWidget(interfaceComboBox);

    // 创建IP信息显示标签
    ipInfoLabel = new QLabel(this);
    ipInfoLabel->setWordWrap(true);
    mainLayout->addWidget(ipInfoLabel);

    // 连接信号槽
    connect(interfaceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NetworkWidget::onInterfaceSelected);

    setLayout(mainLayout);
}

void NetworkWidget::refreshNetworkInterfaces()
{
    interfaceComboBox->clear();

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    foreach(const QNetworkInterface &interface, interfaces) {
        // 跳过禁用的接口和回环接口
        if(!(interface.flags() & QNetworkInterface::IsUp) ||
            !(interface.flags() & QNetworkInterface::IsRunning) ||
            (interface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        // 获取接口信息组装显示文本
        QString displayText = interface.name() + " - " + interface.humanReadableName();

        // 将网卡对象存储在ComboBox的userData中
        interfaceComboBox->addItem(displayText, QVariant::fromValue(interface));
    }
}

QString NetworkWidget::getInterfaceInfo(const QNetworkInterface &interface)
{
    QString info;
    QTextStream stream(&info);

    // 获取IP地址信息
    QList<QNetworkAddressEntry> entries = interface.addressEntries();
    foreach(const QNetworkAddressEntry &entry, entries) {
        QHostAddress address = entry.ip();

        if(address.protocol() == QAbstractSocket::IPv4Protocol) {
            stream << "IPv4: " << address.toString() << "\n";
        }
        else if(address.protocol() == QAbstractSocket::IPv6Protocol) {
            stream << "IPv6: " << address.toString() << "\n";
        }
    }

    return info;
}

void NetworkWidget::onInterfaceSelected(int index)
{
    if(index < 0) {
        ipInfoLabel->clear();
        return;
    }

    // 获取选中的网卡接口
    QNetworkInterface interface = interfaceComboBox->currentData()
                                      .value<QNetworkInterface>();

    // 显示详细信息
    ipInfoLabel->setText(getInterfaceInfo(interface));
}
