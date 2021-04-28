# run script: cat slavebefore.csv slaveafter.csv | gawk -f rpc_ratediff.awk > slave_before_after_diff.csv
BEGIN {
    split("", gCompRates, "") # define empty array
    split("", gCompRpcRates, "") # define empty array
    gPerComponentSectionCount = 0
    gPerRpcSectionCount = 0
    gInSection = 0
    gInError = 0
}

END {
    if (gInError > 0)
        exit 1
    # print per component rpc rate diff
    # copy indices
    j = 1
    for (i in gCompRates) {
        ind[j] = i    # index value becomes element value
        j++
    }
    n = asort(ind)    # index values are now sorted
    printf("[per/component rpc rates diff]\n")
    printf("component,avgrate1,avgrate2,maxrate1,maxrate2\n")
    # print results using sorted component name indexes
    for (i = 1; i <= n; i++)
    {
        compName = ind[i]
        split(gCompRates[compName], curVals, ",")
        printf("%s,%f,%f,%f,%f\n", compName, curVals[1], curVals[3], curVals[2], curVals[4])
        # printf("%s,%s\n", compName, gCompRates[compName])
    }
    printf("\n")
    
    # print per rpc rate diff
    # copy indices
    j = 1
    for (i in gCompRpcRates) {
        ind[j] = i    # index value becomes element value
        j++
    }
    n = asort(ind)    # index values are now sorted
    printf("[per/rpc rates diff]\n")
    printf("component,rpc,avgrate1,avgrate2,maxrate1,maxrate2\n")
    # print results using sorted component name indexes
    for (i = 1; i <= n; i++)
    {
        compRpcName = ind[i]
        periodIdx = index(compRpcName, ".")
        compName = substr(compRpcName, 1, periodIdx-1)
        rpcName = substr(compRpcName, periodIdx+1)
        split(gCompRpcRates[compRpcName], curVals, ",")
        printf("%s,%s,%f,%f,%f,%f\n", compName, rpcName, curVals[1], curVals[3], curVals[2], curVals[4])
        # printf("%s,%s,%s\n", compName, rpcName, gCompRpcRates[compRpcName])
    }
    printf("\n")
}

{
    if (gInSection == 1)
    {
        valuesCount = split($0, valueArr, ",")
        if (valuesCount == 4)
        {
            compName = valueArr[1]
            # skip minVal @ valueArr[2]
            maxVal = valueArr[3] + 0
            avgVal = valueArr[4] + 0
            if (compName in gCompRates)
            {
                oldVal = gCompRates[compName]
                split(oldVal, curVals, ",")
                value = sprintf("%s,%s,%f,%f", curVals[1], curVals[2], avgVal, maxVal)
                gCompRates[compName] = value
            }
            else
            {
                if (gPerComponentSectionCount == 1)
                    value = sprintf("%f,%f,0,0", avgVal, maxVal)
                else
                    value = sprintf("0,0,%f,%f", avgVal, maxVal)
                gCompRates[compName] = value
            }
        }
    }
    else if (gInSection == 2)
    {
        valuesCount = split($0, valueArr, ",")
        if (valuesCount == 5)
        {
            compName = valueArr[1]
            rpcName = valueArr[2]
            # skip minVal @ valueArr[3]
            maxVal = valueArr[4] + 0
            avgVal = valueArr[5] + 0
            compRpcName = compName "." rpcName
            if (compRpcName in gCompRpcRates)
            {
                oldVal = gCompRpcRates[compRpcName]
                split(oldVal, curVals, ",")
                value = sprintf("%s,%s,%f,%f", curVals[1], curVals[2], avgVal, maxVal)
                gCompRpcRates[compRpcName] = value
            }
            else
            {
                if (gPerRpcSectionCount == 1)
                    value = sprintf("%f,%f,0,0", avgVal, maxVal)
                else
                    value = sprintf("0,0,%f,%f", avgVal, maxVal)
                gCompRpcRates[compRpcName] = value
            }
        }
    }
}

/^component,minrate,maxrate,avgrate$/ {
    if (gPerComponentSectionCount > 1)
    {
        printf("ERROR 1: section '%s' cannot occur more than twice in the input stream.\n", $0)
        gInError = 1
        exit 1
    }
    gInSection = 1
    gPerComponentSectionCount++
}

/^component,rpc,minrate,maxrate,avgrate$/ {
    if (gPerRpcSectionCount > 1)
    {
        printf("ERROR 2: section '%s' cannot occur more than twice in the input stream.\n", $0)
        gInError = 1
        exit 1
    }
    gInSection = 2
    gPerRpcSectionCount++
}

/^$/ {
    # blank line resets the rate section
    gInSection = 0
}
