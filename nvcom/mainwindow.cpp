/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "nvcom.h"
#include "connectiondialog.h"
#include "settingsdialog.h"
#include "displaywidget.h"
#include "qtopen3dvisualizer.h"

#include <numeric>
#include <sstream>

#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QFontDatabase>
#include <QComboBox>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLabel>
#include <QScrollArea>
#include <QSurfaceFormat>
#include <QFileDialog>

#ifdef _WIN32
#include <windows.h>
#endif

#define DEMO_CYCLE_INTERVAL 15

using namespace std::chrono;
using namespace std;
using namespace cv;
using namespace visiontransfer;

MainWindow::MainWindow(QWidget *parent, QApplication& app): QMainWindow(parent),
    writeDirSelected(false), fpsLabel(nullptr), sizeLabel(nullptr), droppedLabel(nullptr),
    appSettings("nerian.com", "nvcom"), fpsTimer(this), scrollArea(nullptr),
    displayWidget(nullptr), open3dWidget(nullptr), closeAfterSend(false),
    resizeWindow(false), lastDropped(0), lastNumPoints(0) {

    QObject::connect(&app, &QApplication::lastWindowClosed, this,
        &MainWindow::writeApplicationSettings);

    fpsTimer.setInterval(200);
    for(int i=0; i<6; i++) {
        fpsCounters.push_back(std::make_pair(0, steady_clock::now()));
    }

    qRegisterMetaType<std::vector<int> >("std::vector<int>");
}

MainWindow::~MainWindow() {
    if(nvcom != nullptr) {
        nvcom->terminate();
    }
}

void MainWindow::writeApplicationSettings() {
    // Write settings before exit
    if(!fullscreen) {
        appSettings.setValue("geometry", saveGeometry());
        appSettings.setValue("state", saveState(UI_VERSION));
    }

    appSettings.setValue("write_left", settings.writeLeft);
    appSettings.setValue("write_right", settings.writeRight);
    appSettings.setValue("write_third_color", settings.writeThirdColor);
    appSettings.setValue("write_disparity_raw", settings.writeDisparityRaw);
    appSettings.setValue("write_disparity_color", settings.writeDisparityColor);
    appSettings.setValue("write_pgm", settings.writePgm);
    appSettings.setValue("display_coordinate", settings.displayCoordinate);
    appSettings.setValue("write_point_cloud", settings.writePointCloud);
    appSettings.setValue("point_cloud_max_dist", settings.pointCloudMaxDist);
    appSettings.setValue("color_scheme", settings.colorScheme);
    appSettings.setValue("max_frame_rate", settings.maxFrameRate);
    appSettings.setValue("read_dir", settings.readDir.c_str());
    appSettings.setValue("write_dir", settings.writeDir.c_str());
    appSettings.setValue("write_dir_always_ask", settings.writeDirAlwaysAsk);
    appSettings.setValue("zoom", settings.zoomPercent);
    appSettings.setValue("binary_point_cloud", settings.binaryPointCloud);
    appSettings.setValue("view_3d", settings.view3D);
    appSettings.setValue("adaptive_color_scale", settings.adaptiveColorScale);
    appSettings.setValue("convert_12_bit", settings.convert12Bit);
    appSettings.setValue("file_name_date_time", settings.fileNameDateTime);
}

