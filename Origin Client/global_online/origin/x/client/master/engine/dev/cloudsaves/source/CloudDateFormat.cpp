#include <QLocale>
#include "engine/cloudsaves/CloudDateFormat.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
	namespace CloudDateFormat
	{
		QString format(const QDateTime& datetime, const QLocale &locale)
		{
			QDateTime localDateTime = datetime.toLocalTime();
			return format(localDateTime.date(), locale) + " " + format(localDateTime.time(), locale);
		}
	
		QString format(const QTime& time, const QLocale &locale)
		{
			switch(locale.country())
			{
			case QLocale::UnitedStates:
				return time.toString("h:mmAP");
	
			case QLocale::Denmark:
			case QLocale::Sweden:
				return time.toString("HH.mm");
	
			case QLocale::Hungary:
				return time.toString("H.mm");
	
			case QLocale::UnitedKingdom:
			case QLocale::France:
			case QLocale::Germany:
			case QLocale::Italy:
			case QLocale::Norway:
			case QLocale::Poland:
				return time.toString("HH:mm");
	
			case QLocale::Netherlands:
			case QLocale::Brazil:
			case QLocale::Portugal:
			case QLocale::RussianFederation:
			case QLocale::Spain:
			case QLocale::Mexico:
			case QLocale::Finland:
			case QLocale::RepublicOfKorea:
			case QLocale::Japan:
			case QLocale::China:
			case QLocale::Taiwan:
			case QLocale::Greece:
				return time.toString("H:mm");
                
            default:
                return time.toString("H:mm");
			}
		}
	
		QString format(const QDate& date, const QLocale &locale)
		{
			QString month = QObject::trUtf8(QDate::longMonthName(date.month()).toUtf8());

			switch(locale.country())
			{
			case QLocale::Brazil:		
			case QLocale::Portugal:
				return date.toString("d 'de' %1 'de' yyyy").arg(month);
	
			case QLocale::Spain:			
			case QLocale::Mexico:		
				return date.toString("dd 'de' %1 'de' yyyy").arg(month);
	
			case QLocale::Germany:				
			case QLocale::Norway:				
			case QLocale::Denmark:				
				return date.toString("d'.' %1 yyyy").arg(month);

			case QLocale::Finland:				
				return date.toString("d'.' %1'ta' yyyy").arg(month);
	
			case QLocale::Sweden:				
				return date.toString("d %1 yyyy").arg(month);
	
			case QLocale::Italy:				
			case QLocale::Greece:					
				return date.toString("dd %1 yyyy").arg(month);
	
			case QLocale::France:
				return date.toString("d%1 %2 yyyy").arg((date.day() == 1) ? "er": "").arg(month);
	
			case QLocale::RussianFederation:
			case QLocale::Poland:	
				return date.toString("d  %1  yyyy 'r.'").arg(month);
	
			case QLocale::Hungary:				
				return date.toString("yyyy'.' %1 d'.'").arg(month);
			
			case QLocale::RepublicOfKorea:				
				return date.toString("yyyy'/'%1'/'d").arg(month);

			case QLocale::CzechRepublic:
			case QLocale::Slovak:
				return date.toString("d'.' %1 yyyy").arg(month);
	
			default:
				return date.toString("%1 d',' yyyy").arg(month);
			}
		}

	}
}

}
}