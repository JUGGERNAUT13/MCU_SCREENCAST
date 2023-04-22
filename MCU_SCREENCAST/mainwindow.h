#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #define BYTE            8
    #define MEASURE_TIME    0
//    #define WAIT_MS         60

    #include <QDesktopWidget>
    #include <QApplication>
    #include <QMouseEvent>
    #include <QMainWindow>
    #include <QSerialPort>
    #include <QWidget>
    #include <QDebug>
    #include <QScreen>
    #include <QBitmap>
    #include <QPainter>
#if MEASURE_TIME
    #include <QElapsedTimer>
#endif
//    #include <QTimer>

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

            Ui::MainWindow *ui;
            Screen_Cast_Rect *widg_rndr = nullptr;
            QSerialPort *serial = nullptr;
#if MEASURE_TIME
            QElapsedTimer *tmr = nullptr;
#endif
//            QTimer *tst_tmr = nullptr;

            bool serial_available = false;
            bool grab_started = false;

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
            Screen_Cast_Rect(QWidget *parent = nullptr);
            ~Screen_Cast_Rect();

            uint8_t get_border_width();
            void set_border_width(uint8_t _border_width);

        private:
            void paintEvent(QPaintEvent *) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void draw_border();

            QPolygon border;
            QPoint clck_pos;

            uint8_t base_width = 128;
            uint8_t base_height = 64;
            uint8_t border_width = 5;

            bool is_drag = false;
    };

#endif // MAINWINDOW_H
