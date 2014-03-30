#!/usr/bin/awk -f

/^Create solid/{printf $6 ","} /^\*/{exit 0}