bool MainWindow::init(QApplication& app) {
    if(!parseOptions(app)) {
        return false;
    }

#ifdef _WIN32
    if(!settings.nonGraphical && !keepConsole) {
        FreeConsole();
    }
#endif

    if(!settings.nonGraphical) {
        ui.reset(new Ui::MainWindow);
        ui->setupUi(this);
    }

    QObject::connect(this, &MainWindow::asyncDisplayException, this, [this](const QString& msg){
        displayException(msg.toStdString());
        nvcom->terminate();
        emit enableButtons(true, false);
    });

    QObject::connect(this, &MainWindow::updateStatusBar, this, [this](int dropped, int imgWidth, int imgHeight,
            std::vector<int> bits, int numPoints){
        char str[30];
        std::stringstream ss;
        if(imgWidth > 0 && imgHeight > 0) {
        ss << imgWidth << " x " << imgHeight << " pixels; ";
        }

        for(unsigned int i=0; i<bits.size(); i++) {
            if(bits[i] > 0) {
                if(i > 0) {
                    ss << "/";
                }
                ss << bits[i];
            }
        }
        if (bits.size() > 0 && bits[0] > 0) ss << " bits";

        if(numPoints > 0) {
            ss << " points: " << numPoints;
        }

        sizeLabel->setText(ss.str().c_str());
        snprintf(str, sizeof(str), "Dropped frames: %d", dropped);
        droppedLabel->setText(str);
    });

    QObject::connect(this, &MainWindow::repaintDisplayWidget, this, [this](){
        unique_lock<mutex> lock(displayMutex);
        if(displayWidget != nullptr) {
            displayWidget->repaint();
        }
    });

    QObject::connect(this, &MainWindow::updateFpsLabel, this, [this](const QString& text){
        fpsLabel->setText(text);
    });

    // Initialize window
    if(!settings.nonGraphical) {
        initGui();
        if(fullscreen) {
            makeFullscreen();
        } else {
            // Restore window position
            restoreGeometry(appSettings.value("geometry").toByteArray());
            restoreState(appSettings.value("state").toByteArray(), UI_VERSION);
        }
        show();
    }

    // FPS display
    fpsTimer.start();
    QObject::connect(&fpsTimer, &QTimer::timeout, this, &MainWindow::displayFrameRate);

    if(settings.remoteHost == "") {
        showConnectionDialog();
    } else {
        reinitNVCom();
    }

    if(writeImages) {
        // Automatically start sequence grabbing if a write directory is provided
        if(nvcom != nullptr) {
            nvcom->setCaptureSequence(true);
        }
        if(!settings.nonGraphical) {
            ui->actionCapture_sequence->setChecked(true);
        }
    }

#ifdef DEMO_MODE
    connect(&demoTimer, &QTimer::timeout, this, [this]{switchView(!settings.view3D);});
    demoTimer.start(1000*DEMO_CYCLE_INTERVAL);

    QPalette pal;
    pal.setColor(QPalette::Background, QColor(64, 64, 64));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    this->setPalette(pal);
#endif

    return true;
}

void MainWindow::reinitNVCom() {
    emit enableButtons(false, false);

    // Make sure all sockets are closed before creating a new object
    if(nvcom != nullptr) {
        nvcom->terminate();
        nvcom.reset();
    }

    nvcom.reset(new NVCom(settings));
    nvcom->setFrame2DDisplayCallback([this](int origW, int origH, const std::vector<cv::Mat_<cv::Vec3b>>& images, int numActiveImages, bool resize){
        display2DFrame(origW, origH, images, numActiveImages, resize);
    });

    nvcom->setFrame3DDisplayCallback([this](std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fov){
        display3DFrame(pointcloud, fov);
    });
    nvcom->setExceptionCallback([this](const std::exception& ex){
        emit asyncDisplayException(ex.what());
    });
    nvcom->setSendCompleteCallback([this]() {
        settings.readImages = false;
        if(closeAfterSend) {
            QApplication::quit();
        } else if(!settings.nonGraphical) {
            ui->actionSend_images->setChecked(false);
        }
    });
    nvcom->setConnectedCallback([this]() {
        if(!settings.nonGraphical) {
            emit enableButtons(true, true);
        }
    });
    nvcom->setDisconnectCallback([this]() {
        emit ui->actionConnect->triggered();
    });

    // Re-initialize the 2D or 3D view
    switchView(settings.view3D);

    nvcom->connect();
}

void MainWindow::initGui() {
    initToolBar();
    initStatusBar();

    // Zooming
    zoomLabel = new QLabel("100%", this);
    zoomLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->toolBar->insertWidget(ui->actionZoom_out, zoomLabel);
    QObject::connect(ui->actionZoom_in, &QAction::triggered,
        this, [this]{zoom(1);});
    QObject::connect(ui->actionZoom_out, &QAction::triggered,
        this, [this]{zoom(-1);});

    // Button enable / disable
    QObject::connect(this, &MainWindow::enableButtons, this, [this](bool nvcomReady, bool connected){
        if(fullscreen) {
            return; // No toolbar visible
        }

        ui->actionConnect->setEnabled(nvcomReady);
        ui->actionSend_images->setEnabled(connected);
        ui->actionCapture_single_frame->setEnabled(connected);
        ui->actionCapture_sequence->setEnabled(connected);
        ui->actionZoom_in->setEnabled(connected);
        ui->actionZoom_out->setEnabled(connected);
        ui->actionDisplayCoord->setEnabled(connected);
        ui->action2D_View->setEnabled(connected);
        ui->action3D_View->setEnabled(connected);
        zoomLabel->setEnabled(connected);
    });

    ui->actionDisplayCoord->setChecked(settings.displayCoordinate);
    ui->actionSend_images->setChecked(settings.readImages);
}

