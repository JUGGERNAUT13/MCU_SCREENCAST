#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    widg_rndr = new Screen_Cast_Rect(nullptr, ui->chckBx_tst_scale_img->isChecked());
#ifdef __linux__
    ui->lnEdt_tst_port_name->setPlaceholderText("/dev/ttyACMx");
#else
    ui->lnEdt_tst_port_name->setPlaceholderText("COMx");
#endif
    serial = new QSerialPort(this);
    std::function<void(bool swtch_srl)> switch_serial = [=](bool swtch_srl) {
        serial_available = swtch_srl;
        ui->frm_tst_sttngs_ln_1->setEnabled(!serial_available);
        ui->frm_tst_sttngs_ln_2->setEnabled(!serial_available);
        ui->frm_tst_sttngs_ln_3->setEnabled(!serial_available);
        ui->pshBttn_tst_open_port->setEnabled(!serial_available);
        ui->pshBttn_tst_close_port->setEnabled(serial_available);
    };
    connect(ui->pshBttn_tst_open_port, &QAbstractButton::clicked, this, [=]() {
        switch_serial(open_serial_port());
    });
    connect(ui->pshBttn_tst_close_port, &QAbstractButton::clicked, this, [=]() {
        serial->close();
        switch_serial(false);
    });
    connect(ui->chckBx_tst_msr_time, &QAbstractButton::toggled, this, [=]() {
        ui->lbl_tst_msr_time->clear();
    });
    connect(ui->chckBx_tst_send_on_rqst, &QAbstractButton::toggled, this, [=](bool tggld) {
        ui->lbl_send_timeout->setEnabled(!tggld);
        ui->spnBx_tst_send_timeout->setEnabled(!tggld);
        ui->chckBx_tst_msr_time->setEnabled(tggld);
        ui->lbl_tst_msr_time->setEnabled(tggld);
        ui->lbl_tst_msr_time->clear();
        if(!frst_time_strt) {
            tmr->stop();
            on_pshBttn_tst_strt_clicked();
        }
    });
    emit ui->chckBx_tst_send_on_rqst->toggled(false);
    connect(ui->chckBx_tst_scale_img, &QAbstractButton::toggled, this, [=](bool tggld) {
        ui->lbl_tst_aspct_ratio->setEnabled(tggld);
        ui->cmbBx_tst_aspct_ratio->setEnabled(tggld);
        ui->lbl_tst_trnsfrmtn_mod->setEnabled(tggld);
        ui->cmbBx_tst_trnsfrmtn_mod->setEnabled(tggld);
    });
    std::function<void(bool shw_sttngs)> switch_sttngs = [=](bool shw_sttngs) {
        ui->frm_tst_sttngs->setVisible(shw_sttngs);
        ui->pshBttn_tst_sttngs->setText(shw_sttngs ? "^" : "v");
    };
    connect(ui->pshBttn_tst_sttngs, &QAbstractButton::toggled, this, [=](bool tggld) {
        switch_sttngs(tggld);
    });
    switch_sttngs(ui->pshBttn_tst_sttngs->isChecked());
    connect(ui->chckBx_tst_scrncst_alwys_on_top, &QAbstractButton::toggled, this, [=](bool tggld) {
        if(widg_rndr) {
            widg_rndr->set_screencast_always_on_top(tggld);
            if(!ui->pshBttn_tst_show->isEnabled()) {
                widg_rndr->hide();
                widg_rndr->show();
            }
        }
    });
    tmr_elpsd_time = new QElapsedTimer();
    connect(serial, &QSerialPort::readyRead, this, [=]() {
        if(ui->chckBx_tst_send_on_rqst->isChecked() && grab_started) {
            if(ui->chckBx_tst_msr_time->isChecked()) {
                ui->lbl_tst_msr_time->setText(QString::number(tmr_elpsd_time->restart()) + " msec");
//                qDebug() << "disp:" << serial->readAll();
            }
            grab_image();
        }
    });
    tmr = new QTimer();
    connect(tmr, &QTimer::timeout, this, [=]() {
        tmr->stop();
        grab_image();
        tmr->setInterval(ui->spnBx_tst_send_timeout->value());
        tmr->start();
    });
    this->setWindowTitle("MCU_SCREENCAST");
}

MainWindow::~MainWindow() {
    remove_screen_cast_rect();
    delete tmr_elpsd_time;
    delete serial;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *) {
    remove_screen_cast_rect();
}

