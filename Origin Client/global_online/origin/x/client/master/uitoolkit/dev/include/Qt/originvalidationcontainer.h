#ifndef ORIGINVALIDATIONCONTAINER_H
#define ORIGINVALIDATIONCONTAINER_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>
#include <QtGui/QPixmap>
#include <QtWidgets/QButtonGroup>
#include "originuiBase.h"
#include "originspinner.h"
#include "uitoolkitpluginapi.h"

class QSpacerItem;
class QStackedLayout;

namespace Ui {
    class OriginValidationContainer;
}

namespace Origin {
    namespace UIToolkit {
/// 
/// Public Widget that allows for an item to be contained within it. The outside shell 
/// allows for a title ("FieldName"), a status string ("Description"), and a static icon
///
class UITOOLKIT_PLUGIN_API OriginValidationContainer : public QWidget, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(OriginValidationStatus)
    Q_FLAGS(OriginValidationContainerParts)
//    Q_ENUMS(OriginValidationContainerAlignment)
    Q_ENUMS(OriginValidationContainerType)
    Q_PROPERTY(OriginValidationContainerType containerType READ getContainerType WRITE setContainerType DESIGNABLE true);
    Q_PROPERTY(QString fieldName READ getFieldName WRITE setFieldName DESIGNABLE true);
    Q_PROPERTY(OriginValidationStatus validationStatus READ getValidationStatus WRITE setValidationStatus DESIGNABLE true);
    Q_PROPERTY(QString normalDescription READ getNormalDescription WRITE setNormalDescription DESIGNABLE true);
    Q_PROPERTY(QString validDescription READ getValidDescription WRITE setValidDescription DESIGNABLE true);
    Q_PROPERTY(QString validatingDescription READ getValidatingDescription WRITE setValidatingDescription DESIGNABLE true);
    Q_PROPERTY(QString invalidDescription READ getInvalidDescription WRITE setInvalidDescription DESIGNABLE true)
    Q_PROPERTY(OriginValidationContainerParts parts READ getParts WRITE setParts DESIGNABLE true);
    /*
    Q_PROPERTY(OriginValidationContainerAlignment FieldNameAlignment READ fieldNameAlignment WRITE setFieldNameAlignment);
    Q_PROPERTY(OriginValidationContainerAlignment IconAlignment READ iconAlignment WRITE setIconAlignment);
    Q_PROPERTY(OriginValidationContainerAlignment StatusAlignment READ statusAlignment WRITE setStatusAlignment);
    */


public:
    /// 
    /// Enum that controls what the validation setting should be. This cannot be moved to a lower file
    /// because then the Q_ENUM and Q_PROPERTY do not work so instead I have included it here

#include "originvalidationstatus.h"


    ///
    /// Enum to indicate the type of controller it is
    /// 
    enum OriginValidationContainerType
    {
        ContainerTypeSingleLineEdit = 0,
        ContainerTypeRadioButtonGroup,
        ContainerTypeCheckButtonGroup,
        ContainerTypeDropDown // todo
    };
    /// 
    /// Enum that controls parts of the container should be shown - these are put together as a flag
    /// PartsNone: don't show anything
    /// PartsFieldName: show the field name
    /// PartsIcon: show the icon if applicable (there is no icon in the "Normal" status and a spinner in the "Validating" status)
    /// PartsStatusLabel: shows the status label
    /// PartsAll: shows the FieldName, Icon, StatusLabel
    ///
    enum OriginValidationContainerParts
    {
        PartsNone = 0x00,
        PartsFieldName = 0x01,
        PartsIcon = 0x02,
        PartsStatusLabel = 0x04,
        PartsAll = 0x08
    };

    ///
    /// Enum that controls what the alignment of an item would be
    ///
    enum OriginValidationContainerAlignment
    {
        AlignmentStart,
        AlignmentCenter,
        AlignmentEnd,
    };

    typedef QList<QWidget *> OriginValidationContainerList;

    /// 
    /// Constructor
    ///
    OriginValidationContainer(QWidget* parent = NULL);
    
    ///
    /// Destructor
    /// 
    ~OriginValidationContainer();

    /// 
    /// Returns the frame widget that will contain the individual widgets
    /// for this container
    ///
  //  QFrame* content();
 
    /// 
    /// sets the FieldName to the string (should be in final translated form)
    ///
    void setFieldName(const QString& str);
    /// 
    /// returns the FieldName string
    /// 
    QString getFieldName() const;

    /// 
    /// sets the validation status for this container. It will put up the appropriate
    /// icon and set the status label to the appropriate string
    ///
    void setValidationStatus(const OriginValidationStatus& status);
    ///
    /// returns the current status value
    /// 
    OriginValidationStatus getValidationStatus() const;

    ///
    /// sets the status of the widget to be invalid. Resets the state of the previously
    /// invalid widget (if any) to not be in an error state.
    ///
    void setInvalidItem(QWidget* widget);
    /// 
    /// returns the value of the invalid item. Returns NULL if nothing is currently invalid.
    ///
    QWidget* getInvalidItem() const;

    ///
    /// returns the list of widgets held in the container
    ///
    OriginValidationContainerList getWidgets();

