#include "cimagealigner.h"

#include <QFileDialog>
#include <QStandardPaths>

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QInputDialog>

#include <functional>
#include <QtMath>

#include <qgifimage.h>


//TODO: Implement Gesture with reverse finger movement (e.g. left up, right down) as rotational gesture
//        Implement Gesture movements within application
//        Output images or save them in memory using the rect (is it possible to render even if black?) [use the hi-res version]
//        (Which animation format to output to?)

#define PI 3.14159265

typedef struct _Quad
{
    int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;
}Quad;

Quad CalculateCornersFromAngle(const QRect& Rect, float Angle)
{
    Quad Out;

    int x1 = (int)(Rect.width()/2*cos((Angle+45)*PI/180));
    int y1 = (int)(Rect.height()/2*sin((Angle+45)*PI/180));
    int x2 = -(int)(Rect.width()/2*cos(Angle*PI/180));
    int y2 = -(int)(Rect.height()/2*sin(Angle*PI/180));

    qDebug() << "Rotation: " << Angle << " Rect: " << Rect << " Changed Corners: " << "x1: " << x1 << " y1: " << y1 << " x2: " << x2 << " y2: " << y2;

    Out.x1 = x1;
    Out.x2 = x2;
    Out.y1 = y1;
    Out.y2 = y2;

    return Out;
}


