//
//  EntitlementService.h
//  
//
//  Created by Kiss, Bela on 12-06-08.
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
//

#ifndef _EntitlementService_h
#define _EntitlementService_h

#include <QObject>

class EntitlementService : public QObject
{
    Q_OBJECT
    
public:

    static void init();
    static EntitlementService* instance();
    
    static void fetchEntitlements(QStringList const& args);
    
protected slots:

    static void onFetchEntitlementsFinished();
    
private:

    EntitlementService();
    
    static EntitlementService* sInstance;
};

#endif
