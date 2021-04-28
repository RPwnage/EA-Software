# this script is invoked by rpc_rategen.sh

# to manually run script: cat graphiteoutput.txt | gawk -f rpc_rates.awk > out.csv

BEGIN {

    split("", gCompRateMax, "") # define empty array

    split("", gCompRateMin, "") # define empty array

    split("", gCompRateAvg, "") # define empty array

    split("", gCompRpcRateMax, "") # define empty array

    split("", gCompRpcRateMin, "") # define empty array

    split("", gCompRpcRateAvg, "") # define empty array

    gDetailRatesBuf=""

}



END {

    # copy indices

    j = 1

    for (i in gCompRateMax) {

        ind[j] = i    # index value becomes element value

        j++

    }

    n = asort(ind)    # index values are now sorted

    printf("[rpc_rates.awk, sampled @ 5m]\n\n")

    printf("[per/component rpc rates]\n")

    printf("component,minrate,maxrate,avgrate\n")

    # print results using sorted component name indexes

    for (i = 1; i <= n; i++)

    {

        compName = ind[i]

        printf("%s,%f,%f,%f\n", compName, gCompRateMin[compName], gCompRateMax[compName], gCompRateAvg[compName])

    }

    printf("\n")

    

    # copy indices

    j = 1

    for (i in gCompRpcRateMax) {

        ind[j] = i    # index value becomes element value

        j++

    }

    n = asort(ind)    # index values are now sorted

    printf("[per/rpc rates]\n")

    printf("component,rpc,minrate,maxrate,avgrate\n")

    # print results using sorted component name indexes

    for (i = 1; i <= n; i++)

    {

        compRpcName = ind[i]

        periodIdx = index(compRpcName, ".")

        compName = substr(compRpcName, 1, periodIdx-1)

        rpcName = substr(compRpcName, periodIdx+1)

        printf("%s,%s,%f,%f,%f\n", compName, rpcName, gCompRpcRateMin[compRpcName], gCompRpcRateMax[compRpcName], gCompRpcRateAvg[compRpcName])

    }

    printf("\n")

    

    printf("[per/instance rpc rates]\n")

    printf("server,component,rpc,minrate,maxrate,avgrate\n")

    print gDetailRatesBuf

}



{

    bracket1Idx = index($0, "(")

    bracket2Idx = index($0, ")")

    header = substr($0, bracket1Idx+1, bracket2Idx-bracket1Idx-1)

    headerCount = split(header, headerArr, ".")

    rpcName = headerArr[headerCount-1]

    compName = headerArr[headerCount-3]

    serverName = headerArr[headerCount-4]

    #printf("comp:%s,\trpc: %s\n", compName, rpcName)

    valuesIdx = index($0, "|")

    values = substr($0, valuesIdx+1)

    # printf("values: %s\n", values)

    valuesCount = split(values, valueArr, ",")

    # printf("valuesCount: %u\n", valuesCount)

    valMin = 1000000.0

    valMax = 0.0

    valAvg = 0.0

    for (iVal in valueArr)

    {

        arrVal = valueArr[iVal]

        if (isnum(arrVal))

        {

            v = arrVal+0

            if (v > valMax)

            {

                valMax = v

            }

            else if (v < valMin)

            {

                valMin = v

            }

            valAvg += arrVal/valuesCount

        }

    }

    if (valMin > valMax)

        valMin = valMax

    # printf("valmin: %f, valmax: %f\n", valMin, valMax)

    if (compName in gCompRateMax)

        gCompRateMax[compName] += valMax

    else

        gCompRateMax[compName] = valMax



    if (compName in gCompRateMin)

        gCompRateMin[compName] += valMin

    else

        gCompRateMin[compName] = valMin



    if (compName in gCompRateAvg)

        gCompRateAvg[compName] += valAvg

    else

        gCompRateAvg[compName] = valAvg



    compRpcName = compName "." rpcName

    if (compRpcName in gCompRpcRateMax)

        gCompRpcRateMax[compRpcName] += valMax

    else

        gCompRpcRateMax[compRpcName] = valMax

        

    if (compRpcName in gCompRpcRateMin)

        gCompRpcRateMin[compRpcName] += valMin

    else

        gCompRpcRateMin[compRpcName] = valMin

    

    if (compRpcName in gCompRpcRateAvg)

        gCompRpcRateAvg[compRpcName] += valAvg

    else

        gCompRpcRateAvg[compRpcName] = valAvg

    

    # printf("%s,%s,%s,%f,%f\n", serverName, compName, rpcName, valMin, valMax)

    gDetailRatesBuf = (gDetailRatesBuf "" sprintf("%s,%s,%s,%f,%f,%f\n", serverName, compName, rpcName, valMin, valMax, valAvg))

}



function isnum(x){return(x==x+0)}