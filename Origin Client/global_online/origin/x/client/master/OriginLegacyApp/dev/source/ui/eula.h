#ifndef EULA_H
#define EULA_H

#include <QString>
#include <QList>

struct Eula
{
	QString Description;
	QString InstallName;
    QString FileName;
    bool IsRtfFile;
    bool IsThirdParty;
	bool IsOptional;
	bool accepted;
	QString CancelWarning;
	QString ToolTip;

    Eula(const QString& description, const QString& fileName, bool isRtfFile, bool isThirdParty, bool isOptional = false)
    : Description(description)
    , FileName(fileName)
    , IsRtfFile(isRtfFile)
    , IsThirdParty(isThirdParty)
    , IsOptional (isOptional)
    , accepted (false)
    {
    }
    
	Eula(const QString& description, const QString& installName, const QString& fileName, bool isRtfFile, bool isThirdParty, bool isOptional, QString& cancelMessage, QString& toolTip)
    : Description(description)
    , InstallName(installName)
    , FileName(fileName)
    , IsRtfFile(isRtfFile)
    , IsThirdParty(isThirdParty)
    , IsOptional (isOptional)
    , accepted (false)
    , CancelWarning(cancelMessage)
    , ToolTip(toolTip)
    {
    }
};

struct EulaList
{
	bool isConsolidate;
	QString gameProductId;
	QString gameTitle;
	QString coreHandlerId;
	QString isAutoPatch;
	QList<Eula> listOfEulas;
};

struct EulaLangList
{
	QString coreHandlerId;
	QString gameProductId;
	QList<QString> listOfEulaLang;
};

#endif // EULA_H
