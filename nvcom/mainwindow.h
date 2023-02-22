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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <chrono>
#include <deque>
#include <mutex>

#include <QMainWindow>
#include <QSettings>
#include <QTimer>

#include "settings.h"

// Predeclarations
namespace Ui {
class MainWindow;
}
namespace open3d {
    namespace geometry {
        class PointCloud;
    }
}
class QLabel;
class QComboBox;
class QScrollArea;
class NVCom;
class DisplayWidget;
class QtOpen3DVisualizer;

/*
 * The main application window, displaying the received frames.
 */
class MainWindow: public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent, QApplication& app);
    ~MainWindow();

    bool init(QApplication& app);
    bool isGraphical() const {return !settings.nonGraphical;}
    static void displayException(const std::string& msg);

signals:
    void asyncDisplayException(const QString& msg);
    void updateStatusBar(int dropped, int imgWidth, int imgHeight, std::vector<int> bits,
        int numPoints);
    void updateFpsLabel(const QString& text);
    void enableButtons(bool nvcomReady, bool connected);
    void repaintDisplayWidget();

private:
    static constexpr int UI_VERSION = 1;

    std::unique_ptr<Ui::MainWindow> ui;
    Settings settings;
    std::shared_ptr<NVCom> nvcom;
    bool writeImages;
    bool writeDirSelected;
    bool fullscreen;
    bool keepConsole;

    QLabel* fpsLabel;
    QLabel* sizeLabel;
    QLabel* droppedLabel;
    QLabel* zoomLabel;
    QSettings appSettings;
    QTimer fpsTimer;
    QTimer demoTimer;
    //QComboBox* colorCombo;
    QScrollArea* scrollArea;
    DisplayWidget* displayWidget;
    QtOpen3DVisualizer* open3dWidget;

    bool closeAfterSend;
    bool resizeWindow;
    int lastDropped;
    int lastNumPoints;

    std::deque<std::pair<unsigned int, std::chrono::steady_clock::time_point> > fpsCounters;

    std::mutex displayMutex;

    bool parseOptions(QApplication& app);
    void display2DFrame(int origW, int origH, const std::vector<cv::Mat_<cv::Vec3b>>& frames, int numActiveImages, bool resize);
    void display3DFrame(std::shared_ptr<open3d::geometry::PointCloud> pointcloud, double fovHoriz);
    bool chooseWriteDirectory(bool forceChoice);

    void initGui();
    void initToolBar();
    void initStatusBar();

    void showConnectionDialog();
    void displayFrameRate();
    void reinitNVCom();
    void writeApplicationSettings();
    void transmitInputFolder();
    void zoom(int direction);
    void makeFullscreen();
    void openSettingsDialog();
    void switchView(bool view3D);
};

#endif
