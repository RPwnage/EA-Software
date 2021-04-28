#include "PingServerService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include <QUrlQuery>
#include <QHostInfo>

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            ResponseInfo PingServerService::processRequest(QUrl requestUrl)
            {
                ResponseInfo responseInfo;

                responseInfo.response = "{\"resp\":\"ok\"}";
                responseInfo.errorCode = HttpStatus::OK;

                QUrlQuery queryParams(requestUrl.query());

                // parse ip addresses
                QString ip_string = queryParams.queryItemValue("ipAddrList", QUrl::FullyDecoded);

                mUrlQueue.append(ip_string);

                if (mRequestActive)   // is a request still active? if so, just return and leave it queued
                {
                    ORIGIN_LOG_DEBUG << "processRequest: request active " << mUrlQueue.join('|');
                    return responseInfo;
                }

                executePingRequest();

                return responseInfo;
            }

            bool parseIPString(QString address, int *values)
            {
                QStringList ip_addr_as_list = address.split(".");
                if (ip_addr_as_list.count() == 4)   // must split into 4 numbers
                {
                    // test values make sure they are in the range 0-255
                    for (int j = 0; j < 4; j++)
                    {
                        bool bIsInt = false;    // checks if string is an integer
                        int value = ip_addr_as_list[j].toInt(&bIsInt);

                        if ((value < 0) || (value > 255) || !bIsInt)
                        {
                            return false;
                        }

                        values[j] = value;
                    }

                    return true;
                }

                return false;
            }

            void PingServerService::executePingRequest()
            {
                QString ip_string;
                ResponseInfo responseInfo;

                // check if anything is in the queue
                if (mUrlQueue.isEmpty())
                {
                    mRequestActive = false;
                    return; // if not just return
                }
                mRequestActive = true;

                ip_string = mUrlQueue.takeFirst();  // pop the top request off the queue

                mNumIPs = 0;
                mIPResultsReturned = 0;
                mExtraInterval = false;

                int num_addresses = 0;
                if (!ip_string.isEmpty())
                {
                    QStringList ip_list = ip_string.split(",");

                    // make sure that all IPs are unique (SRM requirement suggested by DICE)
                    ip_list.removeDuplicates();

                    // just for debugging OSX
                    ORIGIN_LOG_EVENT << "IP list: " << ip_string;

                    num_addresses = ip_list.count();
                    if (num_addresses > MAX_PING_PER_REQUEST)
                        num_addresses = MAX_PING_PER_REQUEST;   // cap it

                    for (int i = 0; i < num_addresses; i++)
                    {
                        // parse individual ips
                        bool valid = true;
                        int values[4];
                        QString origIP = ip_list[i];    // preserve this in case it is an FQDN

                        valid = parseIPString(ip_list[i], values);

                        if (!valid) // if still not valid, check to see if it is a FQDN - this is very expensive so only do it when it is clear that the ip is not a valid raw ip
                        {
                            // resolve any FQDNs (ORPLAT-233)
                            // Perform the lookup
                            QHostInfo hostInfo(QHostInfo::fromName(ip_list[i]));

                            if (hostInfo.error() != QHostInfo::NoError)
                            {
                                ORIGIN_LOG_ERROR << "hostInfo.error -> " << hostInfo.error() << " for ip: " << ip_list[i];
                                continue;
                            }

                            valid = true;   // if we made it here, we should be valid again pending ip addr check
                            QList<QHostAddress> iplist = hostInfo.addresses();

                            QList<QHostAddress>::iterator it = iplist.begin();
                            if (it != iplist.end())
                            {
                                QHostAddress address = *it;

                                ip_list[i] = address.toString();  // just take the first one since we cap the pings at 10

                                valid = parseIPString(ip_list[i], values);
                            }

                            if (!valid)
                            {
                                ORIGIN_LOG_ERROR << "invalid IP encountered for ip: " << ip_list[i];
                                continue;
                            }
                        }

                        // convert to 32-bit ip address (111.222.333.444 -> 111 << 24 | 222 << 16 | 333 << 8 | 444)
                        mIPAddresses[mNumIPs] = (values[0] << 24) | (values[1] << 16) | (values[2] << 8) | values[3];
                        mIPAddrStrings[mNumIPs] = origIP;
                        mIPPingResults[mNumIPs] = -1;
                        mNumIPs++;
                    }
                }
                else
                {
                    ORIGIN_LOG_WARNING << "IP list is empty";
                }

                if (mNumIPs == 0)
                {
                    responseInfo.response = "{\"error\":\"NO_VALID_ADDRESSES\"}";
                    responseInfo.errorCode = HttpStatus::OK;
                    emit processCompleted(responseInfo);

                    mRequestActive = false;
                    return;
                }

                int result;
                mProtoPingRef = ProtoPingCreate(0);
                if (mProtoPingRef)
                {
                    DirtyAddrT addr;
                    static unsigned char databuf[256] = "ping";
                    int databuf_length = 256;

                    ProtoPingControl(mProtoPingRef, 'type', PROTOPING_TYPE_ICMP, NULL); // set protocol to ICMP
                    for (int i = 0; i < mNumIPs; i++)
                    {
                        DirtyAddrFromHostAddr(&addr, &mIPAddresses[i]);
                        result = ProtoPingRequest(mProtoPingRef, &addr, databuf, databuf_length, 1500, 0);
                    }
                }

                ORIGIN_VERIFY_CONNECT(&mPollingTimer, SIGNAL(timeout()), this, SLOT(onTimerInterval()));
                mPollingTimer.start(1000);
            }

            void PingServerService::onTimerInterval()
            {
                mPollingTimer.stop();
                ORIGIN_VERIFY_DISCONNECT(&mPollingTimer, SIGNAL(timeout()), this, SLOT(onTimerInterval()));

                ProtoPingResponseT response;

                int result = -1;

                if (mProtoPingRef)
                {
                    unsigned char databuf[256];
                    int databuf_length = 255;

                    while (result != 0)
                    {
                        result = ProtoPingResponse(mProtoPingRef, databuf, &databuf_length, &response);
                        if (result != 0)
                            mIPResultsReturned++;
                        if (result > 0)
                        {
                            // find a matching 32-bit ip
                            for (int i = 0; i < mNumIPs; i++)
                            {
                                if (response.uAddr == mIPAddresses[i])
                                {
                                    mIPPingResults[i] = response.iPing;

                                    ORIGIN_LOG_DEBUG << "ping: " << mIPAddrStrings[i] << ":" << response.iPing;
                                }
                            }
                        }
                    }

                    if (!mExtraInterval)	// first time through?
                    {
                        if (mIPResultsReturned == mNumIPs)	// all responses accounted for
                        {
                            ProtoPingDestroy(mProtoPingRef);
                            mProtoPingRef = NULL;
                        }
                        else
                        {
                            mExtraInterval = true;	// wait another second
                            ORIGIN_VERIFY_CONNECT(&mPollingTimer, SIGNAL(timeout()), this, SLOT(onTimerInterval()));
                            mPollingTimer.start(1000);

                            return;
                        }
                    }
                    else
                    {	// second time through
                        if (mIPResultsReturned != mNumIPs)
                        {
                            ORIGIN_LOG_WARNING << "All responses not accounted for on second pass. (" << mIPResultsReturned << " - " << mNumIPs << ")";
                        }

                        // either way destroy protoping
                        ProtoPingDestroy(mProtoPingRef);
                        mProtoPingRef = NULL;
                    }
                }

                ResponseInfo responseInfo;
                // craft JSON format response as array of properties { "ip":"xxx.xxx.xxx.xxx" , "time":<time> } , ...

                responseInfo.response = "[\n\t";

                for (int i = 0; i < mNumIPs; i++)
                {
                    responseInfo.response.append("{\"ip\":\"" + mIPAddrStrings[i] + "\",\"time\":" + QString("%1").arg(mIPPingResults[i]) + "}");
                    if ((i + 1) < mNumIPs)  // more coming?
                        responseInfo.response.append(",\n\t");
                }

                responseInfo.response.append("\n]");

                responseInfo.errorCode = HttpStatus::OK;

                emit processCompleted(responseInfo);

                executePingRequest();   // send out next one if any
            }
        }
    }
}
