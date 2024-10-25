#ifndef CFRAME_H
#define CFRAME_H

#include <QObject>

#pragma once
#include "cframeimage.h"
#include "cframetext.h"

class CFrame
{
public:
    CFrame();

private:
    int m_iFrame; //expected frame number
    int m_iDelayTime; //time in ms to hold frame
    QList<CFrameImage> m_lstImages;
    QList<CFrameText> m_lstTexts;

};

#endif // CFRAME_H
