#!/usr/bin/awk -f

BEGIN {
    for (i=1; i < ARGC; i++)
        regions[i] = ARGV[i]
    ARGC = 1
    isRegion = 0
}

{
    if ($0 ~ /^Solving for/)
    {
        isRegion = 0
        for (i in regions)
        {
            if ($0 ~ regions[i] "$" )
            {
                isRegion = 1
            }
        }
    }

    if (isRegion && $0 ~ /^Min/)
    {
        print $10 - 273.15, $19 - 273.15
    }
}

