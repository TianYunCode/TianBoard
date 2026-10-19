#include "web/WebTrapWebView.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>
//#include <QWebHitTestResult>
#include <QWebEnginePage>

#include "web/WebKitUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "whiteboard/BoardController.h"


struct WebTrapWebView::Private
{
    QRect mDomElementRect;
    QRect mWebViewElementRect;
    QString mElementQuery;
    WebTrapWebView::WebContentType mCurrentContentType{WebTrapWebView::Unknown};

    bool mIsTrapping{false};

    QWidget* mTrapingWidget{nullptr};
};

struct WebTrapMouseEventMask::Private
{
    WebTrapWebView* mTrappedWebView{nullptr};
};

WebTrapWebView::WebTrapWebView(QWidget* parent)
    : QWebEngineView(parent)
    , d(new Private)
{
    connect(this, &QWebEngineView::loadFinished, this, &WebTrapWebView::viewLoadFinished);
}


WebTrapWebView::~WebTrapWebView()
{
    delete d->mTrapingWidget;
}

bool WebTrapWebView::isTrapping()
{
    return d->mIsTrapping;
}


void WebTrapWebView::setIsTrapping(bool pIsTrapping)
{
    d->mIsTrapping = pIsTrapping;

    d->mDomElementRect = QRect();
    d->mCurrentContentType = Unknown;
/*
#if defined(Q_OS_WIN)

    if(mIsTrapping)
    {
        if (!mTrapingWidget)
        {
            mTrapingWidget = new WebTrapMouseEventMask(this);
            mTrapingWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            mTrapingWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        }

        QPoint topLeft = mapToGlobal (geometry().topLeft());
        QRect geom(topLeft.x(), topLeft.y(), geometry().width(), geometry().height());

        mTrapingWidget->setGeometry(geom);
        mTrapingWidget->setVisible(true);

    }
    else if (mTrapingWidget)
    {
         mTrapingWidget->setVisible(false);
    }

#endif
*/

    update();
}

void WebTrapWebView::hideEvent ( QHideEvent * event )
{
    setIsTrapping(false);
    QWebEngineView::hideEvent(event);
}


void WebTrapWebView::highliteElementAtPos ( const QPoint& pos)
{
    d->mCurrentContentType = Unknown;

    if(page() && page()->webChannel())
    {
        //QWebHitTestResult htr = page()->currentFrame()->hitTestContent (pos);
        //QRect pageHtr = htr.boundingRect().translated(htr.frame()->pos());

        //QRect updateRect = mWebViewElementRect.united(pageHtr);
        //updateRect = updateRect.adjusted(-8, -8, 8, 8);

    //    mDomElementRect = htr.boundingRect();

    //    if (!htr.pixmap().isNull())
    //    {
    //        mCurrentContentType = Image;
    //        qDebug() << "found pixmap at " << htr.boundingRect();
    //    }
    //    else
    //    {
    //        QWebElement element = htr.element();
    //        QString tagName = element.tagName().toLower();

    //        if (tagName == "object"
    //            || tagName == "embed")
    //        {
    //            mCurrentContentType = ObjectOrEmbed;
    //        }
    //        else if ((tagName == "input") || (tagName == "textarea"))
    //        {
    //            QString ec = potentialEmbedCodeAtPos(pos);

    //            if (ec.length() > 0)
    //            {
    //                qDebug() << "found input data \n\n" << ec;
    //                mCurrentContentType = Input;
    //            }
    //        }
    //        else
    //        {
    //            QString tagName = htr.element().tagName();
    //            QString id =  htr.element().attribute("id", "");
    //            QWebElement el = htr.element();

    //            QString idSelector = tagName + "#" + id;
    //            bool idSuccess = (el == el.document().findFirst(idSelector));

    //            if (idSuccess)
    //            {
    //                mElementQuery = idSelector;
    //                mCurrentContentType = ElementByQuery;
    //            }
    //            else
    //            {
    //                //bool isValid = true;

    //                QWebElement elParent = el.parent();
    //                QWebElement currentEl = el;
    //                QString path = tagName;
    //                QStringList pathElements;

    //                do
    //                {
    //                    QWebElement someSibling = elParent.firstChild();

    //                    int index = 0;
    //                    bool foundIndex = false;

    //                    do
    //                    {
    //                        if (someSibling.tagName() == currentEl.tagName())
    //                        {
    //                            if (someSibling == currentEl)
    //                            {
    //                                foundIndex = true;
    //                            }
    //                            else
    //                                index++;
    //                        }

    //                        someSibling = someSibling.nextSibling();
    //                    }
    //                    while(!someSibling.isNull() && !foundIndex);

    //                    QString part;

    //                    if (index > 0)
    //                        part = QString("%1:nth-child(%2)").arg(currentEl.tagName()).arg(index);
    //                    else
    //                        part = currentEl.tagName();

    //                    pathElements.insert(0, part);

    //                    currentEl = elParent;
    //                    elParent = elParent.parent();

    //                } while(!elParent.isNull());

    //                //QString idSelector = tagName + "#" + id;
    //                QString treeSelector =  pathElements.join(" > ");

    //                mElementQuery = treeSelector;
    //                mCurrentContentType = ElementByQuery;

    //                //bool treeSuccess = (el == el.document().findFirst(treeSelector));

    //                //qDebug() << "----------------------------";
    //                //qDebug() << idSuccess << idSelector;
    //                //qDebug() << treeSuccess << treeSelector;
    //            }
    //        }
    //    }

    //    update(updateRect);
    }
}


