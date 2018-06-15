#!/bin/sh
find src -type f | xargs sed -ri "/^.+Copyright \(c\) ([0-9]+-[0-9]+)|$(date -u +%Y)/! s/^(.+)Copyright \(c\) ([0-9]+) $(sedescq "$1")/\1Copyright \(c\) \2-$(date -u +%Y) $(sedescrep "$1")/"
find src -type f | xargs sed -ri "s/Copyright \(c\) ([0-9]+)-[0-9]+ $(sedescq "$1")/Copyright \(c\) \1-$(date -u +%Y) $(sedescrep "$1")/"
