#ifndef PRODUCTVERSION_H
#define PRODUCTVERSION_H

#include <QString>
#include <QList>

#include "services/plugin/PluginAPI.h"

class ORIGIN_PLUGIN_API ProductVersion
{
public:
	ProductVersion();
	ProductVersion(const QString &version, const QString &sep = ".");

	bool isValid();
	friend bool operator>(ProductVersion &, ProductVersion &);
	friend bool operator<(ProductVersion &, ProductVersion &);
	friend bool operator==(ProductVersion &, ProductVersion &);
	friend bool operator>=(ProductVersion &, ProductVersion &);
	friend bool operator<=(ProductVersion &, ProductVersion &);

protected:
	friend QPair<ProductVersion, ProductVersion> padVersions(ProductVersion a, ProductVersion b);
	QList<unsigned int> mComponents;
};

#endif