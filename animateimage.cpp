#include "animateimage.h"
#include "ui_animateimage.h"



void AnimateImage::resizeImageAligner()
{
    QRect rectBounds = QRect(0, 0, this->window()->childrenRect().width(), this->window()->childrenRect().height());

    m_CtlImageAligner.setGeometry(rectBounds);
    m_CtlImageAligner.resizeContents();
    m_CtlImageAligner.update();
}


AnimateImage::AnimateImage(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AnimateImage)
{
    ui->setupUi(this);

    m_CtlImageAligner.setParent(ui->centralwidget);
    resizeImageAligner();

    m_CtlImageAligner.show();
    m_CtlImageAligner.raise();

    this->update();

}

AnimateImage::~AnimateImage()
{
    delete ui;
}

void AnimateImage::resizeEvent(QResizeEvent*)
{
    resizeImageAligner();
}
