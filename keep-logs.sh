#!/bin/bash
mydate=`date +%Y%m%d-%H%M%S`
dir="log-$mydate"
mkdir $dir
mv log-*.txt $dir
mv log.latest $dir
cp statistic-result.pl $dir
mv $dir logs/
echo "logs stored in logs/$dir, log refreshed."
