#pragma once

#include <QWidget>
#include <QSize>

class MdEditor;

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(MdEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    MdEditor *editor_;
};
