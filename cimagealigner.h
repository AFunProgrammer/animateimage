#ifndef CIMAGEALIGNER_H
#define CIMAGEALIGNER_H

#pragma once
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QList>
#include <QInputDialog>
#include <QObject>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QPoint>
#include <QtSvg/QtSvg>
#include <QTimer>
#include <QWidget>
#include <QtOpenGLWidgets/QOpenGLWidget>

#include <functional>
#include <cstring>
#include <iostream>

class CImageAligner;

enum class AlignmentAction
{
    None = 0x0,
    Pan,
    Scale,
    Rotate,
    SkewHorizontal,
    SkewVertical
};

enum class ImageOrientation
{
    Horizontal = 0x0,
    Vertical
};

// Action box - for any UI draw actions with associated information
class ActionBox
{
public:
    ActionBox( QString Name = "",
               QString Tooltip = "",
               QPixmap Pixmap = QPixmap(0,0),
               QRect DrawBox = QRect(0,0,0,0),
               std::function<void(void)> Action = 0) :
        m_Name(Name),
        m_Tooltip(Tooltip),
        m_Pixmap(Pixmap),
        m_DrawBox(DrawBox),
        m_Action(Action)
    {
        m_DrawBox.setWidth(m_Pixmap.width());
        m_DrawBox.setHeight(m_Pixmap.height());
    }

    ActionBox(const ActionBox& copy ) :
        m_Name(copy.m_Name),
        m_Tooltip(copy.m_Tooltip),
        m_Pixmap(copy.m_Pixmap),
        m_DrawBox(copy.m_DrawBox),
        m_Action(copy.m_Action)
    {}

    ActionBox(ActionBox&& move) :
        m_Name(std::move(move.m_Name)),
        m_Tooltip(std::move(move.m_Tooltip)),
        m_Pixmap(std::move(move.m_Pixmap)),
        m_DrawBox(std::move(move.m_DrawBox)),
        m_Action(std::move(move.m_Action))
    {}

    ~ActionBox(){}

    ActionBox& operator=(const ActionBox& RHS)
    {
        m_Name = RHS.m_Name;
        m_Tooltip = RHS.m_Tooltip;
        m_Pixmap = RHS.m_Pixmap;
        m_DrawBox = RHS.m_DrawBox;
        m_Action = RHS.m_Action;

        return *this;
    }

    QString m_Name;
    QString m_Tooltip;
    QPixmap m_Pixmap;
    QRect m_DrawBox;
    std::function<void(void)> m_Action;
};

typedef struct _ImageInfo
{
    QPixmap SourceImage;
    QPixmap ScaledImage;
    QPixmap OutputImage;

    //Opacity control for determining how to display while
    // doing editing of each frame
    float Opacity = 0.0f;
    //Scale of the picture from original to 120% of size of the
    // frame box within the image
    float Scale = 0.0f;
    //Scale of the image with regard to the frame capture box
    QSizeF ScaleXY = QSizeF(0.0f,0.0f);
    QPoint Position = QPoint(0,0);
    int Rotation = 0;
    QTransform Transform;
    bool DoTransform = false;
    bool DoScale = false;

    QRect GetDrawRect(QPoint Center = QPoint(0,0))
    {
        QRect drawRect = QRect(Center.x() + Position.x() - OutputImage.width()/2,
                               Center.y() + Position.y() - OutputImage.height()/2,
                               OutputImage.width(),
                               OutputImage.height());

        return drawRect;
    }
}ImageInfo;

class CImageAligner : public QOpenGLWidget
{
    Q_OBJECT

    AlignmentAction m_AlignmentAction = AlignmentAction::None;

    QQueue<int> m_SelectionHistory;

    QList<ImageInfo> m_ImageInfoList;
    int m_SelectedImage = -1;

    QMap<QString, ActionBox> m_ImageActions;

    QPoint m_ClickSpot = QPoint(0,0);
    QRect m_ClickRect = QRect(0,0,5,5);

    QPoint m_DragChange = QPoint(0,0);
    QPoint m_DragStart = QPoint(0,0);
    bool m_Dragging = false;

    float m_ClickOpacity = 1.0f;
    float m_ImagesOpacity = 0.25f;
    float m_SelectedImageOpacity = 1.0f;
    float m_BoundaryOpacity = 0.75f;

    QBrush m_Clear = QBrush(Qt::black);
    QBrush m_Brush = QBrush(Qt::red);
    QPen m_Pen = QPen(Qt::white,1);

    QPen m_BoundaryPen = QPen(Qt::white,10);
    QBrush m_BoundaryBrush = QBrush(QColor::fromRgba(0x00000000));
    QRect m_BoundaryDrawBox;

    ImageOrientation m_Orientation = ImageOrientation::Horizontal;

    bool m_ClickEvent = true; //Is there a new touch/mouse event?
    bool m_ImageChangeEvent = true;

    QTimer m_DrawTimer;

    void setClickSpot(QMouseEvent* event);

    void checkControlPressedAgainstLastClick();

    void addImages();
    void delImage();

    void nextImage();
    void prevImage();

    void loadImageSet(){}
    void saveImageSet(){}

    void setActionNone()
    {
        m_AlignmentAction = AlignmentAction::None;
        qDebug() << "Action: " << "None";
    }
    void setActionPan(){m_AlignmentAction = AlignmentAction::Pan; qDebug() << "Action: " << "Pan";}
    void setActionScale(){m_AlignmentAction = AlignmentAction::Scale; qDebug() << "Action: " << "Scale";}
    void setActionRotate(){m_AlignmentAction = AlignmentAction::Rotate; qDebug() << "Action: " << "Rotate";}
    void setActionSkewHorizontal(){m_AlignmentAction = AlignmentAction::SkewHorizontal; qDebug() << "Action: " << "Skew Horizontal";}
    void setActionSkewVertical(){m_AlignmentAction = AlignmentAction::SkewVertical; qDebug() << "Action: " << "Skew Vertical";}
    void setOrientation()
    {
        if ( m_Orientation == ImageOrientation::Horizontal )
        {
            m_Orientation = ImageOrientation::Vertical;
            m_ImageActions["orientation"].m_Pixmap = QPixmap(":/images/orientation_10x16_o.png");
        }
        else
        {
            m_Orientation = ImageOrientation::Horizontal;
            m_ImageActions["orientation"].m_Pixmap = QPixmap(":/images/orientation_16x10_o.png");
        }

        resizeBoundaryBox();
    }
    void exportToGif();

    QImage createOutputImage(QRect drawRect, ImageInfo& imageInfo );

    void resizeBoundaryBox();

public:

    CImageAligner();

    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void resizeContents();
};

#endif // CIMAGEALIGNER_H
