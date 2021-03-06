#include <QtGui/QMouseEvent>
#include "floatimagedialog.h"
#include "ui_floatimagedialog.h"
#include <QDebug>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QDateTime>
#include <QClipboard>
#include <memory>
#include <QFlags>
#include <QPoint>
#include <QIcon>
//#include "FloatImageState.h"

/**
 * TODO there is a bug when turn off then on resize option. The image are not resizable after turning such option back on.
 * @param croppingInfo
 * @param image
 * @param parent
 */
FloatImageDialog::FloatImageDialog(CroppingInfo croppingInfo, shared_ptr<QPixmap> image, QWidget *parent) :
        FloatImageDialog(make_shared<FloatImageState>(),croppingInfo,image,parent){
}


FloatImageDialog::FloatImageDialog(shared_ptr<FloatImageState> state,
                                   CroppingInfo croppingInfo,
                                   shared_ptr<QPixmap> image,
                                   QWidget *parent)
                                   :QDialog(parent),ui(new Ui::FloatImageDialog),state(state),originalCroppingInfo(croppingInfo),pixmap(image) {
    ui->setupUi(this);
    this->move(croppingInfo.position);
    this->alignChildViews(croppingInfo.size);
    this->ui->imageLabel->setPixmap(*this->pixmap);
    this->config(Config::getInstance(), croppingInfo);
    this->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    this->setWindowFlags(this->windowFlags()|Qt::Dialog);
    connect(this, &QWidget::customContextMenuRequested,
            this, &FloatImageDialog::showContextMenu);

    connect(Config::getInstance().get(),
            &Config::settingChangedSignal, this,
            [this, croppingInfo](std::shared_ptr<Config> config) {
                this->config(config, croppingInfo);
                this->resettt();
            });

    connect(this->ui->closeButton, &QToolButton::clicked, this, [this]() {
        this->accepted();
    });

    connect(this->ui->saveButton, &QToolButton::clicked, this, [this]() {
        this->saveImage();
    });

    connect(this->ui->copyButton, &QToolButton::clicked, this, [this]() {
        this->copyImageToClipboard();
    });

    connect(this->ui->resetResolutionButton, &QToolButton::clicked, this, [this]() {
        this->resetSize();
    });

    connect(this->ui->pinButton, &QToolButton::clicked,this,[this](){
        shared_ptr<FloatImageState> newState = make_shared<FloatImageState>(this->state->flipPin());
        this->loadState(newState);
    });

}

FloatImageDialog::~FloatImageDialog() {
    delete ui;
}

void FloatImageDialog::mousePressEvent(QMouseEvent *event) {
    qDebug() << tr("FloatImage: Mouse press");
    QWidget::mousePressEvent(event);
//    if (event->button() == Qt::MouseButton::RightButton) {
//        this->preventClose = true;
//    }
    if (event->button() == Qt::MouseButton::LeftButton) {
        this->isLeftMouseDown = true;
        this->xDistanceBetweenCornerAndMouse = abs(event->globalPosition().x() - this->x());
        this->yDistanceBetweenCornerAndMouse = abs(event->globalPosition().y() - this->y());
    }
}

/**
 * this handler is only triggered by the release of left mouse button
 * @param event
 */
void FloatImageDialog::mouseReleaseEvent(QMouseEvent *event) {
    qDebug() << "FloatImage: Mouse release: " << event->button();
    if (this->preventClose == false && Config::getInstance()->getClickToCloseFloatImgFlag()) {
        this->accepted();
    }
    this->preventClose = false;
    this->isLeftMouseDown = false;
}

void FloatImageDialog::mouseMoveEvent(QMouseEvent *event) {
    QWidget::mouseMoveEvent(event);
    //move the image around
    qDebug() << tr("FloatImage: Move mouse");
    qDebug() << event->button();
    if (this->isLeftMouseDown) {
        qDebug() << tr("FloatImage: Move mouse - left button");
        QPoint newXY = QPoint(event->globalPosition().x() - this->xDistanceBetweenCornerAndMouse,
                              event->globalPosition().y() - this->yDistanceBetweenCornerAndMouse);
        qDebug() << newXY;
        this->move(newXY);
        this->preventClose = true;
    }
}

void FloatImageDialog::closeEvent(QCloseEvent *event) {
    this->setParent(nullptr);
    QDialog::closeEvent(event);
    qDebug() << "close float image";
    Q_EMIT QDialog::accepted();
}

void FloatImageDialog::showContextMenu(const QPoint &menuPosition) {
    this->setupContextMenu();
    this->contextMenu->show();
    this->contextMenu->move(this->mapToGlobal(menuPosition));
}