void MainWindow::makeFullscreen() {
    ui->menuBar->hide();
    delete ui->toolBar;
    ui->toolBar = nullptr;
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    showFullScreen();
}

void MainWindow::initStatusBar() {
    // Setup status bar
    droppedLabel = new QLabel(this);
    sizeLabel = new QLabel(this);
    fpsLabel = new QLabel(this);
    ui->statusBar->addPermanentWidget(droppedLabel);
    ui->statusBar->addPermanentWidget(sizeLabel);
    ui->statusBar->addPermanentWidget(fpsLabel);
    sizeLabel->setText("Waiting for data...");
}

void MainWindow::initToolBar() {
    QObject::connect(ui->actionDisplayCoord, &QAction::triggered, this, [this]{
        if(nvcom != nullptr) {
            settings.displayCoordinate = ui->actionDisplayCoord->isChecked();
                nvcom->updateSettings(settings);
            }
    });

    QObject::connect(ui->actionSettings, &QAction::triggered,
        this, [this]{openSettingsDialog();});

    QObject::connect(ui->actionCapture_single_frame, &QAction::triggered,
        this, [this]{
            if(SettingsDialog::chooseWriteDirectory(this, settings, true) && nvcom != nullptr) {
                nvcom->updateSettings(settings);
                nvcom->captureSingleFrame();
            }
        });

    QObject::connect(ui->actionCapture_sequence, &QAction::triggered,
        this, [this]{
            if(SettingsDialog::chooseWriteDirectory(this, settings, true) && nvcom != nullptr) {
                nvcom->updateSettings(settings);
                nvcom->setCaptureSequence(ui->actionCapture_sequence->isChecked());
            } else {
                ui->actionCapture_sequence->setChecked(false);
            }
        });

    QObject::connect(ui->actionQuit, &QAction::triggered,
        this, [this]{close();});

    QObject::connect(ui->actionConnect, &QAction::triggered,
        this, &MainWindow::showConnectionDialog);
    QObject::connect(ui->actionSend_images, &QAction::triggered,
        this, &MainWindow::transmitInputFolder);
    QObject::connect(ui->action3D_View, &QAction::triggered,
        this, [this]{switchView(true);});
    QObject::connect(ui->action2D_View, &QAction::triggered,
        this, [this]{switchView(false);});
}

void MainWindow::openSettingsDialog() {
    SettingsDialog diag(this, settings);
    diag.exec();
    settings = diag.getSettings();
    if(nvcom != nullptr) {
        nvcom->updateSettings(settings);
    }
}

void MainWindow::display2DFrame(int origW, int origH, const std::vector<cv::Mat_<cv::Vec3b>>& frames, int numActiveImages, bool resize) {
    fpsCounters.back().first++;

    unique_lock<mutex> lock(displayMutex, std::defer_lock);
    if(!lock.try_lock()) {
        return;
    }

    if(!settings.nonGraphical && nvcom != nullptr) {
        if(displayWidget != nullptr) {
            displayWidget->setDisplayFrame(frames, numActiveImages, resize);
        emit repaintDisplayWidget();
        }

        int dropped = nvcom->getNumDroppedFrames();

        if(resize || dropped != lastDropped) {
            lastDropped = dropped;
            std::vector<int> bitsArr = nvcom->getBitDepths();
            emit updateStatusBar(dropped, origW, origH, bitsArr, -1);
        }
    }
}

void MainWindow::display3DFrame(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz) {
#ifdef WITH_OPEN3D
    unique_lock<mutex> lock(displayMutex, std::defer_lock);
    if(!lock.try_lock()) {
        return;
    }

    if(open3dWidget != nullptr) {
        if(open3dWidget->setDisplayCloud(pointcloud, fovHoriz)) {
            fpsCounters.back().first++;
        }
    }

    int dropped = nvcom->getNumDroppedFrames();
    if(dropped != lastDropped || lastNumPoints != (int)pointcloud->points_.size()) {
        lastDropped = dropped;
        emit updateStatusBar(dropped, -1, -1, std::vector<int>(), pointcloud->points_.size());
        lastDropped = (int)pointcloud->points_.size();
    }
#endif
}

