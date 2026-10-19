#include "ui/PropertyPalette.h"


struct PropertyPalette::Private
{
};

PropertyPalette::PropertyPalette(QWidget *parent, const char *name):ActionPalette(parent), d(new Private)
{
    setObjectName(name);
    setGripVisible(false);
}


PropertyPalette::PropertyPalette(Qt::Orientation orientation, QWidget *parent):ActionPalette(orientation, parent), d(new Private)
{
    setGripVisible(false);
}

PropertyPalette::~PropertyPalette()
{

}

void PropertyPalette::onMouseRelease()
{
//    qDebug() << "PropertyPalette::onMouseRelease() called (" << mMousePos.x() << "," << mMousePos.y();
//    QWidget* pW = NULL;
//    pW = childAt(mMousePos);

//    if(NULL != pW)
//    {
//        // A widget has been found under the mouse!
//        ActionPaletteButton* pButton = dynamic_cast<ActionPaletteButton*>(pW);
//        if(NULL != pButton)
//        {
//            pButton->click();
//        }
//    }

//    // Close the palette
//    close();
}
