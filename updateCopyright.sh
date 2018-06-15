#!/bin/sh
find -type f | xargs sed -ri "/^.+Copyright \(c\) ([0-9]+-)*$(date -u +%Y)/! s/^(.+)Copyright \(c\) ([0-9]+)/\1Copyright \(c\) \2-$(date -u +%Y)/"
