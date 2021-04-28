///////////////////////////////////////////////////////////////////////////////
// ErrorHandler.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ErrorHandler_h
#define ErrorHandler_h

#include <QStringList>

class QString;

class ErrorHandler
{
public:
	ErrorHandler();
	~ErrorHandler();
	void addError(QString const& error);
	bool isOk();
	void clear();
	// returns the first error received by this instance
	QString getError();
	// returns all the errors
	QStringList const& getAllErrors();
private:
	QStringList m_errors;
};

#endif