QString WebTrapWebView::potentialEmbedCodeAtPos(const QPoint& pos)
{

    QString scr = QString("document.elementFromPoint(%1, %2).tagName")
        .arg(pos.x())
        .arg(pos.y());

    //QVariant tagNameVar = page()->runJavaScript(scr);
	QString tagName = "";// tagNameVar.toString().toLower();

    if ((tagName == "input") || (tagName == "textarea"))
    {
        QString valueScript;

        if (tagName == "input")
        {
            valueScript = QString("document.elementFromPoint(%1, %2).value")
                .arg(pos.x())
                .arg(pos.y());
        }
        else if (tagName == "textarea")
        {
            valueScript = QString("document.elementFromPoint(%1, %2).innerHTML")
                .arg(pos.x())
                .arg(pos.y());
        }

        //QVariant valueVar = page()->runJavaScript(valueScript);
        QString value = "";//valueVar.toString();

        if (value.length() > 0 && value.contains("width") && value.contains("height"))
        {
            return value;
        }
    }

    return "";
}


void WebTrapWebView::trapElementAtPos(const QPoint& pos)
{
    /*QWebHitTestResult htr = page()->currentFrame()->hitTestContent(pos);

    if (!htr.pixmap().isNull())
    {
        emit pixmapCaptured(htr.pixmap(), false);
    }
    else if (mCurrentContentType == ObjectOrEmbed)
    {
        QString embedSelector = QString("document.elementFromPoint(%1, %2)").arg(pos.x()).arg(pos.y());
        QVariant tagName = page()->currentFrame()->evaluateJavaScript(embedSelector + ".tagName");

        QVariant innerHTML = page()->currentFrame()->evaluateJavaScript(embedSelector + ".innerHTML");
        qDebug() << "innerHTML" << innerHTML;

        if (tagName.toString().toLower() == "object")
        {
            embedSelector = QString("document.elementFromPoint(%1, %2).getElementsByTagName('object')[0]")
                .arg(pos.x()).arg(pos.y());
        }

        QString querySource = embedSelector + ".src";
        QVariant resSource = page()->currentFrame()->evaluateJavaScript(querySource);
        qDebug() << "resSource" << resSource;
        QString source = resSource.toString();

        QString queryType = embedSelector + ".type";
        QVariant resType = page()->currentFrame()->evaluateJavaScript(queryType);
        QString type = resType.toString();
        qDebug() << "resType" << resType;

        if (source.trimmed().length() > 0)
        {
            emit objectCaptured(QUrl(page()->currentFrame()->url().toString() + "/" + source), type,
                    htr.boundingRect().width(), htr.boundingRect().height());

            Application::boardController->downloadURL(QUrl(source));
            Application::applicationController->showBoard();
        }
    }
    else if (mCurrentContentType == Input)
    {
        QString embedCode = potentialEmbedCodeAtPos(pos);

        if (embedCode.length() > 0)
        {
            emit embedCodeCaptured(embedCode);
        }
    }
    else if (mCurrentContentType == ElementByQuery)
    {
        webElementCaptured(page()->currentFrame()->url(), mElementQuery);
    }*/
}


void WebTrapWebView::mouseMoveEvent ( QMouseEvent * event )
{
    if (d->mIsTrapping)
    {
        highliteElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseMoveEvent(event);
    }
}

