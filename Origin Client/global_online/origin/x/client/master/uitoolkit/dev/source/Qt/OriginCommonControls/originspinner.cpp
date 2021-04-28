#include "originspinner.h"
#include <QtGui/QMovie>

namespace Origin {
    namespace UIToolkit {
QString OriginSpinner::mPrivateStyleSheet = QString("");

OriginSpinner::OriginSpinner(QWidget *parent/*= 0*/) :
        QLabel(parent),
        mSpinnerType(SpinnerLarge),
        autoStart(false),
        mSpinner(NULL)
{
    m_children.append(this);

    setAttribute(Qt::WA_TranslucentBackground);

    mSpinnerMovies.insert(SpinnerSmall, ":/OriginCommonControls/OriginSpinner/small.gif");
    mSpinnerMovies.insert(SpinnerMedium, ":/OriginCommonControls/OriginSpinner/medium.gif");
    mSpinnerMovies.insert(SpinnerLarge, ":/OriginCommonControls/OriginSpinner/large.mng");
    mSpinnerMovies.insert(SpinnerLarge_Dark, ":/OriginCommonControls/OriginSpinner/large_dark.gif");
    mSpinnerMovies.insert(SpinnerSmallValidating, ":/OriginCommonControls/OriginSpinner/smallvalidating.gif");

    setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    setSpinnerType(SpinnerLarge);
}

OriginSpinner::~OriginSpinner()
{ 
    this->removeElementFromChildList(this);
    // the label does not assume ownership of the movie and therefore needs to be removed.
    // see QLabel::setMovie documentation
    if (mSpinner) {
        mSpinner->stop();
        delete mSpinner;
    }
}

QString & OriginSpinner::getElementStyleSheet()
{
    if (mPrivateStyleSheet.isEmpty() && !m_styleSheetPath.isEmpty())
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


bool OriginSpinner::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QLabel::event(event);
}


void OriginSpinner::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QLabel::paintEvent(event);
}


void OriginSpinner::setSpinnerType(SpinnerType spinnerType)
{
    mSpinnerType = spinnerType;
    switch (mSpinnerType)
    {
    case SpinnerSmall:
        setPixmap(QPixmap(":/OriginCommonControls/OriginSpinner/small.gif"));
        setMinimumSize(16,16);
        break;

    case SpinnerMedium:
        setPixmap(QPixmap(":/OriginCommonControls/OriginSpinner/medium.gif"));
        setMinimumSize(30,30);
        break;

    case SpinnerSmallValidating:
        setPixmap(QPixmap(":/OriginCommonControls/OriginSpinner/smallvalidating.gif"));
        setMinimumSize(16,16);
        break;

    case SpinnerLarge_Dark:
        setPixmap(QPixmap(":/OriginCommonControls/OriginSpinner/large_dark.gif"));
        setMinimumSize(16,16);
        break;

    default:
    case SpinnerLarge:
        setPixmap(QPixmap(":/OriginCommonControls/OriginSpinner/large.mng"));
        setMinimumSize(65,65);
        break;
    }
}


void OriginSpinner::startSpinner()
{
    if (mSpinner == NULL)
        createSpinner();
    else
        mSpinner->setFileName(mSpinnerMovies[mSpinnerType]);
    show();
    mSpinner->start();
}


void OriginSpinner::stopSpinner()
{
    hide();
    if(mSpinner)
        mSpinner->stop();
}


void OriginSpinner::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


void OriginSpinner::createSpinner()
{
    // Create our spinner movie with the default (large) size
    mSpinner = new QMovie(mSpinnerMovies[mSpinnerType], QByteArray(), this);
    setMovie(mSpinner);
}

void OriginSpinner::setAutoStart(bool aAutoStart)
{
    autoStart = aAutoStart;
    if (autoStart)
        startSpinner();
    else
        stopSpinner();
}
}
}
