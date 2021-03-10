#arg1 file name
#arg3 distinct object
#arg4 total point
#arg5 process num

#!/bin/bash

filename=$1
obj=$2
pntNum=$3
proNum=$4
init=10000



if [[ $# -ne 4 ]]; then
	echo "Illegal number of parameters"
	exit
fi

# if [[ $obj -lt 10000 ]]; then
# 	echo "illegal arguments more obj needed"
# 	exit
# fi

# obj=$(((obj * 4) / 3))
step=$((obj / pntNum))
interval=$((step * proNum))
cSize=1
echo $interval
echo $obj
echo $cSize



# for (( i = 0; cSize < 100 && i < 10; i++, cSize+=10 )); do
# 	../runnable/rand_sam_evt_mrc $filename $cSize $samSize &
# done

# wait
# echo $cSize



# for (( j = 0; j < 10; j++ )); do
# 	for (( i = 0; cSize < 10000 && i < 10; i++, cSize+=100 )); do
# 		../runnable/rand_sam_evt_mrc $filename $cSize $samSize &
# 	done

# 	wait
# 	echo $cSize
# done

for (( j = $interval; j <= $obj; j+=$interval )); do
	
	for (( i=0 ; $cSize <= j; cSize+=$step )); do
		output="${filename}_basic_lfu.mrc"
		./basic_lfu_cache $filename $cSize 0 2>> $output &
	done
	
	wait
	echo $cSize 
done

# for (( j = 10; j <= 1700; )); do
	
# 	for (( i=0 ; i <= 11; j+=10 i++ )); do
# 		../runnable/rand_sam_evt_mrc $filename $j $samSize &
# 	done
	
# 	wait
# 	echo $cSize 
# done
