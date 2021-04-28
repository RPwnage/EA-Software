#ifndef __ORIGIN_LSX_MESSAGE_H__
#define __ORIGIN_LSX_MESSAGE_H__

#include <stdlib.h>
#include "lsxReader.h"
#include "lsxWriter.h"
#include "RapidXmlParser.h"
#include "OriginSDKMemory.h"

template<typename T> class LSXMessage
{
public:
	/// \brief Use this function in client code. It will setup the proper sender/recipient settings depending on usage.
	///
	///  <LSX>
	///		<Type sender="Service">
	///			<T> ...
	///			</T>
	///		</Type>
	///  </LSX>
	LSXMessage<T>(const char *Type, const char *Service) :
		m_RequestId(0)
	{
		m_Type = Type;
		m_Service = Service ? Service : "";
	}

	/*/// \brief Use this function if you want more control over the message format. It will use the provided Designation in LSX 
	///
	///  <LSX>
	///		<Type Designation="Service">
	///			<T> ...
	///			</T>
	///		</Type>
	///  </LSX>
	LSXMessage<T>(const char *Type, const char *Designation, const char *Service) :
		m_RequestId(0)
	{
		m_Type = Type;
		m_Service = Service ? Service : "";
		m_Designation = Designation;
	}*/


	bool FromXml(INodeDocument *pDoc)
	{
		if(pDoc->Root())
		{
			if(strcmp(pDoc->GetName(), "LSX") == 0)
			{
				if(pDoc->FirstChild(m_Type.c_str()))
				{
					if(pDoc->GetAttribute("id"))
					{
						m_RequestId = atoi(pDoc->GetAttributeValue());
					}

					const char *pValue = pDoc->GetAttributeValue(m_Designation.empty() ? "sender" : m_Designation.c_str());
					if(pValue)
					{
						if(m_Service.empty() || strcmp(pValue, m_Service.c_str()) == 0)
						{
							if(pDoc->FirstChild(m_T.GetType()))
							{
								return Read(pDoc, m_T);
							}
						}
					}
				}
			}
		}
		return false;
	}

	const Origin::AllocString ToXml(uint64_t requestId)
	{
		RapidXmlParser doc;
		char Buffer[53];
		
		doc.AddChild("LSX");
		doc.AddChild(m_Type.c_str());

		doc.AddAttribute(m_Designation.empty() ? "recipient" : m_Designation.c_str(), m_Service.c_str());

        sprintf(Buffer, "%llu", requestId);

		doc.AddAttribute("id", Buffer);

		Write(&doc, m_T);
        char Buffer2[16];
#ifdef ORIGIN_PC
        _itoa_s(ORIGIN_SDK_VERSION, Buffer2, 16, 10);
#elif defined ORIGIN_MAC
        snprintf(Buffer2, 16, "%i", ORIGIN_SDK_VERSION);
#else
#error(Unsupported platform)
#endif
        if (doc.FirstChild())
        {
            doc.AddAttribute("version", Buffer2);
        }
		doc.Root();
		return doc.GetXml();
	}

	operator T &(){return m_T;}

	T * operator ->() {return &m_T;}

	int GetRequestId(){return m_RequestId;}
	
	const char *GetService(){return m_Service.c_str();}
	const char *GetType(){return m_Type.c_str();}

private:
	T m_T;
	Origin::AllocString m_Type;
	Origin::AllocString m_Service;
	Origin::AllocString m_Designation;
	int m_RequestId;
};


#endif
