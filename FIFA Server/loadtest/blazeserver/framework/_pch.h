//pchenabled:yes
//pchexclude:taskscheduler/**.cpp
//pchexclude:oauth/**.cpp

#pragma once

#include "EAIO/EAFileUtil.h"

#include "framework/controller/processcontroller.h"
#include "framework/controller/mastercontroller.h"
#include "framework/config/config_file.h"
#include "framework/connection/connectionmanager.h"
#include "framework/database/dbscheduler.h"
#include "framework/event/eventmanager.h"
#include "framework/storage/storagemanager.h"
#include "framework/vault/vaultmanager.h"
#include "framework/redis/redis.h"

#include "framework/config/configmerger.h"
#include "framework/config/configdecoder.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/connection/socketutil.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "framework/grpc/outboundgrpcmanager.h"
#include "framework/metrics/outboundmetricsmanager.h"
#include "framework/system/serverrunnerthread.h"
#include "framework/util/profanityfilter.h"
#include "framework/util/localization.h"
#include "framework/protocol/httpxmlprotocol.h"
#include "framework/protocol/eventxmlprotocol.h"
#include "framework/protocol/restprotocol.h"
#include "framework/system/allocation.h"
#include "framework/system/debugsys.h"
#include "framework/slivers/slivermanager.h"
#include "framework/redirector/redirectorutil.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
