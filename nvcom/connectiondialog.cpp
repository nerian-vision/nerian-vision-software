#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

#include <visiontransfer/deviceenumeration.h>
#include "connectiondialog.h"
#include "ui_connectiondialog.h"

#include <sstream>
#include <iostream>

using namespace visiontransfer;

static const int COLUMN_IP     = 0;
static const int COLUMN_PROTO  = 1;
static const int COLUMN_MODEL  = 2;
static const int COLUMN_FW     = 3;
static const int COLUMN_STATUS = 4;
static const int COLUMN_IDX    = 5;

ConnectionDialog::ConnectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectionDialog) {

    ui->setupUi(this);

    // Disable the connect and configure buttons until something is selected
    ui->connectButton->setEnabled(false);
    ui->configButton->setEnabled(false);

    ui->hostsList->sortByColumn(0, Qt::AscendingOrder);

    // Connect signals
    QObject::connect(ui->hostsList, &QTreeWidget::itemDoubleClicked,
        this, [this](QTreeWidgetItem *item, int column){connectToHost();});
    QObject::connect(ui->hostsList, &QTreeWidget::itemSelectionChanged,
        this, [this](){
            if(ui != nullptr) {
                ui->connectButton->setEnabled(ui->hostsList->selectedItems().size() > 0);
                ui->configButton->setEnabled(ui->hostsList->selectedItems().size() > 0);
            }
        });
    QObject::connect(ui->connectButton, &QPushButton::pressed,
        this, [this](){connectToHost();});
    QObject::connect(ui->configButton, &QPushButton::pressed,
        this, [this](){openConfigForHost();});
    QObject::connect(ui->cancelButton, &QPushButton::pressed,
        this, [this](){reject();});
    QObject::connect(&updateTimer, &QTimer::timeout,
        this, [this](){queryDevices();});

    ui->hostsList->header()->setMinimumSectionSize(80);
    ui->hostsList->header()->resizeSection(COLUMN_IP, 130);
    ui->hostsList->header()->resizeSection(COLUMN_PROTO, 80);
    ui->hostsList->header()->resizeSection(COLUMN_MODEL, 130);
    ui->hostsList->header()->resizeSection(COLUMN_FW, 80);
    ui->hostsList->header()->resizeSection(COLUMN_STATUS, 350);

    updateTimer.start(0);
}

ConnectionDialog::~ConnectionDialog() {
    delete ui;
    ui = nullptr;
}

void ConnectionDialog::queryDevices() {
    try {
        DeviceEnumeration devEnum;
        prevDeviceList = currDeviceList;
        currDeviceList = devEnum.discoverDevices();

        createHostList();

        updateTimer.start(UPDATE_INTERVAL_MS);
    } catch(const std::exception& ex) {
        std::cerr << "Exception occured: " << ex.what() << std::endl;
    }
}

void ConnectionDialog::createHostList() {
    // Save selection
    std::string selectedIp = "";
    int selectedIndex = -1;
    QList<QTreeWidgetItem *> selectedItems = ui->hostsList->selectedItems();
    if(selectedItems.size() > 0 && selectedItems[0] != nullptr) {
        selectedIp = selectedItems.first()->data(0, Qt::DisplayRole).toString().toUtf8().constData();
    }

    // Clear all elements
    while (ui->hostsList->topLevelItemCount() > 0) {
        ui->hostsList->takeTopLevelItem(0);
    }
    hostItems.clear();

    // Create item objects
    for(int i=0; i<static_cast<int>(currDeviceList.size()); i++) {
        hostItems.push_back(QTreeWidgetItem());
        hostItems.back().setData(COLUMN_IP, Qt::DisplayRole, currDeviceList[i].getIpAddress().c_str());
        hostItems.back().setData(COLUMN_PROTO, Qt::DisplayRole,
            currDeviceList[i].getNetworkProtocol() == DeviceInfo::PROTOCOL_TCP ? "TCP" : "UDP");

        std::string modelString;
        switch(currDeviceList[i].getModel()) {
            case DeviceInfo::SCENESCAN: modelString = "SceneScan"; break;
            case DeviceInfo::SCENESCAN_PRO: modelString = "SceneScan Pro"; break;
            case DeviceInfo::SCARLET: modelString = "Scarlet"; break;
            case DeviceInfo::RUBY: modelString = "Ruby"; break;
            default: modelString = "Unknown"; break;
        }
        hostItems.back().setData(COLUMN_MODEL, Qt::DisplayRole, modelString.c_str());
        hostItems.back().setData(COLUMN_FW, Qt::DisplayRole, currDeviceList[i].getFirmwareVersion().c_str());
        if(!currDeviceList[i].isCompatible()) {
            hostItems.back().setIcon(0, QIcon(":/nvcom/icons/warning.png"));
        }
        std::stringstream ss;
        ss.precision(2);
        auto status = currDeviceList[i].getStatus();
        if (status.isValid()) {
            if (status.getLastFps() > 0) {
                ss << "OK, " << std::fixed << status.getLastFps() << " fps.";
            } else {
                hostItems.back().setIcon(COLUMN_STATUS, QIcon(":/nvcom/icons/warning.png"));
                if (status.getCurrentCaptureSource() == "arv") {
                    ss << "No images";
                } else {
                    ss << "No images";
                }
            }
            if (status.getJumboFramesEnabled()) {
                ss << " Jumbo frames enabled.";
            } else {
                ss << " Jumbo frames disabled.";
            }
        } else {
            ss << "Legacy firmware, no status report.";
        }
        hostItems.back().setData(COLUMN_STATUS, Qt::DisplayRole, ss.str().c_str());
        hostItems.back().setData(COLUMN_IDX, Qt::DisplayRole, i);
        if(currDeviceList[i].getIpAddress() == selectedIp) {
            selectedIndex = hostItems.size()-1;
        }
    }

    // Insert objects
    for(unsigned int i=0; i<hostItems.size(); i++) {
        ui->hostsList->insertTopLevelItem(i, &hostItems[i]);
    }

    // Select the previously selected item or the first item in the list
    if(selectedIndex != -1) {
        hostItems[selectedIndex].setSelected(true);
    } else if(hostItems.size() > 0 && selectedIp == "") {
        ui->hostsList->itemAt(0,0)->setSelected(true);
    }
}

void ConnectionDialog::openConfigForHost() {
    QList<QTreeWidgetItem *> selectedItems = ui->hostsList->selectedItems();

    if(selectedItems.size() > 0 && selectedItems[0] != nullptr) {
        int index = selectedItems.first()->data(COLUMN_IDX, Qt::DisplayRole).toInt();
        std::string urlstr = "http://";
        urlstr += currDeviceList[index].getIpAddress();
        QDesktopServices::openUrl(QUrl(urlstr.c_str()));
    }
}

void ConnectionDialog::connectToHost() {
    QList<QTreeWidgetItem *> selectedItems = ui->hostsList->selectedItems();

    if(selectedItems.size() > 0 && selectedItems[0] != nullptr) {
        int index = selectedItems.first()->data(COLUMN_IDX, Qt::DisplayRole).toInt();
        if(!currDeviceList[index].isCompatible()) {
            QMessageBox msgBox(QMessageBox::Critical, "Incompatible Firmware!",
                "The selected device has an incompatible firmware version. Please "
                "either update the device firmware or your version of NVCom");
            msgBox.exec();
        } else if(index >= 0 && index < static_cast<int>(currDeviceList.size())) {
            selectedDevice = currDeviceList[index];
            accept();
        }
    }
}