    ///
    /// sets the values of the string that should appear when the container is in
    /// the "Normal" status. An empty string means that nothing should appear in this status state
    /// 
    /// \param str  Normal Description string
    ///
    ///
    void setNormalDescription(const QString& str);
    /// 
    /// returns the description string that is displayed when the container is in a 
    /// "Normal" state. It is possible to return an empty string.
    ///
    QString getNormalDescription() const;

    ///
    /// sets the values of the string that should appear when the container is in
    /// the "Valid" status. An empty string means that nothing should appear in this status state
    /// 
    /// \param str  Valid Description string
    ///
    ///
    void setValidDescription(const QString& str);
    /// 
    /// returns the description string that is displayed when the container is in a 
    /// "Valid" state. It is possible to return an empty string.
    ///
    QString getValidDescription() const;

    ///
    /// sets the values of the string that should appear when the container is in
    /// the "Validating" status. An empty string means that nothing should appear in this status state
    /// 
    /// \param str  Validating Description string
    ///
    ///
    void setValidatingDescription(const QString& str);
    /// 
    /// returns the description string that is displayed when the container is in a 
    /// "Valid" state. It is possible to return an empty string.
    ///
    QString getValidatingDescription() const;

    ///
    /// sets the values of the string that should appear when the container is in
    /// the "Invalid" status. An empty string means that nothing should appear in this status state
    /// 
    /// \param str  Invalid Description string
    ///
    void setInvalidDescription(const QString& str);
    /// 
    /// returns the description string that is displayed when the container is in a 
    /// "Invalid" state. It is possible to return an empty string.
    ///
    QString getInvalidDescription() const;

    /// 
    /// sets which parts are to be shown. It uses the OriginValidationContainerParts enum
    /// as bitwise flags. The flag is a combination of the enum
    /// 
    /// \param flags  Can be: PartsNone, PartsAll, or a combination of PartsFieldName, PartsIcon, PartsStatusLabel
    ///               for example: PartsFieldName | PartsStatusLabel to not show the Icon
    /// 
    void setParts(const int& flags);
    /// 
    /// returns the flags in an integer
    ///
    int getParts() const;
    /*
    /// 
    /// sets the alignment of where the FieldName should appear
    /// 
    /// \param alignment whether the field name should be left (AlignmentStart), center (AlignmentCenter), right (AlignmentEnd)
    /// 
    ///
    void setFieldNameAlignment(const OriginValidationContainerAlignment& alignment);
    /// 
    /// returns the current FieldName Alignment
    ///
    OriginValidationContainerAlignment fieldNameAlignment();

    /// 
    /// sets the alignment of where the Icon should appear
    /// 
    /// \param alignment whether the icon should be top (AlignmentStart), center (AlignmentCenter), bottom (AlignmentEnd)
    /// 
    ///
    void setIconAlignment(const OriginValidationContainerAlignment& alignment);
    /// 
    /// returns the current Icon Alignment
    /// 
    OriginValidationContainerAlignment iconAlignment();

    /// 
    /// sets the alignment of where the StatusLabel should appear
    /// 
    /// \param alignment whether the StatusLabel should be left (AlignmentStart), center (AlignmentCenter), right (AlignmentEnd)
    /// 
    ///
    void setStatusAlignment(const OriginValidationContainerAlignment& alignment);
    /// 
    /// returns the current alignment of the status label
    ///
    OriginValidationContainerAlignment statusAlignment();
    */
    /// 
    /// add a widget to the container area
    /// 
    /// \param w widget to add
    ///
    void addWidget(QWidget* w);
    /// 
    /// remove a widget to the container area
    /// 
    /// \param w widget to remove
    ///
    void removeWidget(QWidget* w);

    OriginValidationContainerType getContainerType() const;
    void setContainerType(const OriginValidationContainerType& containerType);

protected:
    bool event(QEvent *);

private slots:

private:
    QWidget* getSelf() {return this;}
    QString&  getElementStyleSheet();
    void prepForStyleReload();
    void changeLang(QString newLang);
    QString& getCurrentLang();

    /// 
    /// common helper method for setting a widget either invalid or valid
    /// 
    /// \param w - widget to set invalid/valid
    /// \param invalid - whether to set it to invalid or valid
    /// 
    void setInvalidHelper(QWidget* w, bool invalid = true);

    Ui::OriginValidationContainer* ui; // vertical layout container for check buttons, error text label
    OriginValidationStatus mStatus;
    OriginValidationContainerList mWidgetList;
    QString mNormalDescription;
    QString mInvalidDescription;
    QString mValidDescription;
    QString mValidatingDescription;
    int mParts;
    OriginValidationContainerAlignment mFieldNameAlignment;
    OriginValidationContainerAlignment mIconAlignment;
    OriginValidationContainerAlignment mStatusAlignment;
    OriginValidationContainerType mContainerType;

    QString mPrivateStyleSheet;

    QSpacerItem* mIconStartSpacer;
    QSpacerItem* mIconEndSpacer;
    QSpacerItem* mLeftStatusSpacer;
    QButtonGroup mButtonGroup;

    QWidget* mInvalidItem;
    Origin::UIToolkit::OriginSpinner* mSpinner;
    QLabel* mIcon;
    QStackedLayout* mStackedLayout;

};
}
}

#endif
