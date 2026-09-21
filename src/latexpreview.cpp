#include "latexpreview.h"
#include "platform/qt/graphic_qt.h"

#include <QPainter>
#include <QColor>

using namespace tex;

LatexPreviewWidget::LatexPreviewWidget(QWidget *parent, float text_size) : QWidget(parent), _render(nullptr), _text_size(text_size), _padding(10) {
}

LatexPreviewWidget::~LatexPreviewWidget() {
	if(_render) delete _render;
}

void LatexPreviewWidget::setLaTeX(const std::wstring &latex) {
	if(_render) delete _render;
	_render = nullptr;
	if(!latex.empty()) {
		try {
			QColor c = palette().color(QPalette::WindowText);
			_render = LaTeX::parse(latex, width() - _padding * 2, _text_size, _text_size / 3.f, argb(255, c.red(), c.green(), c.blue()));
		} catch(...) {
			_render = nullptr;
		}
	}
	updateGeometry();
	update();
}

void LatexPreviewWidget::paintEvent(QPaintEvent*) {
	if(_render) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);
		Graphics2D_qt g2(&painter);
		_render->draw(g2, _padding, _padding);
	}
}

QSize LatexPreviewWidget::sizeHint() const {
	if(_render) return QSize(_render->getWidth() + _padding * 2, _render->getHeight() + _padding * 2);
	return QSize(0, (int) (_text_size * 2));
}
