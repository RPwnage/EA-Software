///////////////////////////////////////////////////////////////////////////////
// ErrorHandler.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "ErrorHandler.h"
#include <QString>

ErrorHandler::ErrorHandler(void)
{
}

ErrorHandler::~ErrorHandler(void)
{
}

void ErrorHandler::addError(QString const& error)
{
	m_errors << error;
}

bool ErrorHandler::isOk()
{
	return m_errors.isEmpty();
}

QString ErrorHandler::getError()
{
	return isOk() ? "" : m_errors[0];
}

QStringList const& ErrorHandler::getAllErrors()
{
	return m_errors;
}

void ErrorHandler::clear()
{
	m_errors.clear();
}