bool MainWindow::parseOptions(QApplication& app) {
    writeImages = false;
    fullscreen = false;
    keepConsole = false;
    settings.readImages = false;
    settings.readDir = appSettings.value("read_dir", "").toString().toStdString();
    settings.writeDir = appSettings.value("write_dir", "").toString().toStdString();
    settings.writeDirAlwaysAsk = appSettings.value("write_dir_always_ask", true).toBool();
    settings.maxFrameRate = appSettings.value("max_frame_rate", 30).toDouble();
    settings.nonGraphical = false;
    settings.remoteHost = "";
    settings.remotePort = 7681;
    settings.tcp = false;
    settings.writeLeft = appSettings.value("write_left", true).toBool();
    settings.writeRight = appSettings.value("write_right", true).toBool();
    settings.writeThirdColor = appSettings.value("write_third_color", true).toBool();
    settings.writeDisparityRaw = appSettings.value("write_disparity_raw", false).toBool();
    settings.writeDisparityColor = appSettings.value("write_disparity_color", true).toBool();
    settings.displayCoordinate = appSettings.value("display_coordinate", false).toBool();
    settings.disableReception = false;
    settings.printTimestamps = false;
    settings.writePointCloud = appSettings.value("write_point_cloud", false).toBool();
    settings.pointCloudMaxDist = appSettings.value("point_cloud_max_dist", 10).toDouble();
    settings.colorScheme = static_cast<Settings::ColorScheme>(appSettings.value("color_scheme", 2).toInt());
    settings.zoomPercent = appSettings.value("zoom", 100).toInt();
    settings.binaryPointCloud = appSettings.value("binary_point_cloud", false).toBool();
    settings.writePgm = appSettings.value("write_pgm", false).toBool();
    settings.view3D = appSettings.value("view_3d", false).toBool();
    settings.adaptiveColorScale = appSettings.value("adaptive_color_scale", true).toBool();
    settings.convert12Bit = appSettings.value("convert_12_bit", true).toBool();
    settings.fileNameDateTime = appSettings.value("file_name_date_time", true).toBool();
    settings.disparityOffset = 0.0;

    QCommandLineOption optColCoding ("c",  "Select color coding scheme (0 = no color, 1 = red / blue, 2 = rainbow)",
        "VAL", QString::number((int)settings.colorScheme));
    QCommandLineOption optMaxFps    ("f",  "Limit send frame rate to FPS", "FPS", QString::number(settings.maxFrameRate));
    QCommandLineOption optWriteDir  ("w",  "Immediately write all images to DIR", "DIR", QString::fromStdString(settings.writeDir));
    QCommandLineOption optReadDir   ("s",  "Send the images from the given directory", "DIR", QString::fromStdString(settings.readDir));
    QCommandLineOption optNonGraph  ("n",  "Non-graphical");
    QCommandLineOption optRemotePort("p",  "Use the given remote port number for communication", "PORT", QString::number(settings.remotePort));
    QCommandLineOption optRemoteHost("H",  "Use the given remote hostname for communication", "HOST", QString::fromStdString(settings.remoteHost));
    QCommandLineOption optTcp       ("t",  "Activate / deactivate TCP transfers", "on/off", (settings.tcp ? "on" : "off"));
    QCommandLineOption optNoRecept  ("d",  "Disable image reception");
    QCommandLineOption optTimeStamps("T",  "Print frame timestamps");
    QCommandLineOption optWrite3D   ("3",  "Write a 3D point cloud with distances up to VAL (0 = off)", "VAL", QString::number(settings.pointCloudMaxDist));
    QCommandLineOption optZoom      ("z",  "Set zoom factor to VAL percent", "VAL", QString::number(settings.zoomPercent));
    QCommandLineOption optFullscreen("F",  "Run in fullscreen mode");
    QCommandLineOption optBinCloud  ("b",  "Write point clouds in binary rather than text format", "on/off", (settings.binaryPointCloud ? "on" : "off"));
    QCommandLineOption optDispOffset("o",  "Apply constant offset to disparity values", "VAL", QString::number(settings.disparityOffset));
#ifdef _WIN32
    QCommandLineOption optConsole   ("C",  "Keep showing console on Windows");
#endif

    QCommandLineParser cmdParser;
    cmdParser.addOption(optColCoding);
    cmdParser.addOption(optMaxFps);
    cmdParser.addOption(optWriteDir);
    cmdParser.addOption(optReadDir);
    cmdParser.addOption(optNonGraph);
    cmdParser.addOption(optRemotePort);
    cmdParser.addOption(optRemoteHost);
    cmdParser.addOption(optTcp);
    cmdParser.addOption(optNoRecept);
    cmdParser.addOption(optTimeStamps);
    cmdParser.addOption(optWrite3D);
    cmdParser.addOption(optZoom);
    cmdParser.addOption(optFullscreen);
    cmdParser.addOption(optBinCloud);
    cmdParser.addOption(optDispOffset);
#ifdef _WIN32
    cmdParser.addOption(optConsole);
#endif
    cmdParser.addHelpOption();
    cmdParser.process(app);

    settings.colorScheme = static_cast<Settings::ColorScheme>(cmdParser.value(optColCoding).toInt());
    settings.maxFrameRate = cmdParser.value(optColCoding).toDouble();
    if(cmdParser.isSet(optWriteDir)) {
        settings.writeDir = cmdParser.value(optWriteDir).toLocal8Bit().toStdString();
                writeImages = true;
                writeDirSelected = true;
    }
    if(cmdParser.isSet(optReadDir)) {
        settings.readDir = cmdParser.value(optReadDir).toLocal8Bit().toStdString();
                settings.readImages = true;
                closeAfterSend = true;
    }
    settings.nonGraphical = cmdParser.isSet(optNonGraph);
    settings.remotePort = cmdParser.value(optRemotePort).toInt();
    settings.remoteHost = cmdParser.value(optRemoteHost).toLocal8Bit().toStdString();
    settings.tcp = (cmdParser.value(optTcp) == "on");
    settings.disableReception = cmdParser.isSet(optNoRecept);
    settings.printTimestamps = cmdParser.isSet(optTimeStamps);
    if(cmdParser.isSet(optWrite3D)) {
        double dist = cmdParser.value(optWrite3D).toDouble();
        if(dist > 0) {
            settings.pointCloudMaxDist = dist;
            settings.writePointCloud = true;
        } else {
            settings.writePointCloud = false;
        }
    }
    settings.zoomPercent = cmdParser.value(optZoom).toInt();
    fullscreen = cmdParser.isSet(optFullscreen);
    settings.binaryPointCloud = (cmdParser.value(optBinCloud) == "on");
    settings.disparityOffset = cmdParser.value(optDispOffset).toDouble();
#ifdef _WIN32
    keepConsole = cmdParser.isSet(optConsole);
#endif

    return true;
}

