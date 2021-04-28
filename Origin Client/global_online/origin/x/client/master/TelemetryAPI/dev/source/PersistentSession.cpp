#include "PersistentSession.h"

#include "EAStdC/EADateTime.h"
#include "EAStdC/EARandom.h"


PersistentSession::PersistentSession( const char8_t* productId )
: m_numericSessionId(0)
, m_startTimeUTC( (uint32_t) (EA::StdC::GetTime() / UINT64_C(1000000000)) ) 
, m_productId( productId )
{
  EA::StdC::GetRandomSeed(&m_numericSessionId, sizeof(m_numericSessionId));
}

PersistentSession::~PersistentSession()
{
}

uint64_t PersistentSession::startTimeUTC() const
{
  return m_startTimeUTC;
}

uint32_t PersistentSession::sessionLength() const
{
  uint32_t persistentSessionLength = (uint32_t) ((EA::StdC::GetTime() / UINT64_C(1000000000)) - m_startTimeUTC );
  
  return persistentSessionLength;
}

uint64_t PersistentSession::sessionId() const
{
  return m_numericSessionId;
}

const char8_t* PersistentSession::productId() const
{
  return m_productId.c_str();
}
