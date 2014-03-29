#!/usr/bin/awk -f


start && /^Solving for fluid region.*$/ { exit 0 }
!start && /^Solving for fluid region.*$/ { start = 1 }
/^Solving for solid region.*$/ { solids = solids $5 "," }

END { 
        print solids
    }
