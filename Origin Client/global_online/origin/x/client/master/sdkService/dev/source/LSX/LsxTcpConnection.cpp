#include <QTcpSocket>

#include "LSX/LsxTcpConnection.h"
#include "LSX/LsxServer.h"
#include "services/log/LogService.h"

#include <QDomDocument>
#include "EbisuError.h"

#include "lsx.h"
#include "lsxWriter.h"

#define MAX_LSX_MESSAGE_SIZE 128*1024

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            TcpConnection::TcpConnection(Server *server, LsxSocket *socket)
            {
                m_server	= server;
                m_socket	= socket;
            }

            void TcpConnection::sendResponse(const Response& response)
            {
                QByteArray responseBytes = response.toByteArray();

                if(responseBytes.size() < MAX_LSX_MESSAGE_SIZE || CompareSDKVersion(1,0,0,9) >= 0)
                {
                    // Perform our write in the correct thread
                    QMetaObject::invokeMethod(this, "socketWrite", Q_ARG(const QByteArray &, responseBytes));
                }
                else
                {
					// Create a new response based off the actual response.
					Response errorResponse(response);

                    ORIGIN_LOG_ERROR << "SDK message too big: " << responseBytes;

					lsx::ErrorSuccessT resp;

					resp.Code = EBISU_ERROR_SDK_INTERNAL_BUFFER_TOO_SMALL;
					resp.Description = "The Origin SDK Buffer is too small to hold the response.";

					lsx::Write(errorResponse.document(), resp);

                    QMetaObject::invokeMethod(this, "socketWrite", Q_ARG(const QByteArray &, errorResponse.toByteArray()));
                }
            }

            void TcpConnection::socketWrite(const QByteArray &data)
            {
                // Check that socket is valid before writing. Socket hangup occurs in another thread.
                // Is is possible that thread will close the socket before this thread completes the write.
                if (connected())
                {
                    m_socket->writeMessage(data);
                }
            }

			void TcpConnection::closeConnection()
			{
				if (m_socket != NULL)
				{
					LsxSocket * temp = m_socket;
					m_socket = NULL;
					temp->close();
				}
			}

		}
    }
}