#!/bin/bash 
LOOP=10
THREAD=("32" "24" "16" "8" "4" "2" "1")
LENGTH_THREAD=${#THREAD[@]}
COUNT_THREAD=0

echo -e "----------AKGROUP-----------\n"
echo -e "8-1-1"
echo -e "Performing SigMod ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ]
	do
		./harness corpus/init-file.txt corpus/workload-file.txt corpus/result-file.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done

echo -e "Performing Pockec ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ]; do
		./harness corpus/init-pokec.txt corpus/workload-pokec.txt corpus/result-pokec.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done

echo -e "Performing LiveJournal ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ]; do
		./harness corpus/init-livejournal.txt corpus/workload-livejournal.txt corpus/result-livejournal.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done

echo -e "\n5-4-1"
echo -e "Performing SigMod ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ];	do
		./harness corpus/init-file.txt corpus/workload-file415_1M.txt corpus/result-file415_1M.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done

echo -e "Performing Pockec ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ]; do
		./harness corpus/init-pokec.txt corpus/workload-pokec415_1M.txt corpus/result-pokec415_1M.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done

echo -e "Performing LiveJournal ................"
for (( i=0; i<${LENGTH_THREAD}; i++ ));
do
	echo -e "\nT=${THREAD[$i]}---"
	COUNTER=0
	while [  $COUNTER -lt $LOOP ]; do
		./harness corpus/init-livejournal.txt corpus/workload-livejournal415_1M.txt corpus/result-livejournal415_1M.txt ./akgroup/akgroup${THREAD[$i]} 2> /dev/null
		let COUNTER=COUNTER+1
	done
done


