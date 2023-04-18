#include "mainwindow.h"
#include "ui_mainwindow.h"

#define BYTE    8
#define WAIT_MS 16

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    widg_rndr = new Screen_Cast_Rect(nullptr);
    serial = new QSerialPort(this);
    serial_available = open_serial_port();
    tmr = new QTimer();
    connect(ui->chckBx_tst_scale_img, &QAbstractButton::toggled, this, [=](bool tggld) {
        ui->lbl_tst_aspct_ratio->setEnabled(tggld);
        ui->cmbBx_tst_aspct_ratio->setEnabled(tggld);
        ui->lbl_tst_trnsfrmtn_mod->setEnabled(tggld);
        ui->cmbBx_tst_trnsfrmtn_mod->setEnabled(tggld);
    });
    connect(tmr, &QTimer::timeout, this, [=]() {
        tmr->stop();
        grab_image();
        tmr->setInterval(WAIT_MS);
        tmr->start();
    });
}

MainWindow::~MainWindow() {
    tmr->stop();
    delete tmr;
    delete serial;
    if(widg_rndr) {
        delete widg_rndr;
    }
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *) {
    widg_rndr->hide();
    delete widg_rndr;
}

void MainWindow::on_pshBttn_tst_show_clicked() {
    widg_rndr->show();
    change_ui();
}

void MainWindow::on_pshBttn_tst_hide_clicked() {
    widg_rndr->hide();
    change_ui();
}

void MainWindow::on_pshBttn_tst_grab_clicked() {
    grab_image();
}

void MainWindow::on_pshBttn_tst_strt_clicked() {
    tmr->setInterval(WAIT_MS);
    tmr->start();
    grab_started = true;
    change_ui();
}

void MainWindow::on_pshBttn_tst_stop_clicked() {
    tmr->stop();
    grab_started = false;
    change_ui();
}

bool MainWindow::open_serial_port() {
    serial->setPortName("/dev/ttyACM0");
//    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setBaudRate(921600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    if(!serial->open(QIODevice::WriteOnly)) {
        qDebug() << "serial port not opened";
        return false;
    }
    return true;
}

void MainWindow::change_ui() {
    ui->pshBttn_tst_show->setEnabled(!widg_rndr->isVisible());
    ui->pshBttn_tst_hide->setEnabled(widg_rndr->isVisible() && !grab_started);
    ui->pshBttn_tst_strt->setEnabled(widg_rndr->isVisible() && !grab_started);
    ui->pshBttn_tst_grab->setEnabled(widg_rndr->isVisible() && !grab_started);
    ui->pshBttn_tst_stop->setEnabled(widg_rndr->isVisible() && grab_started);
}

void MainWindow::grab_image() {
    QPixmap pxmp;
    QScreen *screen = QGuiApplication::primaryScreen();
    uint8_t brdr_wdth = widg_rndr->get_border_width();
    pxmp = screen->grabWindow(0, (widg_rndr->x() + brdr_wdth), (widg_rndr->y() + brdr_wdth), (widg_rndr->width() - (brdr_wdth * 2)), (widg_rndr->height() - (brdr_wdth * 2)));
    if((pxmp.width() == (widg_rndr->width() - (brdr_wdth * 2))) && (pxmp.height() == (widg_rndr->height() - (brdr_wdth * 2)))) {
        QByteArray tmp;
        uint8_t tmp_byte;
        QBitmap bm(pxmp);
        if(ui->chckBx_tst_scale_img->isChecked()) {
            bm = bm.scaled(ui->lbl_tst_screen->size(), static_cast<Qt::AspectRatioMode>(ui->cmbBx_tst_aspct_ratio->currentIndex()),
                                                       static_cast<Qt::TransformationMode>(ui->cmbBx_tst_trnsfrmtn_mod->currentIndex()));
        }
        ui->lbl_tst_screen->setPixmap(bm);
        QImage img = bm.toImage();
        for(uint16_t cur_wdth_pix = 0; cur_wdth_pix < img.width(); cur_wdth_pix++) {
            for(uint16_t cur_hght_pix = 0; cur_hght_pix < (img.height() / BYTE); cur_hght_pix++) {
                tmp_byte = 0;
                for(uint8_t i = 0; i < BYTE; i++) {
                    tmp_byte += static_cast<bool>(img.pixelColor(cur_wdth_pix, ((BYTE * cur_hght_pix) + i)).value()) << i;
                }
                tmp.append(tmp_byte);
            }
        }
        if(serial_available) {
            serial->write(tmp);
        }
    } else {
        qDebug() << "grab failed";
    }
}

//////////////////////////////////////Screen_Cast_Rect/////////////////////////////////////////////////////////////
Screen_Cast_Rect::Screen_Cast_Rect(QWidget *parent) : QWidget(parent) {
    this->setWindowFlags(this->windowFlags() | Qt::Tool | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->set_border_width(border_width);
}

Screen_Cast_Rect::~Screen_Cast_Rect() {}

void Screen_Cast_Rect::paintEvent(QPaintEvent *) {
    this->draw_border();
}

void Screen_Cast_Rect::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        clck_pos = event->pos();
        is_drag = true;
    }
}

void Screen_Cast_Rect::mouseMoveEvent(QMouseEvent *event) {
    if(is_drag) {
        move((event->globalX() - clck_pos.x()), (event->globalY() - clck_pos.y()));
    }
}

void Screen_Cast_Rect::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        is_drag = false;
    }
}

uint8_t Screen_Cast_Rect::get_border_width() {
    return border_width;
}

void Screen_Cast_Rect::set_border_width(uint8_t _border_width) {
    this->border_width = _border_width;
    this->setFixedSize((base_width + (border_width * 2)), (base_height + (border_width * 2)));
    QVector<QPoint> border_points;
    border_points.push_back(QPoint(0, 0));
    border_points.push_back(QPoint(this->width(), 0));
    border_points.push_back(QPoint(this->width(), this->height()));
    border_points.push_back(QPoint(0, this->height()));
    border_points.push_back(QPoint(0, 0));
    border_points.push_back(QPoint(border_width, border_width));
    border_points.push_back(QPoint((this->width() - border_width), border_width));
    border_points.push_back(QPoint((this->width() - border_width), (this->height() - border_width)));
    border_points.push_back(QPoint(border_width, (this->height() - border_width)));
    border_points.push_back(QPoint(border_width, border_width));
    border = QPolygon(border_points);
    this->draw_border();
}

void Screen_Cast_Rect::draw_border() {
    if(this->isVisible()) {
        QPainter painter(this);
        painter.setPen(QPen(QColor(100, 100, 100, 255), Qt::SolidLine));
        painter.setBrush(QBrush(QColor(100, 100, 100, 255)));
        painter.drawPolygon(border);
    }
}
