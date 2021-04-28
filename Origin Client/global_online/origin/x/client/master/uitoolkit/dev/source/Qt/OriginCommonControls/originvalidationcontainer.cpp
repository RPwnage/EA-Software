#include "originvalidationcontainer.h"
#include <QtWidgets/QSpacerItem>
#include <QtCore/QTimer>
#include <QtWidgets/QStackedLayout>

#include "originradiobutton.h"
#include "origincheckbutton.h"
#include "originsinglelineedit.h"
#include "originmultilineedit.h"
#include "originspinner.h"
#include "originlabel.h"
#include "ui_originvalidationcontainer.h"

namespace Origin {
    namespace UIToolkit {
    
OriginValidationContainer::OriginValidationContainer(QWidget* parent) :
 QWidget(parent)
, ui(new Ui::OriginValidationContainer)
, mStatus(Normal)
, mNormalDescription(QString(""))
, mInvalidDescription(QString(""))
, mValidDescription(QString(""))
, mInvalidItem(NULL)
{
     ui->setupUi(this);
     mLeftStatusSpacer = new QSpacerItem(7, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

     mIconStartSpacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
     ui->iconLayout->addSpacerItem(mIconStartSpacer);
     mStackedLayout = new QStackedLayout();
     ui->iconLayout->addLayout(mStackedLayout);
     mIconEndSpacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
     ui->iconLayout->addSpacerItem(mIconEndSpacer);
     mIcon = new QLabel("");
     mIcon->setPixmap(QPixmap());
     mIcon->setMaximumSize(21, 21);
     mSpinner = new Origin::UIToolkit::OriginSpinner();
     mSpinner->setSpinnerType(Origin::UIToolkit::OriginSpinner::SpinnerSmallValidating);
     mSpinner->stopSpinner();
     mSpinner->setMaximumSize(21, 21);
     mStackedLayout->insertWidget(0, mIcon);
     mStackedLayout->insertWidget(1, mSpinner);

     setFieldName("fieldName");
     setParts(PartsAll);
     setValidationStatus(Normal);
 }

 OriginValidationContainer::~OriginValidationContainer()
 {
     // delete the items that could be just floating around
     if (ui->iconLayout->itemAt(0) != mIconStartSpacer)
     {
         delete mIconStartSpacer;
     }
     if (ui->iconLayout->itemAt(ui->iconLayout->count() - 1) != mIconEndSpacer) 
     {
         delete mIconEndSpacer;
     }
     if (ui->statusLayout->itemAt(0) != mLeftStatusSpacer) 
     {
        delete mLeftStatusSpacer;
     }
 }

 /*
QFrame* OriginValidationContainer::content()
{
    return ui->content;
}
*/
OriginValidationContainer::OriginValidationContainerList OriginValidationContainer::getWidgets()
{
    return mWidgetList;
}
void OriginValidationContainer::setFieldName(const QString &str)
{
    ui->fieldName->setText(str);
}

QString OriginValidationContainer::getFieldName() const
{
    return ui->fieldName->text();
}

void OriginValidationContainer::setValidationStatus(const OriginValidationStatus& status)
{
    bool showIcon = ((mParts & PartsIcon) ||
        (mParts & PartsAll));

    mStatus = status;

    switch (mStatus)
    {
        case Normal:
            {
                ui->statusLabel->setText(mNormalDescription);
                ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Normal);
                mSpinner->stopSpinner();
                mIcon->setPixmap(QPixmap());
                mStackedLayout->setCurrentIndex(0);
            }
            break;
        case Invalid:
            {
                ui->statusLabel->setText(mInvalidDescription);
                ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Invalid);
                mSpinner->stopSpinner();
                if (showIcon)
                    mIcon->setPixmap(QPixmap(":/OriginCommonControls/OriginMessageBox/error.png"));
                else 
                    mIcon->setPixmap(QPixmap());
                mStackedLayout->setCurrentIndex(0);
            }
            break;
        case Valid:
            {
                ui->statusLabel->setText(mValidDescription);
                ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Valid);
                mSpinner->stopSpinner();
                if (showIcon)
                    mIcon->setPixmap(QPixmap(":/OriginCommonControls/OriginMessageBox/validated.png"));
                else 
                    mIcon->setPixmap(QPixmap());
                mStackedLayout->setCurrentIndex(0);
            }
            break;
        case Validating:
            {
                ui->statusLabel->setText(mValidatingDescription);
                ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Validating);
                mIcon->setPixmap(QPixmap());
                if (showIcon)
                {
                    mSpinner->startSpinner();
                    mStackedLayout->setCurrentIndex(1);
                }
            }
            break;
    };
}

