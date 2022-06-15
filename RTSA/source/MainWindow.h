#pragma once

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>
#include <QTableView>
#include <QMouseEvent>
#include <QtCharts/qchartview.h>
QT_CHARTS_USE_NAMESPACE

#include "ui_MainWindow.h"
#include "cail_impl.hpp"


class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindowClass ui;
    QStandardItemModel* model;

    std::unique_ptr<cail_impl_t> cail_impl_;

    double strat_freq_;
    double end_freq_;
    double bandwidth_;
    double stepping_;

    std::uint32_t times_ = 0;
    double percent_ = 0.0;

    std::uint32_t channel_num_;

public:
    MainWindow(QWidget *parent = Q_NULLPTR);
    ~MainWindow();

    void set_ui(double, double);

signals:
    void refresh_result(double);
    void add_cailebrate();
 

public slots:
    void slot_refresh_result(double);
    void slot_ui(const std::vector<double>&, double);
};