void MainWindow::on_pshBttn_tst_show_clicked() {
    if(frst_time_strt) {
        widg_rndr->set_screencast_always_on_top(true);
        widg_rndr->show();
        widg_rndr->set_screencast_always_on_top(false);
        widg_rndr->show();
        widg_rndr->set_screencast_always_on_top(ui->chckBx_tst_scrncst_alwys_on_top->isChecked());
        frst_time_strt = false;
    }
    widg_rndr->show();
    change_ui();
    widg_rndr->move((this->x() + (this->width() / 2) - (widg_rndr->width() / 2)), (this->y() + (this->height() / 2) - (widg_rndr->height() / 2)));
}

void MainWindow::on_pshBttn_tst_hide_clicked() {
    widg_rndr->hide();
    change_ui();
}

void MainWindow::on_pshBttn_tst_grab_clicked() {
    grab_image();
}

void MainWindow::on_pshBttn_tst_strt_clicked() {
    if(!ui->chckBx_tst_send_on_rqst->isChecked()) {
        tmr->setInterval(ui->spnBx_tst_send_timeout->value());
        tmr->start();
    }
    grab_started = true;
    change_ui();
    if(ui->chckBx_tst_send_on_rqst->isChecked()) {
        grab_image();
        if(ui->chckBx_tst_msr_time->isChecked()) {
            tmr_elpsd_time->restart();
        }
    }
}

void MainWindow::on_pshBttn_tst_stop_clicked() {
    tmr->stop();
    grab_started = false;
    change_ui();
}