void MainWindow::showConnectionDialog() {
    ConnectionDialog diag(this);
    if(diag.exec() == QDialog::Accepted && diag.getSelectedDevice().getIpAddress() != "") {
        settings.remoteHost = diag.getSelectedDevice().getIpAddress();
        settings.tcp = (diag.getSelectedDevice().getNetworkProtocol() == DeviceInfo::PROTOCOL_TCP);
        reinitNVCom();
    }
}

void MainWindow::displayException(const std::string& msg) {
    cerr << "Exception occurred: " << msg << endl;
    QMessageBox msgBox(QMessageBox::Critical, "NVCom Exception!", msg.c_str());
    msgBox.exec();
}

void MainWindow::displayFrameRate() {
    // Compute frame rate
    int framesCount = 0;
    for(auto counter: fpsCounters) {
        framesCount += counter.first;
    }

    int elapsedTime = duration_cast<microseconds>(
        steady_clock::now() - fpsCounters[0].second).count();
    double fps = framesCount / (elapsedTime*1.0e-6);

    // Drop one counter
    fpsCounters.pop_front();
    fpsCounters.push_back(std::make_pair(0, steady_clock::now()));

    // Update label
    char fpsStr[6];
    snprintf(fpsStr, sizeof(fpsStr), "%.2lf", fps);

    if(fpsLabel != nullptr) {
        emit updateFpsLabel(QString(fpsStr) +  " fps");
    }

    // Print console status messages at a lower update rate
    static int printCounter = 0;
    if((printCounter++)>5 && nvcom != nullptr) {
        cout << "Fps: " << fpsStr << "; output queue: " << nvcom->getWriteQueueSize() << endl;
        printCounter = 0;
    }
}