CImageAligner::CImageAligner()
{
    m_DrawTimer.setInterval(33);
    m_DrawTimer.connect(&m_DrawTimer, &QTimer::timeout, [this]()
    {
        if ( m_ClickEvent == true )

            m_ClickOpacity = 1.0f;
        else
        {
            if ( m_ClickOpacity > 0.1f )
                m_ClickOpacity -= 0.05f;
        }

        if ( m_ImageChangeEvent )
            m_SelectedImageOpacity = 1.0f;
        else
        {
            if ( m_SelectedImageOpacity > 0.0f )
                m_SelectedImageOpacity -= 0.05f;
        }

        static int iCount = 0;
        iCount++;

        if ( m_Dragging )
        {
            if ( m_SelectedImage != -1 )
            {
                bool bContainsClick = m_ImageInfoList[m_SelectedImage].GetDrawRect(m_BoundaryDrawBox.center()).contains(m_ClickSpot);
                m_DragChange = QPoint(m_ClickSpot.x() - m_DragStart.x(), m_ClickSpot.y() - m_DragStart.y());
                QPoint center = m_ImageInfoList[m_SelectedImage].ScaledImage.rect().center();
                int Rotation = m_ImageInfoList[m_SelectedImage].Rotation;
                float scaleRotationX = 0;
                float scaleRotationY = 0;

                switch( m_AlignmentAction)
                {
                case AlignmentAction::Pan:
                    if ( bContainsClick )
                        m_ImageInfoList[m_SelectedImage].Position += m_DragChange / 2;
                    break;
                case AlignmentAction::Rotate:
                    m_ImageInfoList[m_SelectedImage].Rotation -= m_DragChange.x() / 10;
                    m_ImageInfoList[m_SelectedImage].Transform.reset();
                    m_ImageInfoList[m_SelectedImage].Transform.translate(-center.x(), -center.y());
                    m_ImageInfoList[m_SelectedImage].Transform.rotate(m_ImageInfoList[m_SelectedImage].Rotation);
                    //qDebug() << "Rotation: " << Rotation;
                    if (m_ImageInfoList[m_SelectedImage].Rotation >= 360)
                        m_ImageInfoList[m_SelectedImage].Rotation = 0;

                    m_ImageInfoList[m_SelectedImage].DoTransform = true;
                    break;
                case AlignmentAction::Scale:
                    scaleRotationX = static_cast<float>(m_DragChange.x())*cos((Rotation)*PI/180) + static_cast<float>(m_DragChange.y())*sin((Rotation)*PI/180);
                    scaleRotationY = static_cast<float>(m_DragChange.x())*sin((Rotation)*PI/180) + static_cast<float>(m_DragChange.y())*cos((Rotation)*PI/180);
                    qDebug() << "OnScale: Scaling: " << scaleRotationX << ", " << scaleRotationY << " Drag Change: " << m_DragChange;
                    m_ImageInfoList[m_SelectedImage].ScaleXY += QSizeF(scaleRotationX/100.0f, scaleRotationY/100.0f);
                    m_ImageInfoList[m_SelectedImage].DoScale = true;
                    m_ImageInfoList[m_SelectedImage].DoTransform = true;
                    break;
                case AlignmentAction::SkewHorizontal:
                    break;
                case AlignmentAction::SkewVertical:
                    break;
                }

                //update the drag start position, only if it is being used to move an image
                m_DragStart = m_ClickSpot;
            }
        }
        this->update();
    });//end of timer loop

    //set the visible UI actions
    ActionBox Add = ActionBox("add", "Add Image", QPixmap(":/images/Add.png"), QRect(0,0,1,1), std::bind(&CImageAligner::addImages, this));
    ActionBox Del = ActionBox( "remove", "Remove Image", QPixmap(":/images/Minus.png"), QRect(0,0,1,1), std::bind(&CImageAligner::delImage, this) );
    ActionBox Right = ActionBox( "next", "Next Image", QPixmap(":/images/RightArrowSmall.png"), QRect(0,0,1,1), std::bind(&CImageAligner::nextImage, this) );
    ActionBox Left = ActionBox( "previous", "Previous Image", QPixmap(":/images/LeftArrowSmall.png"), QRect(0,0,1,1), std::bind(&CImageAligner::prevImage, this));

    ActionBox Load = ActionBox( "load", "Load Saved Set", QPixmap(":/images/load_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::loadImageSet, this));

    //add invisible elements or not drawn until called upon (maybe...)
    ActionBox Pan = ActionBox( "pan", "Pan Image", QPixmap(":/images/pan_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionPan, this));
    ActionBox Scale = ActionBox( "scale", "Scale Image", QPixmap(":/images/scale_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionScale, this));
    ActionBox Rotate = ActionBox( "rotate", "Rotate Image", QPixmap(":/images/rotate_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionRotate, this));
    ActionBox SkewHorizontal = ActionBox( "skewhorizontal", "Skew Horizontal", QPixmap(":/images/trapezoidhorizontal.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionSkewHorizontal, this));
    ActionBox SkewVertical = ActionBox( "skewvertical", "Skew Vertical", QPixmap(":/images/trapezoidvertical.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionSkewVertical, this));
    ActionBox NoAction = ActionBox( "noaction", "No Action", QPixmap(":/images/noaction_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setActionNone, this));

    ActionBox Export = ActionBox( "export", "Export To Gif", QPixmap(":/images/save_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::exportToGif, this));
    ActionBox Orientation = ActionBox( "orientation", "Change Output Orientation", QPixmap(":/images/orientation_16x10_o.png"), QRect(0,0,1,1), std::bind(&CImageAligner::setOrientation, this));

    m_ImageActions[Add.m_Name]          = Add;
    m_ImageActions[Del.m_Name]          = Del;
    m_ImageActions[Right.m_Name]        = Right;
    m_ImageActions[Left.m_Name]         = Left;
    m_ImageActions[Pan.m_Name]          = Pan;
    m_ImageActions[Scale.m_Name]        = Scale;
    m_ImageActions[Rotate.m_Name]       = Rotate;
    m_ImageActions[SkewHorizontal.m_Name] = SkewHorizontal;
    m_ImageActions[SkewVertical.m_Name]   = SkewVertical;
    m_ImageActions[NoAction.m_Name]     = NoAction;
    m_ImageActions[Export.m_Name]       = Export;
    m_ImageActions[Orientation.m_Name]  = Orientation;

    //set the focus policy to make sure and receive keyboard input
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // Start the timer with the awesome lambda inside :-)
    m_DrawTimer.start(33);
}

void CImageAligner::exportToGif()
{
#if defined(Q_OS_WINDOWS)
    QString saveFileFilter = "Gif (*.gif)";
#else
    QString saveFileFilter = "Gif (*.gif)";
#endif
    static QString saveFilePath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0];
    QFileDialog saveDialog = QFileDialog(this, tr("Save GIF Image File"), saveFilePath, saveFileFilter);
    QString saveFilename;

    // don't export to Gif if there are no images to be saved
    if (m_ImageInfoList.count() <= 0)
        return;

    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix(tr("gif"));
    saveDialog.setNameFilter(tr("Gif Files(*.gif)"));

    saveFilename = saveDialog.getSaveFileName();

    if ( saveFilename == "" || saveFilename.trimmed() == "" )
        return;

    if ( saveFilename.endsWith(tr(".gif"),Qt::CaseInsensitive) == false )
        saveFilename += tr(".gif");

    QGifImage gifImage;

    QInputDialog getDefaultDelay(this);
    getDefaultDelay.setInputMode(QInputDialog::IntInput);
    getDefaultDelay.setIntRange(50,1000);
    getDefaultDelay.setIntStep(50);
    getDefaultDelay.setIntValue(250);
    getDefaultDelay.setLabelText("Select default frame delay time (in ms)");
    getDefaultDelay.setOkButtonText("Set Delay");
    getDefaultDelay.setCancelButtonText(tr("Set Delay"));
    getDefaultDelay.exec();

    int iDelayTime = getDefaultDelay.intValue();

    gifImage.setDefaultDelay(iDelayTime);
    for( ImageInfo image: this->m_ImageInfoList)
    {
        gifImage.addFrame(createOutputImage(m_BoundaryDrawBox,image));
    }

    gifImage.save(saveFilename);
}



void CImageAligner::addImages()
{
    static QString openFilePath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0];
#if defined(Q_OS_WINDOWS)
    QString openFileFilter = "Images (*.png *.jpg *.jpeg *.bmp)";
#else
    QString openFileFilter = "Images (*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.bmp *.BMP)";
#endif

    QStringList selectedFiles;
    QFileDialog openDialog = QFileDialog(this, QString("Add Image File"), openFilePath, openFileFilter);

    openDialog.setAcceptMode(QFileDialog::AcceptOpen);
    openDialog.setFileMode(QFileDialog::ExistingFiles);

    if ( openDialog.exec() )
        selectedFiles = openDialog.selectedFiles();

    openFilePath = openDialog.directory().absolutePath();

    //Load in reverse order because the user selected the centerimages
    // to be loaded in this order specifically
    for(int idx = 0; idx < selectedFiles.count(); idx++)
    {
        ImageInfo imageInfo;
        imageInfo.SourceImage = QPixmap(selectedFiles[idx]);
        imageInfo.OutputImage = imageInfo.ScaledImage;
        imageInfo.Position = QPoint(0,0);
        imageInfo.Opacity = 0.25f;
        //TODO: Figure out scale with regard to output scene dimensions
        if ( m_Orientation == ImageOrientation::Horizontal )
            imageInfo.Scale = static_cast<float>(m_BoundaryDrawBox.width()) / static_cast<float>(imageInfo.SourceImage.width());
        else
            imageInfo.Scale = static_cast<float>(m_BoundaryDrawBox.height()) / static_cast<float>(imageInfo.SourceImage.height());

        imageInfo.ScaleXY = QSizeF(1.2f, 1.2f);
        imageInfo.ScaledImage = imageInfo.SourceImage.scaled( static_cast<int>(static_cast<float>(imageInfo.SourceImage.width()) * imageInfo.ScaleXY.width() * imageInfo.Scale),
                                                              static_cast<int>(static_cast<float>(imageInfo.SourceImage.height()) * imageInfo.ScaleXY.height() * imageInfo.Scale));

        imageInfo.OutputImage = QPixmap(imageInfo.ScaledImage);

        //set scale against size of box to be just a bit larger
        if ( m_SelectedImage != -1 )
            m_ImageInfoList.insert(m_SelectedImage, imageInfo);
        else
            m_ImageInfoList.append(imageInfo);
    }

    //Increment every selection history based upon number of images
    // inserted and the current index that was selected
    for ( int i = 0; i < m_SelectionHistory.count(); i++ )
    {
        if ( m_SelectionHistory[i] >= m_SelectedImage )
            m_SelectionHistory[i]+=selectedFiles.count();
    }
}

void CImageAligner::delImage()
{
    int iRemoved = 0;

    if ( m_ImageInfoList.count() == 0 )
        return;

    if ( m_SelectedImage != -1 )
    {
        iRemoved = m_SelectedImage;
        m_ImageInfoList.removeAt(m_SelectedImage);
        if ( m_SelectedImage > 0 )
            m_SelectedImage--;
    }
    else
    {
        iRemoved = m_ImageInfoList.count()-1;
        m_ImageInfoList.removeLast();
    }

    for ( int i = 0; i < m_SelectionHistory.count(); i++ )
    {
        if ( m_SelectionHistory[i] > iRemoved )
            m_SelectionHistory[i]--;
    }

    if ( m_ImageInfoList.count() == 0)
        m_SelectedImage = -1;
}

void CImageAligner::nextImage()
{
    if ( m_SelectedImage < m_ImageInfoList.count() - 1)
        m_SelectedImage++;
    else
        m_SelectedImage = 0;

    m_ImageChangeEvent = true;

    if ( m_ImageInfoList.count() == 0 )
        m_SelectedImage = -1;
    else
        m_SelectionHistory.append(m_SelectedImage);

    if (m_SelectionHistory.count() >= 10 )
        m_SelectionHistory.dequeue();
}

void CImageAligner::prevImage()
{
    if ( m_SelectedImage > 0)
        m_SelectedImage--;
    else
        m_SelectedImage = m_ImageInfoList.count() - 1;

    m_ImageChangeEvent = true;

    if ( m_ImageInfoList.count() == 0 )
        m_SelectedImage = -1;
    else
        m_SelectionHistory.append(m_SelectedImage);

    if (m_SelectionHistory.count() >= 10 )
        m_SelectionHistory.dequeue();
}

void CImageAligner::setClickSpot(QMouseEvent* event)
{
    QPainter qpMeasure(this);
    QPoint clickPos = event->pos();
    QRect clickArea = QRect(0,0,0,0);

    float fDpcmX = (float)qpMeasure.device()->logicalDpiX() / 2.54f;
    float fDpcmY = (float)qpMeasure.device()->logicalDpiY() / 2.54f;

    //Click area is considered the 'nice' area of being clicked in
    // allowing for 'fat' finger touch to select a specific spot
    // within a region and allowing for making mistakes but
    // still having the pleasure of using touch
    clickArea.setX(clickPos.x() - (int)(fDpcmX/2.0f) );
    clickArea.setY(clickPos.y() - (int)(fDpcmY/2.0f) );
    clickArea.setWidth((int)(fDpcmX) );
    clickArea.setHeight((int)(fDpcmY) );

    m_ClickSpot = clickPos;
    m_ClickRect = clickArea;
    m_ClickEvent = true;
}

void CImageAligner::checkControlPressedAgainstLastClick()
{
#if defined(Q_OS_ANDROID)
    for( ActionBox action: m_ImageActions )
    {
        if( action.m_DrawBox.intersects(m_ClickRect) )
        {
#else
    for( ActionBox action: m_ImageActions )
    {
        if( action.m_DrawBox.contains(m_ClickSpot) )
        {
#endif
            action.m_Action();
            break; //found the associated ui element, no further processing needed
        }
    }
}


void CImageAligner::mouseMoveEvent(QMouseEvent* event)
{
    Qt::MouseButtons mouseButtons = qGuiApp->mouseButtons();
    if ( mouseButtons != Qt::NoButton )
    {
        //qDebug() << "CImageAligner::mouseMoveEvent";
        setClickSpot(event);
    }
}

void CImageAligner::mousePressEvent(QMouseEvent *event)
{
    //qDebug() << "CImageAligner::mousePressEvent";
    m_Dragging = true;
    m_DragStart = event->pos();

    setClickSpot(event);

    checkControlPressedAgainstLastClick();
}

void CImageAligner::mouseReleaseEvent(QMouseEvent* event)
{
    m_Dragging = false;
}


void CImageAligner::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Left:
            prevImage();
            break;
        case Qt::Key_Right:
            nextImage();
            break;
    }
}

void CImageAligner::resizeBoundaryBox()
{
    // Adjust draw box to be 16:9 in given area
    // TODO: allow customization of draw box

    int iWidth = 0;
    int iHeight = 0;

    if ( m_Orientation == ImageOrientation::Horizontal )
    {
        if ( ((geometry().width()/16)*10) > geometry().height() )
        {
            iWidth = (int)(((float)((geometry().height()/10)*16))*0.8f);
            iHeight =  (int)(((float)geometry().height())*0.8f);
        }
        else
        {
            iWidth =  (int)(((float)geometry().width())*0.8f);
            iHeight =  (int)(((float)((geometry().width()/16)*10))*0.8f);
        }
    }
    else
    {
        if ( ((geometry().height()/16)*10) > geometry().width() )
        {
            iWidth =  (int)(((float)geometry().width())*0.8f);
            iHeight =  (int)(((float)((geometry().width()/10)*16))*0.8f);
        }
        else
        {
            iWidth = (int)(((float)((geometry().height()/16)*10))*0.8f);
            iHeight =  (int)(((float)geometry().height())*0.8f);
        }
    }

    m_BoundaryDrawBox.setLeft( (geometry().width() - iWidth) / 2 );
    m_BoundaryDrawBox.setTop( (geometry().height() - iHeight) / 2 );
    m_BoundaryDrawBox.setWidth(iWidth);
    m_BoundaryDrawBox.setHeight(iHeight);
}


void CImageAligner::resizeContents()
{
    m_ImageActions["add"].m_DrawBox = QRect(0,
                         geometry().bottom() - m_ImageActions["add"].m_Pixmap.height(),
                         m_ImageActions["add"].m_Pixmap.width(),
                         m_ImageActions["add"].m_Pixmap.height());

    m_ImageActions["remove"].m_DrawBox = QRect(geometry().right() - m_ImageActions["remove"].m_Pixmap.width(),
                         geometry().bottom() - m_ImageActions["remove"].m_Pixmap.height(),
                         m_ImageActions["remove"].m_Pixmap.width(),
                         m_ImageActions["remove"].m_Pixmap.height());

    m_ImageActions["previous"].m_DrawBox = QRect(0,
                          geometry().bottom() / 2 - m_ImageActions["previous"].m_Pixmap.height() / 2,
                          m_ImageActions["previous"].m_Pixmap.width(),
                          m_ImageActions["previous"].m_Pixmap.height());

    m_ImageActions["next"].m_DrawBox = QRect(geometry().right() - m_ImageActions["next"].m_Pixmap.width(),
                           geometry().bottom() / 2 - m_ImageActions["next"].m_Pixmap.height() / 2,
                           m_ImageActions["next"].m_Pixmap.width(),
                           m_ImageActions["next"].m_Pixmap.height());

    m_ImageActions["pan"].m_DrawBox = QRect(10, 10, 32, 32);

    m_ImageActions["scale"].m_DrawBox = QRect(10, m_ImageActions["pan"].m_DrawBox.top() + 32 + 20, 32, 32);
    m_ImageActions["rotate"].m_DrawBox = QRect(10, m_ImageActions["scale"].m_DrawBox.top() + 32 + 20, 32, 32);
    m_ImageActions["skewhorizontal"].m_DrawBox = QRect(geometry().right() - 32 - 10, 10, 32, 32);
    m_ImageActions["skewvertical"].m_DrawBox = QRect(geometry().right() - 32 - 10, m_ImageActions["skewhorizontal"].m_DrawBox.top() + 32 + 20, 32, 32);
    m_ImageActions["noaction"].m_DrawBox = QRect(geometry().right() - 32 - 10, m_ImageActions["skewvertical"].m_DrawBox.top() + 32 + 20, 32, 32);

    m_ImageActions["export"].m_DrawBox = QRect(geometry().right() - 32 - 10, m_ImageActions["noaction"].m_DrawBox.top() + 32 + 20, 32, 32);
    m_ImageActions["orientation"].m_DrawBox = QRect(geometry().right() - 32 - 10, m_ImageActions["export"].m_DrawBox.top() + 32 + 20, 32, 32);

    //Resize the output boundary box
    resizeBoundaryBox();

    // Resize all the scaled images (takes time...
    //  TODO: Find way to do this process after all scaling has been done
    for( int idx = 0; idx < m_ImageInfoList.count(); idx++ )
    {
        //TODO: Figure out scale with regard to output scene dimensions
        m_ImageInfoList[idx].Scale = static_cast<float>(m_BoundaryDrawBox.width()) / static_cast<float>(m_ImageInfoList[idx].SourceImage.width());
        QSize scaledSize = QSize( static_cast<int>(static_cast<float>(m_ImageInfoList[idx].SourceImage.width()) * m_ImageInfoList[idx].ScaleXY.width() * m_ImageInfoList[idx].Scale),
                                  static_cast<int>(static_cast<float>(m_ImageInfoList[idx].SourceImage.height()) * m_ImageInfoList[idx].ScaleXY.height() * m_ImageInfoList[idx].Scale));

        m_ImageInfoList[idx].ScaledImage = m_ImageInfoList[idx].SourceImage.scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_ImageInfoList[idx].OutputImage = m_ImageInfoList[idx].ScaledImage.transformed(m_ImageInfoList[idx].Transform, Qt::FastTransformation);

        qDebug() << "OnResize: Scaled Size: " << scaledSize;
    }
}

QImage CImageAligner::createOutputImage(QRect drawRect, ImageInfo& imageInfo )
{
    static int iOutput = 0;

    QRect outRect = QRect(0,0,drawRect.width(), drawRect.height());
    QPoint center = outRect.center();
    QImage image;

    /*
    //If there is a rotation then there is a need to determine the real extents of the
    // image that needs to be drawn
    if (imageInfo.Rotation != 0 )
    {
        QPoint ptMax, ptMin;
        //find min and max x,y

        ptMax.setX((int)fmax(drawRect.x()*cos((imageInfo.Rotation)*PI/180), drawRect.width()*cos((imageInfo.Rotation)*PI/180)));
        ptMin.setX((int)fmin(drawRect.x()*cos((imageInfo.Rotation)*PI/180), drawRect.width()*cos((imageInfo.Rotation)*PI/180)));
        ptMax.setY((int)fmax(drawRect.y()*sin((imageInfo.Rotation)*PI/180), drawRect.height()*sin((imageInfo.Rotation)*PI/180)));
        ptMin.setY((int)fmin(drawRect.y()*sin((imageInfo.Rotation)*PI/180), drawRect.height()*sin((imageInfo.Rotation)*PI/180)));

        imgRect.setLeft(ptMin.x());
        imgRect.setTop(ptMin.y());
        imgRect.setRight(ptMax.x());
        imgRect.setBottom(ptMax.y());
    }
    */

    //new pixmap to draw to
    QPixmap pixmap(outRect.width(),outRect.height());

    //painter to do the drawing
    QPainter painter(&pixmap);

    //draw to the specific pixmap
    QSize scaledSize = QSize( static_cast<int>(static_cast<float>(imageInfo.SourceImage.width()) * imageInfo.ScaleXY.width() * imageInfo.Scale),
                              static_cast<int>(static_cast<float>(imageInfo.SourceImage.height()) * imageInfo.ScaleXY.height() * imageInfo.Scale));

    imageInfo.ScaledImage = imageInfo.SourceImage.scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    imageInfo.OutputImage = imageInfo.ScaledImage.transformed(imageInfo.Transform, Qt::SmoothTransformation);

    QRect rctOutput = QRect(center.x() + imageInfo.Position.x() - imageInfo.OutputImage.width()/2,
                            center.y() + imageInfo.Position.y() - imageInfo.OutputImage.height()/2,
                            imageInfo.OutputImage.width(),
                            imageInfo.OutputImage.height());

    painter.fillRect(outRect, Qt::black);
    painter.drawPixmap(rctOutput, imageInfo.OutputImage);
    painter.end();
    //convert to an image
    image = pixmap.toImage();

#if defined(Q_OS_WINDOWS)
    QString filename =  QString("c:\\temp\\" + QString::number(iOutput) + ".png");
#else
    QString filename = QString("/home/grandma/images/" + QString::number(iOutput) + ".png");
#endif

    //bool bSaved = image.save(filename);
    //qDebug() << "Saved File: " << filename << " Success: " << bSaved;
    iOutput++;

    return image;
}

void CImageAligner::paintEvent(QPaintEvent *event)
{
    QPainter Painter(this);

    //Save the initial state
    Painter.save();

    Painter.fillRect(event->rect(),m_Clear);

    //Set image opacities
    int iPrev = -1;
    if ( m_SelectionHistory.count() >= 2 )
    {
        int iCurrent = m_SelectionHistory[m_SelectionHistory.count() - 1];
        iPrev = m_SelectionHistory[m_SelectionHistory.count() - 2];
        int iPrevPrev = -1;

        if ( m_SelectionHistory.count() >= 3 )
            iPrevPrev = m_SelectionHistory[m_SelectionHistory.count() - 3];

        //qDebug() << "Current: " << iCurrent << " Prev: " << iPrev << " Prev Prev: " << iPrevPrev;

        if ( iPrevPrev > -1 && iPrevPrev < m_ImageInfoList.count())
            m_ImageInfoList[iPrevPrev].Opacity = 0.25f;

        if ( iCurrent > -1 && iCurrent < m_ImageInfoList.count())
            m_ImageInfoList[iCurrent].Opacity = 0.50f;

        if ( iPrev > -1 && iPrev < m_ImageInfoList.count())
            m_ImageInfoList[iPrev].Opacity = 0.75f;
    }

    //Draw Images
    Painter.setOpacity(0.25f);
    for( int idx = 0; idx < m_ImageInfoList.count(); idx++ )
    {
        Painter.setOpacity(m_ImageInfoList[idx].Opacity);

        if ( idx != m_SelectedImage && idx != iPrev )
            Painter.drawPixmap(m_ImageInfoList[idx].GetDrawRect(m_BoundaryDrawBox.center()), m_ImageInfoList[idx].OutputImage);
    }

    if ( iPrev > -1 && iPrev < m_ImageInfoList.count() && iPrev != m_SelectedImage )
    {
        Painter.setOpacity(0.50f);
        Painter.drawPixmap(m_ImageInfoList[iPrev].GetDrawRect(m_BoundaryDrawBox.center()), m_ImageInfoList[iPrev].OutputImage);
    }


    //Draw Selected Image Last to make more visible
    if ( m_SelectedImage > -1 )
    {
        Painter.setOpacity(1.0f);
        Painter.setBrush(Qt::white);
        Painter.setPen(Qt::red);
        Painter.drawText(QPoint(200,30),std::to_string(m_SelectedImage).c_str());

        Painter.setOpacity(m_ImageInfoList[m_SelectedImage].Opacity + m_SelectedImageOpacity);
        //Painter.save();

        if ( m_ImageInfoList[m_SelectedImage].DoScale )
        {
            QSize scaledSize = QSize( static_cast<int>(static_cast<float>(m_ImageInfoList[m_SelectedImage].SourceImage.width()) * m_ImageInfoList[m_SelectedImage].ScaleXY.width() * m_ImageInfoList[m_SelectedImage].Scale),
                                      static_cast<int>(static_cast<float>(m_ImageInfoList[m_SelectedImage].SourceImage.height()) * m_ImageInfoList[m_SelectedImage].ScaleXY.height() * m_ImageInfoList[m_SelectedImage].Scale));

            m_ImageInfoList[m_SelectedImage].ScaledImage = m_ImageInfoList[m_SelectedImage].SourceImage.scaled(scaledSize, Qt::IgnoreAspectRatio,  Qt::FastTransformation);
            m_ImageInfoList[m_SelectedImage].DoScale = false;
        }

        if ( m_ImageInfoList[m_SelectedImage].DoTransform )
        {
            m_ImageInfoList[m_SelectedImage].OutputImage = m_ImageInfoList[m_SelectedImage].ScaledImage.transformed(m_ImageInfoList[m_SelectedImage].Transform,
                                                                                                                    Qt::FastTransformation);
            m_ImageInfoList[m_SelectedImage].DoTransform = false;
        }

        Painter.drawPixmap(m_ImageInfoList[m_SelectedImage].GetDrawRect(m_BoundaryDrawBox.center()), m_ImageInfoList[m_SelectedImage].OutputImage);

        //qDebug() << "Center: " << m_ImageInfoList[m_SelectedImage].OutputImage.rect().center() << " Rect: " << m_ImageInfoList[m_SelectedImage].OutputImage.rect() << " Rotation: " << m_ImageInfoList[m_SelectedImage].Rotation;
        //Painter.restore();
    }

    //Draw Touch Spot
    Painter.setBrush(m_Brush);
    Painter.setPen(m_Pen);

    Painter.setOpacity(m_ClickOpacity);
    Painter.drawEllipse(m_ClickRect);

    //Draw the boundary for the frame
    Painter.setBrush(m_BoundaryBrush);
    Painter.setPen(m_BoundaryPen);
    Painter.setOpacity(0.75f);

    Painter.drawRect(m_BoundaryDrawBox);

    //Draw the Left/Right Add/Del last in the list
    Painter.restore();
    Painter.setRenderHint(QPainter::SmoothPixmapTransform);
    Painter.drawPixmap(m_ImageActions["add"].m_DrawBox, m_ImageActions["add"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["remove"].m_DrawBox, m_ImageActions["remove"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["previous"].m_DrawBox, m_ImageActions["previous"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["next"].m_DrawBox, m_ImageActions["next"].m_Pixmap);

    Painter.setOpacity(0.75f);
    Painter.setRenderHint(QPainter::SmoothPixmapTransform);
    Painter.drawPixmap(m_ImageActions["pan"].m_DrawBox, m_ImageActions["pan"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["scale"].m_DrawBox, m_ImageActions["scale"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["rotate"].m_DrawBox, m_ImageActions["rotate"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["skewhorizontal"].m_DrawBox, m_ImageActions["skewhorizontal"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["skewvertical"].m_DrawBox, m_ImageActions["skewvertical"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["noaction"].m_DrawBox, m_ImageActions["noaction"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["export"].m_DrawBox, m_ImageActions["export"].m_Pixmap);
    Painter.drawPixmap(m_ImageActions["orientation"].m_DrawBox, m_ImageActions["orientation"].m_Pixmap);

    /*
    for( ActionBox action: m_ImageActions )
    {
        Painter.drawPixmap(action.m_DrawBox, action.m_Pixmap);
    }*/

    Painter.end();

    m_ClickEvent = false;
    m_ImageChangeEvent = false;
}
