#include "MainWindow.h"

#include <QTableView>
#include <QSpinBox>
#include <QStylePainter>
#include <QTreeView>
#include <QFile>
#include <QString>
#include <QPixmap>
#include <iostream>
#include <fstream>

#include <common/system.hpp>
#include <common/utility.hpp>

using namespace QtCharts;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{

    ui.setupUi(this);

    setWindowTitle(u8"相参测试");
    setWindowIcon(QIcon(":/MainWindow/icon.png"));
    setWindowFlags(Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);

    ui.label_10->move((ui.widget_2->width() - ui.label_10->width()) / 2 ,
        (ui.widget_2->height() - ui.label_10->height()) / 2);
 
    model = new QStandardItemModel(ui.cailTableView);
    ui.cailTableView->setModel(model);

    ui.cailTableView->setAlternatingRowColors(true);
    ui.cailTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.cailTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui.cailTableView->setRowHeight(0, 28);//设置固定高度

    ui.cailTableView->verticalHeader()->setVisible(false);

    ui.cailTableView->horizontalHeader()->setObjectName("hHeader");
    ui.cailTableView->verticalHeader()->setObjectName("vHeader");
    ui.cailTableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui.cailTableView->scrollToBottom();

    ui.cailTableView->horizontalHeader()->setStyleSheet("QHeaderView#hHeader::section {"
        "color: #FFFFFF; padding-left: 4px; border: 1px solid #6c6c6c; background-color: #38B09E}");
    ui.cailTableView->horizontalHeader()->setSectionsClickable(false);
    
    ui.cailTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.cailTableView->setSelectionMode(QAbstractItemView::SingleSelection);
   
    ui.cailButton->setStyleSheet("QPushButton{background-color:#29363A;"
        "border-width: 1px; border-color: rgb(59,176,158);"
        "color: white; border-style: inset;}"
        "QPushButton:hover{background-color:#29474A; color: #3CB09E;}"
        "QPushButton:pressed{background-color:#29474A);"
        "border-style: inset;}"
    );

    cail_impl_ = std::make_unique<cail_impl_t>([this](const auto& msg, auto type, auto level) {});

    connect(cail_impl_.get(), &cail_impl_t::sig_channelinfo, [this](std::uint32_t number, double iq_rate, double carrier_frequency) {

        model->setColumnCount(number);
        QStringList  horizontal_header_list;
        horizontal_header_list << u8"序号";
        for (auto i = 1u; i < number; i++)
            horizontal_header_list << u8"通道" + QString::number(i);
        model->setHorizontalHeaderLabels(horizontal_header_list);
        ui.cailTableView->resizeColumnToContents(0);
        if (number > 8)
            ui.cailTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        else {
            for (auto i = 1u; i < number; i++) {
                ui.cailTableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
            }
        }
        channel_num_ = number;
        this->set_ui(iq_rate, carrier_frequency);
        });

    connect(cail_impl_.get(), &cail_impl_t::sig_setlist, this, &MainWindow::slot_ui, Qt::QueuedConnection);
    connect(ui.cailButton, &QPushButton::clicked, [this](bool val) {
        if (channel_num_ <= 1) {
            return;
        }
        else {
            cail_impl_->recailbrate();
        }
 
        });

    connect(this, &MainWindow::refresh_result, this, &MainWindow::slot_refresh_result);
}

MainWindow::~MainWindow()
{
}

void MainWindow::set_ui(double iq_rate, double carrier_frequency)
{
    ui.label_6->setText(QString::fromStdString(intellicube::utility::hz_format(carrier_frequency, 2)));
    ui.label_8->setText(QString::fromStdString(intellicube::utility::hz_format(iq_rate / 1.25, 2)));
}

void MainWindow::slot_refresh_result(double percent)
{
   
    int remainAngle = (1.0 - percent / 100.0) * 360;
    int halfRemainAngle = remainAngle / 2;
    int startAngle = halfRemainAngle;
    int endAngle = startAngle + 360;
    if (percent >= 99)
    {
        ui.label_10->setStyleSheet("color: #3CB09E;");
        ui.label_3->setPixmap(QPixmap(":/MainWindow/yes.png"));
        ui.label_3->setScaledContents(true);
    }
    else
    {
        ui.label_10->setStyleSheet("color: #ff0000;");
        ui.label_3->setPixmap(QPixmap(":/MainWindow/no.png"));
        ui.label_3->setScaledContents(true);
    }

    ui.label_10->setText(QString("%1%").arg((int)percent));
}

void MainWindow::slot_ui(const std::vector<double>& phase_differ, double passing_rate) {
    static std::uint32_t times = 0;
    std::vector<double> phase;
    model->insertRow(model->rowCount());
    auto index_number = model->index(times, 0);
    auto item_number = model->itemFromIndex(index_number);
    item_number->setText(QString::number(times));
    item_number->setTextAlignment(Qt::AlignCenter);
    item_number->setForeground(QBrush(QColor("#FFFFFF")));
    for (auto i = 1; i < phase_differ.size() + 1; i++) {
        auto index_0 = model->index(times, i);
        auto item_0 = model->itemFromIndex(index_0);
        item_0->setText(u8"Δ" + QString::number(phase_differ[i - 1], 'f', 3) + u8"°");
        item_0->setTextAlignment(Qt::AlignCenter);
        item_0->setForeground(QBrush(QColor("#FFFFFF")));
        phase.emplace_back(phase_differ[i - 1]);
    }
    if (times)
        emit refresh_result(passing_rate);
    times++;
}









