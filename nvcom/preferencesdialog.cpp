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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include <iostream>

PreferencesDialog::PreferencesDialog(QWidget *parent, const Settings& settings) :
    QDialog(parent), ui(new Ui::PreferencesDialog), settings(settings)
{
    ui->setupUi(this);
    resize(width(), 1);

    ui->radioUdp->setChecked(!settings.tcp);
    ui->radioTcp->setChecked(settings.tcp);
    ui->localPortBox->setValue(settings.localPort);
    ui->remotePortBox->setValue(settings.remotePort);
    ui->localHostEdit->setText(settings.localHost.c_str());
    ui->remoteHostEdit->setText(settings.remoteHost.c_str());
    enableWidgets();

    QObject::connect(this, &QDialog::accepted, this, &PreferencesDialog::dialogAccepted);
    QObject::connect(ui->radioUdp, &QRadioButton::toggled, this,
        &PreferencesDialog::enableWidgets);
}

PreferencesDialog::~PreferencesDialog() {
    delete ui;
}

const Settings& PreferencesDialog::getSettings() {
    return settings;
}

void PreferencesDialog::dialogAccepted() {
    settings.tcp = ui->radioTcp->isChecked();
    settings.localPort = ui->localPortBox->value();
    settings.remotePort = ui->remotePortBox->value();
    settings.localHost = ui->localHostEdit->text().toStdString();
    settings.remoteHost = ui->remoteHostEdit->text().toStdString();
}

void PreferencesDialog::enableWidgets() {
    bool udp = ui->radioUdp->isChecked();
    ui->localPortBox->setEnabled(udp);
    ui->localHostEdit->setEnabled(udp);
}
