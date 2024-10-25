#ifndef CFRAMEOBJECT_H
#define CFRAMEOBJECT_H

#pragma once

#include <QObject>
#include <QPixmap>


enum class AlignmentAction
{
    None = 0x0,
    Pan,
    Scale,
    Rotate,
    SkewHorizontal,
    SkewVertical
};

class CFrameObject
{
private:
    float m_fOpacity = 1.0f; //Opacity of object
    float m_fScale = 1.0f; //Scale from initial size
    QSizeF m_sFrameScale = QSizeF(1.0f, 1.0f); //Scale against the frame output
    QPoint m_ptPosition = QPoint(0,0); //Position to move within the frame

    int m_iRotation = 0; //Rotation from original position
    QTransform m_tTransform; //Transformation object (is this necessary?)

    QRect m_rctObject = QRect(0,0,1,1);
public:
    CFrameObject();

    void setOpacity(float fOpacity){m_fOpacity=fOpacity;}
    float getOpacity(){return m_fOpacity;}

    void setScale(float fScale){m_fScale=fScale;}
    float getScale(){return m_fScale;}

    void setFrameScale(QSizeF ScaleSize){m_sFrameScale=ScaleSize;}
    QSizeF getFrameScale(){return m_sFrameScale;}

    void setPosition(QPoint Position){m_ptPosition=Position;}
    QPoint getPosition(){return m_ptPosition;}

    void setObjectRect(QRect Rect){m_rctObject=Rect;}
    QRect getObjectRect(){return m_rctObject;}

    bool DoTransform = false;
    bool DoScale = false;

    QRect GetDrawRect(QPoint Center = QPoint(0,0))
    {
        QRect drawRect = QRect(Center.x() + m_ptPosition.x() - m_rctObject.width()/2,
                               Center.y() + m_ptPosition.y() - m_rctObject.height()/2,
                               m_rctObject.width(),
                               m_rctObject.height());

        return drawRect;
    }

};

#endif // CFRAMEOBJECT_H