OriginValidationContainer::OriginValidationStatus OriginValidationContainer::getValidationStatus() const
{
    return mStatus;
}

void OriginValidationContainer::setInvalidItem(QWidget* widget)
{
    if (mInvalidItem && mInvalidItem != widget) 
    {
        setInvalidHelper(mInvalidItem, false);
        mInvalidItem = NULL;
    }

    if (widget == NULL) {
        return;
    }

    for (int i = 0; i < mWidgetList.size(); ++i) 
    {
        if (mInvalidItem == mWidgetList.at(i))
        {
            setInvalidHelper(mWidgetList.at(i), true);
            break;
        }
    }   

}

QWidget* OriginValidationContainer::getInvalidItem() const
{
    return mInvalidItem;
}

void OriginValidationContainer::addWidget(QWidget* w)
{
    ui->gridLayout->removeItem(ui->iconLayout);
    ui->gridLayout->removeItem(ui->statusLayout);

    ui->gridLayout->addWidget(w, ui->gridLayout->rowCount(), 0);

    ui->gridLayout->addLayout(ui->iconLayout, ui->gridLayout->rowCount() - 1, 1);
    ui->gridLayout->addLayout(ui->statusLayout, ui->gridLayout->rowCount(), 0, 1, 1);

    // if it is a radio button or a check button, add it to the button box group
    OriginRadioButton* rb = dynamic_cast<OriginRadioButton *>(w);
    Origin::UIToolkit::OriginCheckButton* cb = dynamic_cast<Origin::UIToolkit::OriginCheckButton *>(w);
    if (rb)

    {
        mButtonGroup.addButton(rb);
    }
    else if (cb)
    {
        mButtonGroup.addButton(cb);
    }

    mWidgetList.append(w);
    adjustSize();
}

void OriginValidationContainer::removeWidget(QWidget* w)
{
    ui->gridLayout->removeWidget(w);
    mWidgetList.removeOne(w);

    // if it is a radio button or a check button, remove it from the button group
    OriginRadioButton* rb = dynamic_cast<OriginRadioButton *>(w);
    Origin::UIToolkit::OriginCheckButton* cb = dynamic_cast<Origin::UIToolkit::OriginCheckButton *>(w);
    if (rb || cb)
    {
        mButtonGroup.removeButton(dynamic_cast<QAbstractButton *>(w));
    }

    adjustSize();
}


void OriginValidationContainer::setNormalDescription(const QString& str)
{
    mNormalDescription = str;
    if (mStatus == Normal) {
        ui->statusLabel->setText(str);
        ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Normal);
    }
}

QString OriginValidationContainer::getNormalDescription() const
{
    return mNormalDescription;
}

void OriginValidationContainer::setValidDescription(const QString& str)
{
    mValidDescription = str;
    if (mStatus == Valid) {
        ui->statusLabel->setText(str);
        ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Valid);
    }
}

QString OriginValidationContainer::getValidDescription() const
{
    return mValidDescription;
}

void OriginValidationContainer::setValidatingDescription(const QString& str)
{
    mValidatingDescription = str;
    if (mStatus == Validating) {
        ui->statusLabel->setText(str);
        ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Validating);
    }
}

QString OriginValidationContainer::getValidatingDescription() const
{
    return mValidatingDescription;
}

void OriginValidationContainer::setInvalidDescription(const QString& str)
{
    mInvalidDescription = str;
    if (mStatus == Invalid) 
    {
        ui->statusLabel->setText(str);
        ui->statusLabel->setValidationStatus(Origin::UIToolkit::OriginLabel::Invalid);
    }
}

QString OriginValidationContainer::getInvalidDescription() const
{
    return mInvalidDescription;
}

