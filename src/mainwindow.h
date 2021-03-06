#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <QtWidgets/QSystemTrayIcon>
#include "transparent_dialog/transparentdialog.h"
#include "floatImage_dialog/floatimagedialog.h"
#include <QWindow>
#include "config/Config.h"
#include <memory>
#include "config/ConfigView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void toTray();

private:
    Ui::MainWindow *ui;
    TransparentDialog *transparentDialog = nullptr;
    QMenu *trayIconMenu = nullptr;
    QSystemTrayIcon *trayIcon = nullptr;
    QAction *takePicAction = nullptr;
    QAction *showTestWindowAction = nullptr;
    QAction *exitAction = nullptr;
    QAction *openConfigDialogAction = nullptr;
    ConfigView *configView = nullptr;
    QVector<FloatImageDialog*> imagePanels;

    void createTrayIcon();
    void createActions();
    void createTransparentWindow();
    void showTransparentWindow();
    void showConfig();
    void removeFloatImage(FloatImageDialog*);
public slots:
    void takeScreenshot(CroppingInfo croppingInfo);


signals:
    void doneTakingImage(shared_ptr<QPixmap> image, CroppingInfo croppingInfo);
};
#endif // MAINWINDOW_H
