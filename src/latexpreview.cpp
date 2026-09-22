#include "latexpreview.h"
#include "platform/qt/graphic_qt.h"

#include <QPainter>
#include <QColor>
#include <QRegularExpression>
#include <QSet>

using namespace tex;

// libqalculate's LaTeX output uses a large, open-ended set of siunitx-style
// unit/prefix commands (\radian, \kg, \kilo, ...) that MicroTeX doesn't
// define; it renders any command it doesn't recognize as literal red error
// text. Rather than trying to keep a matching list of every unit
// libqalculate might mention (it has hundreds, and which name variant ends
// up in the LaTeX depends on the active UI language), whitelist the fixed,
// small vocabulary of real LaTeX/math commands and treat everything else as
// plain text instead - degrades gracefully instead of erroring out.
static const QSet<QString> &latexCommandWhitelist() {
	static const QSet<QString> whitelist = {
		QStringLiteral("num"), QStringLiteral("qty"), QStringLiteral("si"), QStringLiteral("unit"),
		QStringLiteral("per"), QStringLiteral("square"), QStringLiteral("cubic"),
		QStringLiteral("displaystyle"), QStringLiteral("textstyle"), QStringLiteral("scriptstyle"),
		QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("begin"), QStringLiteral("end"),
		QStringLiteral("quad"), QStringLiteral("qquad"), QStringLiteral("text"),
		QStringLiteral("mathrm"), QStringLiteral("mathbf"), QStringLiteral("mathit"), QStringLiteral("mathsf"),
		QStringLiteral("mathtt"), QStringLiteral("mathbb"), QStringLiteral("mathcal"), QStringLiteral("operatorname"),
		QStringLiteral("overline"), QStringLiteral("underline"), QStringLiteral("vec"), QStringLiteral("hat"),
		QStringLiteral("dot"), QStringLiteral("ddot"), QStringLiteral("dddot"), QStringLiteral("tilde"), QStringLiteral("bar"),
		QStringLiteral("frac"), QStringLiteral("dfrac"), QStringLiteral("tfrac"), QStringLiteral("genfrac"), QStringLiteral("sqrt"),
		QStringLiteral("binom"), QStringLiteral("dbinom"), QStringLiteral("tbinom"),
		QStringLiteral("sum"), QStringLiteral("prod"), QStringLiteral("int"), QStringLiteral("oint"),
		QStringLiteral("lim"), QStringLiteral("sup"), QStringLiteral("inf"), QStringLiteral("det"), QStringLiteral("gcd"),
		QStringLiteral("max"), QStringLiteral("min"), QStringLiteral("arg"), QStringLiteral("mod"), QStringLiteral("pmod"),
		QStringLiteral("sin"), QStringLiteral("cos"), QStringLiteral("tan"), QStringLiteral("cot"),
		QStringLiteral("sec"), QStringLiteral("csc"), QStringLiteral("arcsin"), QStringLiteral("arccos"),
		QStringLiteral("arctan"), QStringLiteral("sinh"), QStringLiteral("cosh"), QStringLiteral("tanh"),
		QStringLiteral("ln"), QStringLiteral("log"), QStringLiteral("exp"),
		QStringLiteral("infty"), QStringLiteral("partial"), QStringLiteral("nabla"),
		QStringLiteral("times"), QStringLiteral("div"), QStringLiteral("cdot"), QStringLiteral("cdots"),
		QStringLiteral("ldots"), QStringLiteral("vdots"), QStringLiteral("ddots"),
		QStringLiteral("pm"), QStringLiteral("mp"), QStringLiteral("leq"), QStringLiteral("geq"),
		QStringLiteral("neq"), QStringLiteral("approx"), QStringLiteral("equiv"), QStringLiteral("sim"),
		QStringLiteral("simeq"), QStringLiteral("propto"),
		QStringLiteral("in"), QStringLiteral("notin"), QStringLiteral("subset"), QStringLiteral("subseteq"),
		QStringLiteral("supset"), QStringLiteral("supseteq"), QStringLiteral("cup"), QStringLiteral("cap"),
		QStringLiteral("setminus"), QStringLiteral("emptyset"), QStringLiteral("forall"), QStringLiteral("exists"),
		QStringLiteral("neg"), QStringLiteral("wedge"), QStringLiteral("vee"), QStringLiteral("oplus"), QStringLiteral("otimes"),
		QStringLiteral("rightarrow"), QStringLiteral("leftarrow"), QStringLiteral("Rightarrow"), QStringLiteral("Leftarrow"),
		QStringLiteral("Leftrightarrow"), QStringLiteral("leftrightarrow"), QStringLiteral("mapsto"), QStringLiteral("to"),
		QStringLiteral("alpha"), QStringLiteral("beta"), QStringLiteral("gamma"), QStringLiteral("delta"),
		QStringLiteral("epsilon"), QStringLiteral("varepsilon"), QStringLiteral("zeta"), QStringLiteral("eta"),
		QStringLiteral("theta"), QStringLiteral("vartheta"), QStringLiteral("iota"), QStringLiteral("kappa"),
		QStringLiteral("lambda"), QStringLiteral("mu"), QStringLiteral("nu"), QStringLiteral("xi"),
		QStringLiteral("pi"), QStringLiteral("varpi"), QStringLiteral("rho"), QStringLiteral("varrho"),
		QStringLiteral("sigma"), QStringLiteral("varsigma"), QStringLiteral("tau"), QStringLiteral("upsilon"),
		QStringLiteral("phi"), QStringLiteral("varphi"), QStringLiteral("chi"), QStringLiteral("psi"), QStringLiteral("omega"),
		QStringLiteral("Gamma"), QStringLiteral("Delta"), QStringLiteral("Theta"), QStringLiteral("Lambda"),
		QStringLiteral("Xi"), QStringLiteral("Pi"), QStringLiteral("Sigma"), QStringLiteral("Upsilon"),
		QStringLiteral("Phi"), QStringLiteral("Psi"), QStringLiteral("Omega"),
	};
	return whitelist;
}

static std::wstring sanitizeUnknownCommands(const std::wstring &latex) {
	QString s = QString::fromStdWString(latex);
	static const QRegularExpression re(QStringLiteral("\\\\([A-Za-z]+)"));
	QString out;
	out.reserve(s.size());
	int last = 0;
	QRegularExpressionMatchIterator it = re.globalMatch(s);
	while(it.hasNext()) {
		QRegularExpressionMatch m = it.next();
		out += s.mid(last, m.capturedStart() - last);
		QString word = m.captured(1);
		out += latexCommandWhitelist().contains(word) ? m.captured(0) : (QStringLiteral("\\mathrm{") + word + QStringLiteral("}"));
		last = m.capturedEnd();
	}
	out += s.mid(last);
	return out.toStdWString();
}

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
			_render = LaTeX::parse(sanitizeUnknownCommands(latex), width() - _padding * 2, _text_size, _text_size / 3.f, argb(255, c.red(), c.green(), c.blue()));
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