void WebTrapWebView::mousePressEvent(QMouseEvent* event)
{
    if (d->mIsTrapping)
    {
        // NOOP - avoid event routing to web kit (accept is not enough)
    }
    else
    {
        //QWebEngineView::mousePressEvent(event);
    }
}


void WebTrapWebView::mouseReleaseEvent ( QMouseEvent * event )
{
    //bool eventConsumed = false;

    //qDebug() << "mouse release" << event->pos();

    if (d->mIsTrapping)
    {
        trapElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseReleaseEvent(event);
    }

    //if (!eventConsumed)


        /*
        QString script;
        script += QString("var element = document.elementFromPoint(%1, %2);").arg(event->pos().x()).arg(event->pos().y());

        script += "var path = '';";
        script += "var currentNode = element;";
        script += "while(currentNode){";

        script += "    if (currentNode.nodeType == Node.ELEMENT_NODE){";
        script += "        var tn = currentNode.tagName;";

        script += "        var elp = currentNode.parentNode;";
        script += "        var index = -1;";
        script += "        if(elp){";

        script += "            for(i = 0; i < elp.childNodes.length; i++){";
        script += "                var node = elp.childNodes[i];";
        script += "                if(node.nodeType == Node.ELEMENT_NODE && node.tagName == tn){";
        script += "                    index++;";
        script += "                }";
        script += "                if(node == currentNode){";
        script += "                    break;";
        script += "                }";

        script += "            }";
        script += "        }";

        script += "        path = tn + '[' + index + '] > ' + path;";
        script += "    }";

        script += "    currentNode = currentNode.parentNode;";
        script += "}";

        script += "'id: ' + element.id + ', TagName: ' + element.tagName + ', at: ' + path;";

        QVariant result = page()->currentFrame()->evaluateJavaScript(script);

        qDebug() << result.isValid() << result.toString();
        */


    //if (result.isValid())
    //{
        //emit trapped(url(), result.toString());
    //}
}


void WebTrapWebView::paintEvent ( QPaintEvent * event )
{
    QWebEngineView::paintEvent(event);

    if (d->mIsTrapping && d->mDomElementRect.isValid() && page())
    {
        QPainter p(this);

        QPen pen;

        if (d->mCurrentContentType != Unknown)
            pen = QPen(QColor(0, 255, 0, 127));
        //else if (mCurrentContentType == ObjectOrEmbed)
        //    pen = QPen(QColor(0, 0, 255, 127));
        //else if (mCurrentContentType == Input)
        //    pen = QPen(QColor(0, 0, 255, 127));
        else
            pen = QPen(QColor(255, 0, 0, 127));

        pen.setJoinStyle(Qt::RoundJoin);
        pen.setWidth(8);
        p.setPen(pen);

		QPoint scrollPosition;//= page()->currentFrame()->scrollPosition();
        QRect webViewPosition = d->mDomElementRect;
        webViewPosition.translate(-scrollPosition);

        d->mWebViewElementRect = webViewPosition.adjusted(-4, -4, 4, 4);

        p.drawRect(d->mWebViewElementRect);

        //pen.setColor(Qt::black);
        //p.setPen(pen);
        //p.drawText(mDomElementRect.adjusted(8, 8, -8, -8), mElementID);
    }

}


void WebTrapWebView::viewLoadFinished ( bool ok )
{
    Q_UNUSED(ok);

}



WebTrapMouseEventMask::WebTrapMouseEventMask(WebTrapWebView* pWebView)
    : d(new Private)
{
    d->mTrappedWebView = pWebView;
    setMouseTracking(true);
}

WebTrapMouseEventMask::~WebTrapMouseEventMask()
{
    // NOOP
}

void WebTrapMouseEventMask::mousePressEvent(QMouseEvent* event)
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        // NOOP
    }
    else
    {
        QWidget::mousePressEvent(event);
    }
}


void WebTrapMouseEventMask::mouseMoveEvent ( QMouseEvent * event )
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        d->mTrappedWebView->highliteElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}


void WebTrapMouseEventMask::mouseReleaseEvent ( QMouseEvent * event )
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        d->mTrappedWebView->trapElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseReleaseEvent(event);
    }
}


void WebTrapMouseEventMask::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(127, 127, 127, 15));

    painter.drawRect(0, 0, width(), height());
}

