#pragma once

#include <QWidget>

#include <memory>

class QComboBox;
class QSlider;
class QSplitter;
class QToolButton;

class DocumentThumbnailWidget;
class DocumentTreeView;

// Hand written replacement for ui/forms/documents.ui. The class owns the whole
// widget tree the generated Ui::documents class used to build, so DocumentController
// only ever talks to it through the accessors below.
class DocumentsWidget : public QWidget {
    Q_OBJECT

public:
    DocumentsWidget(QWidget *parent = nullptr);
    ~DocumentsWidget() override;

    DocumentTreeView *documentTreeView() const;
    DocumentThumbnailWidget *thumbnailWidget() const;

    QComboBox *sortKind() const;
    QToolButton *sortOrder() const;

    QSplitter *splitter() const;
    QSlider *documentZoomSlider() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