bool MainWindow::open_serial_port() {
    serial->setPortName(ui->lnEdt_tst_port_name->text());
    serial->setBaudRate(ui->cmbBx_tst_baud_rate->currentText().toInt());
    serial->setDataBits(static_cast<QSerialPort::DataBits>(ui->cmbBx_tst_data_bits->currentIndex() ? (ui->cmbBx_tst_data_bits->currentIndex() + 4) : (ui->cmbBx_tst_data_bits->currentIndex() - 1)));
    serial->setParity(static_cast<QSerialPort::Parity>(((ui->cmbBx_tst_parity->currentIndex() - 1) > 0) ? ui->cmbBx_tst_parity->currentIndex() : (ui->cmbBx_tst_parity->currentIndex() - 1)));
    serial->setStopBits(static_cast<QSerialPort::StopBits>(ui->cmbBx_tst_stop_bits->currentIndex() ? ui->cmbBx_tst_stop_bits->currentIndex() : (ui->cmbBx_tst_stop_bits->currentIndex() - 1)));
    serial->setFlowControl(static_cast<QSerialPort::FlowControl>(ui->cmbBx_tst_flw_cntrl->currentIndex() - 1));
    if(!serial->portName().count()) {
        show_message_box("", tr("Enter Serial Port Name"), QMessageBox::Warning, this);
        return false;
    }
//    qDebug() << serial->portName() << serial->baudRate() << serial->dataBits() << serial->parity() << serial->stopBits() << serial->flowControl();
    if(!serial->open(QIODevice::ReadWrite)) {
        show_message_box("", tr("Serial Port Not Opened"), QMessageBox::Warning, this);
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
    ui->chckBx_tst_scrncst_alwys_on_top->setEnabled(!grab_started);
}

void MainWindow::grab_image() {
    QScreen *screen = QGuiApplication::primaryScreen();
    uint16_t brdr_wdth = widg_rndr->get_border_width();
    QDesktopWidget *dw = QApplication::desktop();
    QPixmap pxmp = screen->grabWindow(dw->winId(), (widg_rndr->x() + brdr_wdth), (widg_rndr->y() + brdr_wdth), (widg_rndr->width() - (brdr_wdth * 2)), (widg_rndr->height() - (brdr_wdth * 2)));
    uint8_t tmp_byte;
    QBitmap bm(pxmp);
    QImage img = bm.toImage();
    QByteArray tmp;
    uint8_t mask, lastbit;
    if(ui->chckBx_fst_btmp->isChecked()) {
        tmp.fill(0x0, (img.width() * (img.height() / BYTE)));
        tmp_scr = (uint8_t *) malloc(img.width() * (img.height() / BYTE));
        memset(tmp_scr, 0x0, (img.width() * (img.height() / BYTE)));
    }
    for(uint16_t cur_hght_pix = 0; cur_hght_pix < (img.height() / BYTE); cur_hght_pix++) {
        for(uint16_t cur_wdth_pix = 0; cur_wdth_pix < img.width(); cur_wdth_pix++) {
            tmp_byte = 0;
            for(uint8_t i = 0; i < BYTE; i++) {
                tmp_byte += static_cast<bool>(img.pixelColor(cur_wdth_pix, ((BYTE * cur_hght_pix) + i)).value()) << i;
            }
            if(ui->chckBx_tst_invrs->isChecked()) {
                tmp_byte ^= 0xFF;
            }
            if(ui->chckBx_fst_btmp->isChecked()) {
                mask = 0x80 >> (cur_wdth_pix & 7);
                lastbit = img.height() - cur_hght_pix * BYTE;
                if(lastbit > BYTE) {
                    lastbit = BYTE;
                }
                for(uint8_t b = 0; b < lastbit; b++) {
                    if(tmp_byte & 1) {
                        *(tmp_scr + (cur_hght_pix * BYTE + b) * (img.width() / BYTE) + (cur_wdth_pix / BYTE)) |= mask;
                    }
                    tmp_byte >>= 1;
                }
            } else {
                tmp.append(tmp_byte);
            }
        }
    }
    if(ui->chckBx_fst_btmp->isChecked()) {
        memcpy(tmp.data(), tmp_scr, (img.width() * (img.height() / BYTE)));
        free(tmp_scr);
    }
    if(serial_available) {
        serial->write(tmp);
    }
    if(ui->chckBx_tst_scale_img->isChecked()) {
        bm = bm.scaled(ui->lbl_tst_screen->size(), static_cast<Qt::AspectRatioMode>(ui->cmbBx_tst_aspct_ratio->currentIndex()),
                       static_cast<Qt::TransformationMode>(ui->cmbBx_tst_trnsfrmtn_mod->currentIndex()));
    }
    ui->lbl_tst_screen->setPixmap(bm);
}

void MainWindow::remove_screen_cast_rect() {
    grab_started = false;
    serial_available = false;
    if(widg_rndr) {
        widg_rndr->hide();
        delete widg_rndr;
        widg_rndr = nullptr;
    }
}

//-------------------------------------------------------------------------
// DISPLAYING WARNING/QUESTION/INFO MESSAGES
//-------------------------------------------------------------------------
int MainWindow::show_message_box(QString title, QString message, QMessageBox::Icon type, QWidget *parent) {
    QMessageBox msgBox(type, title, QString("\n").append(message));
    if(!title.count()) {
        msgBox.setWindowTitle(tr("Warning"));
        if(type == QMessageBox::Question) {
            msgBox.setWindowTitle(tr("Question"));
        } else if(type == QMessageBox::Information) {
            msgBox.setWindowTitle(tr("Information"));
        }
    }
    msgBox.setParent(parent);
    if(type == QMessageBox::Question) {
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
    }
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setModal(true);
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QGridLayout *layout = dynamic_cast<QGridLayout *>(msgBox.layout());
    layout->setColumnMinimumWidth(1, 20);
    msgBox.setLayout(layout);
    return msgBox.exec();
}

//////////////////////////////////////Screen_Cast_Rect/////////////////////////////////////////////////////////////
Screen_Cast_Rect::Screen_Cast_Rect(QWidget *parent, bool scrncst_alwys_on_top) : QWidget(parent) {
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->dflt_wndw_flgs = this->windowFlags();
    this->set_screencast_always_on_top(scrncst_alwys_on_top);
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
    border = QPolygon({QPoint(-1, -1), QPoint(this->width(), -1), QPoint(this->width(), this->height()), QPoint(-1, this->height()), QPoint(-1, -1),
                       QPoint((border_width - 1), (border_width - 1)), QPoint((this->width() - border_width), (border_width - 1)),
                       QPoint((this->width() - border_width), (this->height() - border_width)), QPoint((border_width - 1), (this->height() - border_width)),
                       QPoint((border_width - 1), (border_width - 1))});
    this->draw_border();
}

void Screen_Cast_Rect::set_screencast_always_on_top(bool scrncst_alwys_on_top) {
    if(scrncst_alwys_on_top) {
#ifdef __linux__
        this->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
#else
        this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#endif
    } else {
        this->setWindowFlags(dflt_wndw_flgs | Qt::Tool | Qt::FramelessWindowHint);
    }
}

void Screen_Cast_Rect::draw_border() {
    if(this->isVisible()) {
        QPainter painter(this);
        painter.setPen(QPen(QColor(100, 100, 100, 255), Qt::SolidLine));
        painter.setBrush(QBrush(QColor(100, 100, 100, 255)));
        painter.drawPolygon(border);
    }
}
