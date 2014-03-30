#!/usr/bin/awk -f

$5 ~ /solid/ { solid = 1 }
$5 ~ /fluid/ { solid = 0 }
solid && /^Min/ { print $10-273.15, $19-273.15  }
