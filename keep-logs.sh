#!/bin/bash
mydate=`date +%Y%m%d-%H%M%S`
dir="log-$mydate"
mkdir $dir
mv log-*.txt $dir
mv log.latest $dir
cp statistic-result.pl $dir
cd $dir
./statistic-result.pl > result.txt
cd ..
mv $dir logs/
cd logs/
tar zcvf $dir.tar.gz $dir

echo "logs stored in logs/$dir, log refreshed."
