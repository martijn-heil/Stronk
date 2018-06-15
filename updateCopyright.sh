#!/bin/sh
if [ $# -eq 0 ]
  then
    while true; do
      read -p "No author name provided. Are you sure that you want to update copyright for ALL authors? " yn
      case $yn in
          [Yy]* ) break;;
          [Nn]* ) echo "Cancelling"; exit;;
          * ) echo "Please answer yes or no.";;
      esac
    done
fi
author="'$*'"

find src -type f | xargs sed -ri "/^.+Copyright \(c\) ([0-9]+-[0-9]+)|$(date -u +%Y)/! s/^(.+)Copyright \(c\) ([0-9]+) $(sedescq "$author")/\1Copyright \(c\) \2-$(date -u +%Y) $(sedescrep "$author")/"
find src -type f | xargs sed -ri "s/Copyright \(c\) ([0-9]+)-[0-9]+ $(sedescq "$author")/Copyright \(c\) \1-$(date -u +%Y) $(sedescrep "$author")/"
