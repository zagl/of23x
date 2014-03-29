#!/usr/bin/awk -f

/^Solving for solid region.*$/ { solid = 1 }
/^Solving for fluid region.*$/ { solid = 0 }
solid && /^Min/ { print $10 , $19  }
