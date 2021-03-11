#arg1 file name
#arg3 distinct object
#arg4 total point
#arg5 process num

#!/bin/bash

app=$1
filename=$2
obj=$3
pntNum=$4
proNum=$5


if [[ $# -ne 5 ]]; then
	echo "Illegal number of parameters"
	exit
fi


# obj=$(((obj * 4) / 3))
step=$((obj / pntNum))
interval=$((step * proNum))
cSize=1
echo $interval
echo $obj
echo $cSize


for (( j = $interval; j <= $obj; j+=$interval )); do
	
	for (( i=0 ; $cSize <= j; cSize+=$step )); do
		output="${filename}_${app}.mrc"
		$app $filename $cSize 0 2>> $output &
	done
	
	wait
	echo $cSize 
done
