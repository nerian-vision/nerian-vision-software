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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QFileDialog>
#include <QScrollBar>
#include <QTimer>
#include <iostream>

SettingsDialog::SettingsDialog(QWidget *parent, const Settings& settings) :
    QDialog(parent), ui(new Ui::SettingsDialog), settingsNew(settings), settingsOrig(settings)
{
    ui->setupUi(this);
    resize(width(), 1);

    ui->colorScheme->setCurrentIndex(settings.colorScheme);
    ui->adaptiveScale->setChecked(settings.adaptiveColorScale);
    ui->captureDir->setText(settings.writeDir.c_str());
    ui->alwaysAskCaptureDir->setChecked(settings.writeDirAlwaysAsk);

    ui->writeLeft->setChecked(settings.writeLeft);
    ui->writeRight->setChecked(settings.writeRight);
    ui->writeColor->setChecked(settings.writeThirdColor);
    ui->writeDisparityColor->setChecked(settings.writeDisparityColor);
    ui->writeDisparityRaw->setChecked(settings.writeDisparityRaw);
    ui->writePointCloud->setChecked(settings.writePointCloud);

    ui->fileFormat->setCurrentIndex(settings.writePgm ? 1 : 0);
    ui->convert12Bit->setChecked(settings.convert12Bit);

    ui->maxDist->setValue(settings.pointCloudMaxDist);
    ui->formatBinary->setChecked(settings.binaryPointCloud);
    ui->formatText->setChecked(!settings.binaryPointCloud);

    ui->fileNameDateTime->setChecked(settings.fileNameDateTime);
    ui->fileNameSequential->setChecked(!settings.fileNameDateTime);

    QObject::connect(ui->changeDir, &QPushButton::clicked,
        this, [this]{
            chooseWriteDirectory(this, this->settingsNew, false);
            ui->captureDir->setText(this->settingsNew.writeDir.c_str());
        });

    QObject::connect(this, &QDialog::accepted, this, &SettingsDialog::dialogAccepted);

    QTimer::singleShot(0, this, &SettingsDialog::adjustSize);
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

const Settings& SettingsDialog::getSettings() {
    if(accepted) {
        return settingsNew;
    } else {
        return settingsOrig;
    }
}

void SettingsDialog::adjustSize() {
    const QSize maxSize(1000, 1000);
    const int step = 5;
    while (ui->scrollArea->verticalScrollBar()->isVisible() && height() < maxSize.height())
        resize(width(), height() + step);
    while (ui->scrollArea->horizontalScrollBar()->isVisible() && width() < maxSize.width())
        resize(width()+step, height());

    auto hostRect = parentWidget()->geometry();
    move(hostRect.center() - rect().center());
}

void SettingsDialog::dialogAccepted() {
    accepted = true;

    settingsNew.colorScheme = (Settings::ColorScheme)ui->colorScheme->currentIndex();
    settingsNew.adaptiveColorScale = ui->adaptiveScale->isChecked();

    settingsNew.writeLeft = ui->writeLeft->isChecked();
    settingsNew.writeRight = ui->writeRight->isChecked();
    settingsNew.writeThirdColor = ui->writeColor->isChecked();
    settingsNew.writeDisparityColor = ui->writeDisparityColor->isChecked();
    settingsNew.writeDisparityRaw = ui->writeDisparityRaw->isChecked();
    settingsNew.writePointCloud = ui->writePointCloud->isChecked();

    settingsNew.writePgm = (ui->fileFormat->currentIndex() == 1);
    settingsNew.convert12Bit = ui->convert12Bit->isChecked();

    settingsNew.pointCloudMaxDist = ui->maxDist->value();
    settingsNew.binaryPointCloud = ui->formatBinary->isChecked();

    settingsNew.writeDirAlwaysAsk = ui->alwaysAskCaptureDir->isChecked();
    settingsNew.fileNameDateTime = ui->fileNameDateTime->isChecked();
}

bool SettingsDialog::chooseWriteDirectory(QWidget *parent, Settings& settings, bool allowSkip) {
    if(allowSkip && (settings.writeDirSelected || !settings.writeDirAlwaysAsk)) {
        return true;
    }

    QString newDir = QFileDialog::getExistingDirectory(parent,
        "Choose capture directory", settings.writeDir.c_str(),
         QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if(newDir != "") {
#ifdef _WIN32
        settings.writeDir = newDir.toLocal8Bit().constData();
#else
        settings.writeDir = newDir.toUtf8().constData();
#endif
        settings.writeDirSelected = true;

        return true;
    } else {
        return false;
    }
}
