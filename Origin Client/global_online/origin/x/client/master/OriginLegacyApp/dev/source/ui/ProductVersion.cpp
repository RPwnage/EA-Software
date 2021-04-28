#include <QStringList>
#include <QPair>

#include "services/debug/DebugService.h"
#include "ProductVersion.h"

ProductVersion::ProductVersion()
{
}

ProductVersion::ProductVersion(const QString &version, const QString &sep)
{
	QStringList versionParts = version.split(sep);

	for(QStringList::const_iterator it = versionParts.begin();
		it != versionParts.end();
		it++)
	{
		bool ok;
		unsigned int component = (*it).toUInt(&ok);

		// Make sure we could parse
		ORIGIN_ASSERT(ok);
		Q_UNUSED(ok);

		mComponents.push_back(component);
	}
}

bool ProductVersion::isValid()
{
	return !mComponents.empty();
}

QPair<ProductVersion, ProductVersion> padVersions(ProductVersion a, ProductVersion b)
{
	while(a.mComponents.length() > b.mComponents.length())
	{
		b.mComponents.push_back(0);
	}

	while(b.mComponents.length() > a.mComponents.length())
	{
		a.mComponents.push_back(0);
	}

	return QPair<ProductVersion, ProductVersion>(a, b);
}

bool operator>(ProductVersion &rawA, ProductVersion &rawB)
{
	QPair<ProductVersion, ProductVersion> padded = padVersions(rawA, rawB);
	ProductVersion a = padded.first;
	ProductVersion b = padded.second;

	for(int i = 0; i < a.mComponents.length(); i++)
	{
		if (a.mComponents[i] > b.mComponents[i])
		{
			return true;
		}
		if (a.mComponents[i] < b.mComponents[i])
		{
			return false;
		}
	}

	// Equal
	return false;
}

bool operator<(ProductVersion &rawA, ProductVersion &rawB)
{
	QPair<ProductVersion, ProductVersion> padded = padVersions(rawA, rawB);
	ProductVersion a = padded.first;
	ProductVersion b = padded.second;

	for(int i = 0; i < a.mComponents.length(); i++)
	{
		if (a.mComponents[i] < b.mComponents[i])
		{
			return true;
		}
		if (a.mComponents[i] > b.mComponents[i])
		{
			return false;
		}
	}

	// Equal
	return false;
}

bool operator==(ProductVersion &rawA, ProductVersion &rawB)
{
	QPair<ProductVersion, ProductVersion> padded = padVersions(rawA, rawB);
	ProductVersion a = padded.first;
	ProductVersion b = padded.second;

	for(int i = 0; i < a.mComponents.length(); i++)
	{
		if (a.mComponents[i] != b.mComponents[i])
		{
			return false;
		}
	}

	// Equal
	return true;
}

bool operator<=(ProductVersion &rawA, ProductVersion &rawB)
{
	return (rawA < rawB) || (rawA == rawB);
}

bool operator>=(ProductVersion &rawA, ProductVersion &rawB)
{
	return (rawA > rawB) || (rawA == rawB);
}