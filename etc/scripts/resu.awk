#!/usr/bin/awk -f

BEGIN {
    for (i=2; i < ARGC; i++)
        regions[i] = ARGV[i]
    ARGC = 2
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

    if (isRegion && \
         ( $0 ~ /Solving for Ux/ \
        || $0 ~ /Solving for Uy/ \
        || $0 ~ /Solving for Uz/) )
    {
        residual = $12
        sub(/,/, "", residual)
        print log(residual)/log(10)
    }
}