void OriginValidationContainer::setContainerType(const OriginValidationContainerType& containerType)
{
    mContainerType = containerType;
    mFieldNameAlignment = AlignmentStart;
    ui->fieldName->setAlignment(Qt::AlignLeft);

    switch (mContainerType)
    {
    case ContainerTypeCheckButtonGroup:
    case ContainerTypeRadioButtonGroup:
        {
            mStatusAlignment = AlignmentStart;
            ui->statusLabel->setAlignment(Qt::AlignLeft);
            if (ui->statusLayout->itemAt(0) != mLeftStatusSpacer) 
            {
                ui->statusLayout->insertSpacerItem(0, mLeftStatusSpacer);
            }
            break;
        }
    case ContainerTypeSingleLineEdit:
    case ContainerTypeDropDown:
    default:
        {
            mStatusAlignment = AlignmentEnd;
            ui->statusLabel->setAlignment(Qt::AlignRight);
            if (ui->statusLayout->itemAt(0) == mLeftStatusSpacer) 
            {
                ui->statusLayout->removeItem(mLeftStatusSpacer);
            }
            break;
        }
    };

    if (mContainerType == ContainerTypeRadioButtonGroup)
        mButtonGroup.setExclusive(true);
    else 
        mButtonGroup.setExclusive(false);
}

OriginValidationContainer::OriginValidationContainerType OriginValidationContainer::getContainerType() const
{
    return mContainerType;
}

void OriginValidationContainer::setParts(const int& flags)
{
    mParts = flags;
    
    ui->fieldName->setVisible((mParts & PartsAll) || (mParts & PartsFieldName));
    ui->statusLabel->setVisible((mParts & PartsAll) || (mParts & PartsStatusLabel));

    // I could make the logic into one line but I think this is easier to understand
    if ((mParts & PartsIcon) || (mParts & PartsAll)) 
    {
        if (mStatus == Validating) 
        {
            mSpinner->startSpinner();
            mStackedLayout->setCurrentIndex(1);
        }
        else 
        {
            if (mStatus == Invalid)
                mIcon->setPixmap(QPixmap(":/OriginCommonControls/OriginMessageBox/error.png"));
            else if (mStatus == Valid) 
                mIcon->setPixmap(QPixmap(":/OriginCommonControls/OriginMessageBox/validated.png"));
            mStackedLayout->setCurrentIndex(0);
        }
    }
    else
    {
        mIcon->setPixmap(QPixmap());
        mStackedLayout->setCurrentIndex(0);
    }
}
int OriginValidationContainer::getParts() const
{
    return mParts;
}
/// *** protected functions ****
bool OriginValidationContainer::event(QEvent *e)
{
    if(e->type() == QEvent::ChildPolished)
    {
        QChildEvent *cE = (QChildEvent*)e;
        QWidget* w = dynamic_cast<QWidget *>(cE->child());
        Origin::UIToolkit::OriginCheckButton* cb = dynamic_cast<Origin::UIToolkit::OriginCheckButton *>(w);
        OriginRadioButton* rb = dynamic_cast<OriginRadioButton *>(w);
        OriginSingleLineEdit* sedit = dynamic_cast<OriginSingleLineEdit *>(w);
        OriginMultiLineEdit*  medit = dynamic_cast<OriginMultiLineEdit *>(w);
        OriginValidationContainer* vc = dynamic_cast<OriginValidationContainer *>(w);
        if (cb || rb || sedit || medit || vc) 
        {
            addWidget(w);
        }
    }

    return QWidget::event(e);
}

/// *** private functions ****

QString&  OriginValidationContainer::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

void OriginValidationContainer::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
void OriginValidationContainer::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}
QString& OriginValidationContainer::getCurrentLang()
{
    return OriginUIBase::getCurrentLang();
}

void OriginValidationContainer::setInvalidHelper(QWidget* w, bool invalid)
{
    Q_ASSERT(w);

    /*
    ValidationState vs = Invalid;
    if (!invalid)
        vs = Valid;
        */
    // only labels, checkbuttons, and radio buttons right now (and not even all of them...)
    Origin::UIToolkit::OriginCheckButton* cb = dynamic_cast<Origin::UIToolkit::OriginCheckButton *>(w);
    if (cb) 
    {
        //cb->setValidationState(vs)
        return;
    }
    OriginRadioButton* rb = dynamic_cast<OriginRadioButton *>(w);
    if (rb)
    {
        //rb->setValidationState(vs);
        return;
    }
    OriginSingleLineEdit* sedit = dynamic_cast<OriginSingleLineEdit *>(w);
    OriginMultiLineEdit*  medit = dynamic_cast<OriginMultiLineEdit *>(w);
    if (sedit || medit)
    {
        //elabel->setValidationState(vs);
        return;
    }
}

}
}