void FloatImageDialog::setupContextMenu() {
    if (this->saveAction == nullptr) {
        this->saveAction = new QAction("Save", this);
        connect(this->saveAction, &QAction::triggered, this, [this]() {
            this->saveImage();
        });
    }

    if (this->copyToClipboardAction == nullptr) {
        this->copyToClipboardAction = new QAction("Copy to clipboard", this);
        connect(this->copyToClipboardAction, &QAction::triggered, this, [this]() {
            this->copyImageToClipboard();
        });
    }

    auto closeAction = new QAction("Close", this);
    connect(closeAction, &QAction::triggered, this, [this]() {
        this->accepted();
    });

    auto resetSizeAction = new QAction("Reset size");
    connect(resetSizeAction, &QAction::triggered, this, [this]() {
        this->resetSize();
    });

    if (this->contextMenu == nullptr) {
        this->contextMenu = new QMenu(this);
        this->contextMenu->addAction(this->saveAction);
        this->contextMenu->addAction(this->copyToClipboardAction);
        this->contextMenu->addAction(resetSizeAction);
        this->contextMenu->addAction(closeAction);
    }
}

void FloatImageDialog::config(shared_ptr<Config> config, CroppingInfo croppingInfo) {
    this->setWindowFlag(Qt::FramelessWindowHint, !config->getShowTitleBarOnFloatImage());
    this->setWindowFlag(Qt::WindowStaysOnTopHint, config->getFloatImageAlwaysOnTopFlag());

    if (config->getShowUiFloatImg()) {
        this->ui->buttonFrame->show();
    } else {
        this->ui->buttonFrame->hide();
    }
    qDebug()<< "Enable resize: "<<config->getEnableResizeFloatImg();
    this->setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, !config->getEnableResizeFloatImg());

    if (config->getEnableResizeFloatImg()) {
        this->resize(croppingInfo.size);
    } else {
        this->setFixedSize(croppingInfo.size);
    }
}

void FloatImageDialog::alignChildViews(QSize size) {
    // move image to top-left corner
    this->ui->imageLabel->move(0, 0);
    this->ui->imageLabel->resize(size);

    // move buttons to top-right corner
    int buttonXPosition = size.width() - this->ui->buttonFrame->width() - 5;
    if (buttonXPosition >= 0) {
        this->ui->buttonFrame->move(buttonXPosition, 5);
    }
}

void FloatImageDialog::saveImage() {
    QString defaultFileName = "image_" + QDateTime::currentDateTime().toLocalTime().toString() + ".png";
    QString fileName = QFileDialog::getSaveFileName(this, "Save image", defaultFileName, "*.png");
    if (!fileName.isEmpty()) {
        bool saveResult=this->pixmap->toImage().save(fileName, "png");
        if(saveResult){
            qDebug() << "FloatImageDialog: save image to " + fileName;
        }else{
            // TODO add code to handle unable to save file case.
            qDebug() << "Unable to save: " + fileName;
        }
    }
}

void FloatImageDialog::copyImageToClipboard() {
    qDebug() << "FloatImageDialog: Copy to clipboard";
    QApplication::clipboard()->setPixmap(*this->pixmap);
}

void FloatImageDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    this->alignChildViews(event->size());
    // scale the image to the new size
    this->pixmap->scaled(event->size());
    this->ui->imageLabel->setPixmap(this->pixmap->scaled(event->size()));
}

QPoint FloatImageDialog::getTopRightCorner() {
    QPoint fixedTopRightCorner = QPoint(this->x() +this->width(), this->y());
    return fixedTopRightCorner;
}

void FloatImageDialog::moveByTopRight(QPoint newTopRightCorner) {
    // compute new top left corner for new size
    QPoint newTopLeftCorner = QPoint(newTopRightCorner.x() - this->width(), newTopRightCorner.y());
    this->move(newTopLeftCorner);
}

void FloatImageDialog::resetSize() {
    QPoint topRightCorner = this->getTopRightCorner();
    this->resize(this->originalCroppingInfo.size);
    this->moveByTopRight(topRightCorner);
}

void FloatImageDialog::loadState(shared_ptr<FloatImageState> state) {
    this->state = state;
    qDebug()<<state->getPinned();
    this->window()->setWindowFlag(Qt::WindowStaysOnTopHint, state->getPinned());
    if(state->getPinned()){
        this->ui->pinButton->setIcon(QIcon(tr("://pinned_icon_black.png")));
    }else{
        this->ui->pinButton->setIcon(QIcon(tr("://unpinned_icon_black.png")));
    }
    this->resettt();
}

void FloatImageDialog::resettt() {
    this->destroy();
    this->create();
    this->show();
}