void MainWindow::transmitInputFolder() {
    if(!settings.readImages) {
        QString newReadDir= QFileDialog::getExistingDirectory(this,
            "Choose input directory", settings.readDir.c_str(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(newReadDir != "") {
#ifdef _WIN32
            settings.readDir = newReadDir.toLocal8Bit().constData();
#else
            settings.readDir = newReadDir.toUtf8().constData();
#endif
            settings.readImages = true;
        }
    } else {
        settings.readImages = false;
    }

    if(settings.readImages) {
        bool ok = false;
        settings.maxFrameRate = QInputDialog::getDouble(this, "Choose frame rate",
            "Send frame rate:", settings.maxFrameRate, 0.1, 100, 1, &ok);
        if(!ok) {
            settings.readImages = false;
        }
    }

    ui->actionSend_images->setChecked(settings.readImages);
    if(nvcom != nullptr) {
        nvcom->updateSettings(settings);
    }
}

void MainWindow::zoom(int direction) {
    if(direction != 0) {
        settings.zoomPercent =
            (settings.zoomPercent / 25)*25 + 25*direction;
    }

    if(settings.zoomPercent > 400) {
        settings.zoomPercent = 400;
    } else if(settings.zoomPercent < 25) {
        settings.zoomPercent = 25;
    }

    {
        unique_lock<mutex> lock(displayMutex);
        if(displayWidget != nullptr) {
            displayWidget->setZoom(settings.zoomPercent);
        }
    }

    if(fullscreen) {
        // No widgets to update
        return;
    }

    char labelText[5];
    snprintf(labelText, sizeof(labelText), "%3d%%", settings.zoomPercent);
    zoomLabel->setText(labelText);

    ui->actionZoom_in->setEnabled(settings.zoomPercent != 400);
    ui->actionZoom_out->setEnabled(settings.zoomPercent != 25);
}

void MainWindow::switchView(bool view3D) {
    if(settings.nonGraphical) {
        return; // nothing to do
    }

    ui->action3D_View->setChecked(view3D);
    ui->action2D_View->setChecked(!view3D);

    {
        unique_lock<mutex> lock(displayMutex);

        if(displayWidget != nullptr) {
            ui->clientWidget->layout()->removeWidget(scrollArea);
            scrollArea->setWidget(nullptr);

            delete displayWidget;
            displayWidget = nullptr;

            delete scrollArea;
            scrollArea = nullptr;
        }

        if(open3dWidget != nullptr) {
            ui->clientWidget->layout()->removeWidget(open3dWidget);
            delete open3dWidget;
            open3dWidget = nullptr;
        }

        if(!view3D) {
            // Create 2D view
            scrollArea = new QScrollArea(this);
            scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->clientWidget->layout()->addWidget(scrollArea);

            displayWidget = new DisplayWidget(this);
            displayWidget->setNVCom(nvcom);
            scrollArea->setWidget(displayWidget);
        } else {
            // Create 3D View
            open3dWidget = new QtOpen3DVisualizer(this);
            open3dWidget->setErrorCallback([this]{
                settings.view3D = false;});
            ui->clientWidget->layout()->addWidget(open3dWidget);
        }
    }

    settings.view3D = view3D;
    if(nvcom != nullptr) {
        nvcom->updateSettings(settings);
    }

    if(!fullscreen) {
        if(!view3D) {
            zoomLabel->setEnabled(true);
        } else {
            ui->actionZoom_in->setEnabled(false);
            ui->actionZoom_out->setEnabled(false);
        }
    }

    zoom(0);
}

int main(int argc, char** argv) {
    bool graphical = true;

    try {
        QApplication app(argc, argv);

        // Set the default OpenGL surface format before any
        // window is created.
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSamples(4);
        format.setSwapInterval(1);
        QSurfaceFormat::setDefaultFormat(format);

        MainWindow win(nullptr, app);

        if(!win.init(app)) {
            return 1;
        }

        graphical = win.isGraphical();

        return app.exec();
    } catch(const std::exception& ex) {
        if(graphical) {
            QApplication app(argc, argv);
            MainWindow::displayException(ex.what());
        }
        return -1;
    }
}
