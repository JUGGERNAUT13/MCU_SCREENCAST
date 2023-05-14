#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #define BYTE                        8

    #include <QDesktopWidget>
    #include <QElapsedTimer>
    #include <QApplication>
    #include <QMouseEvent>
    #include <QMainWindow>
    #include <QSerialPort>
    #include <QMessageBox>
    #include <QWidget>
    #include <QDebug>
    #include <QScreen>
    #include <QBitmap>
    #include <QPainter>
    #include <QTimer>

    QT_BEGIN_NAMESPACE
    namespace Ui {
        class MainWindow;
    }
    QT_END_NAMESPACE

    class Screen_Cast_Rect;

    class MainWindow : public QMainWindow {
        Q_OBJECT
        public:
            MainWindow(QWidget *parent = nullptr);
            ~MainWindow();

        private:
            bool open_serial_port();
            void change_ui();
            void grab_image();
            void remove_screen_cast_rect();
            int show_message_box(QString title, QString message, QMessageBox::Icon type, QWidget *parent);

            Ui::MainWindow *ui;
            Screen_Cast_Rect *widg_rndr = nullptr;
            QSerialPort *serial = nullptr;
            QElapsedTimer *tmr_elpsd_time = nullptr;
            QTimer *tmr = nullptr;
            uint8_t *tmp_scr = nullptr;
            bool serial_available = false;
            bool grab_started = false;
            bool frst_time_strt = true;

        private slots:
            void closeEvent(QCloseEvent *) override;
            void on_pshBttn_tst_show_clicked();
            void on_pshBttn_tst_hide_clicked();
            void on_pshBttn_tst_grab_clicked();
            void on_pshBttn_tst_strt_clicked();
            void on_pshBttn_tst_stop_clicked();
    };

    //////////////////////////////////////Screen_Cast_Rect/////////////////////////////////////////////////////////////
    class Screen_Cast_Rect : public QWidget {
        Q_OBJECT
        public:
            Screen_Cast_Rect(QWidget *parent = nullptr, bool scrncst_alwys_on_top = false);
            ~Screen_Cast_Rect();

            uint8_t get_border_width();
            void set_border_width(uint8_t _border_width);
            void set_screencast_always_on_top(bool scrncst_alwys_on_top);

        private:
            void paintEvent(QPaintEvent *) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void draw_border();

            QPolygon border;
            QPoint clck_pos;
            Qt::WindowFlags dflt_wndw_flgs;

            uint8_t base_width = 128;
            uint8_t base_height = 64;
            uint8_t border_width = 5;

            bool is_drag = false;
    };

#endif // MAINWINDOW_H
