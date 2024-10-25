#include "animateimage.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AnimateImage w;
    w.show();
    return a.exec();
}

template <typename T>
class CObjectMethodPointer
{
public:
    typedef void (T::*mfp)();

    alignas(T*) T* m_Object;
    alignas(16) mfp m_Function;

    unsigned long long m_Id;

    CObjectMethodPointer(T* Object, mfp Function) :
        m_Object(Object),
        m_Function(Function)
    {
        m_Id = (unsigned long long)this;
    }

    CObjectMethodPointer& operator=(const CObjectMethodPointer &RHS)
    {
        m_Object = RHS.m_Object;
        m_Function = RHS.m_Function;

        return *this;
    }

    void call()
    {
        (m_Object->*m_Function)();
    }
};

/*
// Action box - for any UI draw actions with associated information
template <typename T>
class ActionBox
{
public:
    ActionBox( QString Name = "",
                QString Tooltip = "",
                QPixmap Pixmap = QPixmap(0,0),
                QRect DrawBox = QRect(0,0,0,0),
                CObjectMethodPointer<T> Action = CObjectMethodPointer<T>(0, 0)) :
        m_Name(Name),
        m_Tooltip(Tooltip),
        m_Pixmap(Pixmap),
        m_DrawBox(DrawBox),
        m_Action(Action)
    {
        qDebug() << "Allocated m_Action: on Initialize: For Object: " << (qulonglong)this;
    }

    ActionBox(const ActionBox& copy ) :
        m_Name(copy.m_Name),
        m_Tooltip(copy.m_Tooltip),
        m_Pixmap(copy.m_Pixmap),
        m_DrawBox(copy.m_DrawBox),
        m_Action(copy.m_Action)
    {
        qDebug() << "Allocated m_Action: on Copy: For Object: " << (qulonglong)this << " From Object: " << (qulonglong)&copy;
    }

    ActionBox(ActionBox&& move) :
        m_Name(std::move(move.m_Name)),
        m_Tooltip(std::move(move.m_Tooltip)),
        m_Pixmap(std::move(move.m_Pixmap)),
        m_DrawBox(std::move(move.m_DrawBox)),
        m_Action(std::move(move.m_Action))
    {
        qDebug() << "Allocated m_Action: on Move: For Object: " << (qulonglong)this << " From Object: " << (qulonglong)&move;
    }

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
    CObjectMethodPointer<T> m_Action;
};

//set the visible UI actions
ActionBox<CImageAligner> Add = ActionBox( "add",
                                          "Add Image",
                                          QPixmap(":/images/Add.png"),
                                          QRect(0,0,1,1),
                                          CObjectMethodPointer(this, &CImageAligner::addImages) );
*/
