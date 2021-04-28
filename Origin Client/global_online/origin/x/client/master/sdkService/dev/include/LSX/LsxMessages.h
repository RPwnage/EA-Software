#ifndef _LSXMESSAGES_H
#define _LSXMESSAGES_H

#include <NodeDocument.h>
#include <QDomDocument>
#include <QDomElement>
#include <QByteArray>
#include <QSharedPointer>

#include <LSX/LsxSocket.h>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class Connection;
            //
            //	Request
            //

            class Request
            {
            public:

                Request(const QByteArray &packet, Connection *connection, Protocol encoding);
                Request() : m_protocol(PROTOCOL_UNKNOWN), m_connection(NULL) {};
                ~Request();

                const QByteArray &recipient() const
                {
                    return m_recipient;
                }

                const QByteArray & id() const
                {
                    return m_id;
                }

                Protocol protocol() const
                {
                    return m_protocol;
                }

                INodeDocument * document()
                {
                    return m_document.data();
                }

                const INodeDocument * document() const
                {
                    return m_document.data();
                }

                bool valid() const
                {
                    return !m_document.isNull();
                }

                const QByteArray &command() const
                {
                    return m_command;
                }

                Connection *connection() const
                {
                    return m_connection;
                }

            private:
                QByteArray m_recipient;
                QByteArray m_id;
                QByteArray m_command;
                QByteArray m_msg;
                QSharedPointer<INodeDocument> m_document;
                Protocol m_protocol;

                Connection *m_connection;
            };


            //
            //	Response
            //
            class Response
            {
            public:
                enum ResponseType
                {
                    RESPONSETYPE_RESPONSE,
                    RESPONSETYPE_EVENT
                };

                bool isPending() { return m_bPending; }

                Lsx::Connection * connection() const { return m_connection; }
                void setConnection(Lsx::Connection * connection) { m_connection = connection; }

                void send();

                // Used for creating explicit responses to requests
                // This passes the request ID through
                Response(const Request &request);

                // Use this constructor if you already have a response object, but you want to create a new response to replace it.
                Response(const Response &response);

                // Creates an empty response of the given type
                Response(ResponseType type, Protocol protocol = PROTOCOL_XML, Lsx::Connection * connection = NULL);
                ~Response();

                // Set the sender for an event, be sure to set this before writing the body.
                void setSender(const QByteArray &sender);

                QByteArray toByteArray() const
                {
                    m_document->Root();
                    return m_document->Print();
                }

                INodeDocument * document()
                {
                    // The only time we need the document is when we write a response back into the document.
                    m_bPending = false;
                    return m_document.data();
                }


                void addRef() { m_ref++; }
                void release() { m_ref--; if(m_ref == 0) delete this; }

            private:
                void init(ResponseType type);

                QSharedPointer<INodeDocument> m_document;
                Protocol m_protocol;
                QByteArray m_id;
                QByteArray m_sender;
                Lsx::Connection * m_connection;
                bool m_bPending;
                bool m_bSent;
                int m_ref;
            };
        }

    }
}

#endif
