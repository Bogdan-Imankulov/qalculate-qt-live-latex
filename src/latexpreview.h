// Live LaTeX rendering of the current expression, using MicroTeX (vendor/microtex).
#ifndef LATEXPREVIEW_H
#define LATEXPREVIEW_H

#include "latex.h"

#include <QWidget>

class LatexPreviewWidget : public QWidget {
	public:
		LatexPreviewWidget(QWidget *parent = nullptr, float text_size = 20.f);
		virtual ~LatexPreviewWidget();

		void setLaTeX(const std::wstring &latex);
		void paintEvent(QPaintEvent *event) override;
		QSize sizeHint() const override;

	private:
		tex::TeXRender *_render;
		float _text_size;
		int _padding;
};

#endif
