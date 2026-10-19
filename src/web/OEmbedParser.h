#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMutex>
#include <new>
#include <memory>
#include <QJSEngine>

typedef struct{
    QString providerUrl;
    QString title;
    QString author;
    int height;
    int width;
    int thumbWidth;
    float version;
    QString authorUrl;
    QString providerName;
    QString thumbUrl;
    QString type;
    QString thumbHeight;
    QString html;
    QString url;
}sOEmbedContent;

class OEmbedParser : public QObject
{
    Q_OBJECT
public:
    OEmbedParser(QObject* parent=0, const char* name="OEmbedParser");
    ~OEmbedParser();
    void parse(const QString& html);
    void setNetworkAccessManager(QNetworkAccessManager* nam);

signals:
    void parseContent(QString url);
    void oembedParsed(QVector<sOEmbedContent> contents);

private slots:
    void onFinished(QNetworkReply* reply);
    void onParseContent(QString url);

private:
    sOEmbedContent getJSONInfos(const QString& json);
    sOEmbedContent getXMLInfos(const QString& xml);
    QVector<sOEmbedContent> mContents;
    QVector<QString> mParsedTitles;
    QNetworkAccessManager* mpNam;
    int mPending;
};

