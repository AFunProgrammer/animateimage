#ifndef ANIMATEIMAGE_H
#define ANIMATEIMAGE_H

#pragma once
#include <QMainWindow>
#include <cimagealigner.h>

QT_BEGIN_NAMESPACE
namespace Ui { class AnimateImage; }
QT_END_NAMESPACE

class AnimateImage : public QMainWindow
{
    Q_OBJECT

    CImageAligner m_CtlImageAligner;

    void resizeImageAligner();

public:
    AnimateImage(QWidget *parent = nullptr);
    ~AnimateImage();

    void resizeEvent(QResizeEvent* event);

private:
    Ui::AnimateImage *ui;
};
#endif // ANIMATEIMAGE_H
